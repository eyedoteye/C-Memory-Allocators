#define internal static
#define global static
#define local_persist static

struct free_list_header;
struct list
{
  memory Memory;
  size_t SpaceRemaining;
  free_list_header *Head;
};

struct free_list_header
{
  free_list_header *Next;
  size_t ChunkSize;
  unsigned char Offset;
};

struct allocated_list_header
{
  size_t ChunkSize;
  unsigned char PrePadding : 6;
	unsigned char Type : 2;
};

// Note: Having the post padding value be seperate allows it to be packed right
// before the allocated chunk, This allows for easier lookup of the allocated
// header position given only the allocated chunk position. The type must be the
// smallest type available. This ensures it is always aligned if the allocated
// chunk is aligned.
typedef unsigned char allocated_list_header_post_padding;

size_t
AlignAddress(size_t Address, unsigned char Alignment)
{
  assert(Alignment > 0);

	int Shift = 0;

  while(Alignment > 1)
  {
    Alignment /= 2;
		Shift += 1;
  }

  return (Address >> Shift) << Shift;
}

// Note: This struct groups together the return values for the
// GetAllocatedHeaderFromFreeHeader method.
struct allocated_list_header_properties
{
  size_t Address;
  size_t ChunkSize;
  unsigned char PrePadding;
	unsigned char Type;
  allocated_list_header_post_padding PostPadding;
};

internal void
GetAllocatedHeaderFromFreeHeader(
  allocated_list_header_properties *AllocatedHeaderProperties,
  free_list_header *FreeHeader,
  unsigned char RequiredAlignment)
{
  size_t BaseAddress = (size_t)FreeHeader - FreeHeader->Offset;
  size_t AllocatedHeaderAddress =
    AlignAddress(BaseAddress + alignof(allocated_list_header) - 1,
                 alignof(allocated_list_header));
  size_t ChunkAddress = AlignAddress(AllocatedHeaderAddress
                                     + sizeof(allocated_list_header)
                                     + sizeof(allocated_list_header_post_padding)
                                     + RequiredAlignment - 1, RequiredAlignment);
  AllocatedHeaderProperties->PrePadding =
    (unsigned char)(AllocatedHeaderAddress - BaseAddress);
  AllocatedHeaderProperties->PostPadding =
    (unsigned char)(ChunkAddress
                    - (AllocatedHeaderAddress + sizeof(allocated_list_header)));
  AllocatedHeaderProperties->ChunkSize =
    (size_t)(BaseAddress + FreeHeader->ChunkSize - ChunkAddress);
  AllocatedHeaderProperties->Address = AllocatedHeaderAddress;
}

internal void
InitializeList(list *List, size_t Size)
{
  List->Memory.AllocatedSpace = malloc(Size);
  List->Memory.Size = Size;
  List->SpaceRemaining = Size;
  List->Head = (free_list_header *)List->Memory.AllocatedSpace;
  List->Head->Next = NULL;
  List->Head->ChunkSize = Size;
  List->Head->Offset = 0;
}

internal void*
FindAndResizeFittingChunkFromList(
  list *List, size_t RequestedSize,
  unsigned char Alignment)
{
  if (List->Head == NULL)
  {
    return NULL;
  }

  free_list_header *PreviousFreeHeader = NULL;
  free_list_header *FreeHeader = List->Head;

  allocated_list_header_properties PotentialAllocatedHeaderProperties;
  GetAllocatedHeaderFromFreeHeader(&PotentialAllocatedHeaderProperties,
                                   FreeHeader, Alignment);

  size_t AllocatedChunkAddress = PotentialAllocatedHeaderProperties.Address
    + sizeof(allocated_list_header)
    + PotentialAllocatedHeaderProperties.PostPadding;
  size_t RequiredSize = PotentialAllocatedHeaderProperties.PrePadding
    + sizeof(allocated_list_header)
    + PotentialAllocatedHeaderProperties.PostPadding 
    + RequestedSize;

  while (RequiredSize > FreeHeader->ChunkSize)
  {
    PreviousFreeHeader = FreeHeader;
    FreeHeader = FreeHeader->Next;
    if (FreeHeader == NULL)
    {
      return NULL;
    }
  
    GetAllocatedHeaderFromFreeHeader(&PotentialAllocatedHeaderProperties,
                                     FreeHeader, Alignment);

    AllocatedChunkAddress = PotentialAllocatedHeaderProperties.Address
      + sizeof(allocated_list_header)
      + PotentialAllocatedHeaderProperties.PostPadding;
    RequiredSize = PotentialAllocatedHeaderProperties.PrePadding
      + sizeof(allocated_list_header)
      + PotentialAllocatedHeaderProperties.PostPadding
      + RequestedSize;
  }
  // Move FreeHeader Pointer To Appropriate Location And Reduce Size

  // Todo(sigmasleep): Rewrite this to factor in alignment
	size_t RemainingChunkSize = FreeHeader->ChunkSize - RequiredSize;
	size_t LeftoverSize = 0;

	if(RemainingChunkSize < sizeof(free_list_header))
  {
    // Header Needs To Be Removed And Remaining Space Pushed To Adjacent Allocations.
		LeftoverSize += RemainingChunkSize;

    if(PreviousFreeHeader == NULL)
    {
      List->Head = FreeHeader->Next;
    }
    else
    {
      PreviousFreeHeader->Next = FreeHeader->Next;
    }
  }
  else
  {
    // Previous FreeHeader Pointer To Next Needs To Be Updated

    size_t UpdatedFreeChunkAddress = AllocatedChunkAddress + RequestedSize;
    size_t UpdatedFreeHeaderAddress = AlignAddress(UpdatedFreeChunkAddress + alignof(free_list_header) - 1, alignof(free_list_header));

    free_list_header *NextFreeHeader = FreeHeader->Next;
    size_t UpdatedChunkSize = FreeHeader->ChunkSize - RequiredSize;
    unsigned char UpdatedOffset = unsigned char(UpdatedFreeHeaderAddress - UpdatedFreeChunkAddress);

    if(PreviousFreeHeader == NULL)
    {
      List->Head = (free_list_header *)UpdatedFreeHeaderAddress;
      List->Head->Next = NextFreeHeader;
      List->Head->ChunkSize = UpdatedChunkSize;
      List->Head->Offset = UpdatedOffset;
    }
    else
    {
      PreviousFreeHeader->Next = (free_list_header *)UpdatedFreeHeaderAddress;
      PreviousFreeHeader->Next->Next = NextFreeHeader;
      PreviousFreeHeader->Next->ChunkSize = UpdatedChunkSize;
      PreviousFreeHeader->Next->Offset = UpdatedOffset;
    }
  }

  List->SpaceRemaining -= RequiredSize + LeftoverSize;

	unsigned char Type = 0;
  size_t SizeForTyping = RequestedSize; 
	while(SizeForTyping > 1)
	{
		++Type;
		SizeForTyping /= 2;
	}

  allocated_list_header *AllocatedHeader = (allocated_list_header *)PotentialAllocatedHeaderProperties.Address;
  AllocatedHeader->PrePadding = PotentialAllocatedHeaderProperties.PrePadding; 
  AllocatedHeader->ChunkSize = RequestedSize + LeftoverSize;
	AllocatedHeader->Type = Type;

  allocated_list_header_post_padding *AllocatedHeaderPostPadding = (allocated_list_header_post_padding *)
	  (AllocatedChunkAddress - sizeof(allocated_list_header_post_padding));
  *AllocatedHeaderPostPadding = PotentialAllocatedHeaderProperties.PostPadding;
  
  // Note: Places size of allocation leftover into chunk for debugging purposes.
  switch(Type)
  {
    case 0:
    {
      *((unsigned char *)AllocatedChunkAddress) = (unsigned char)LeftoverSize;
    } break;
    case 1:
    {
      *((size_t *)AllocatedChunkAddress) = (size_t)LeftoverSize;
    } break;
    case 2:
    {
      *((unsigned long *)AllocatedChunkAddress) = (unsigned long)LeftoverSize;
    } break;
  }

  return (void *)(AllocatedChunkAddress);
}

#define AllocateSpaceOnList(Stack, Type) \
        AllocateSpaceOnList_(Stack, sizeof(Type), alignof(Type))
          
internal void*
AllocateSpaceOnList_(list *List, size_t Size, unsigned char Alignment)
{
  return FindAndResizeFittingChunkFromList(List, Size, Alignment);
}

#pragma warning(push)
#pragma warning(disable:4505)
internal size_t
GetAllocatedHeaderAddress(size_t ChunkAddress)
{
  allocated_list_header_post_padding AllocatedHeaderPostPadding =
    (allocated_list_header_post_padding)ChunkAddress -
    sizeof(allocated_list_header_post_padding);

  return ChunkAddress - AllocatedHeaderPostPadding
    - sizeof(allocated_list_header);
}
#pragma warning(pop)

internal void
GetAllocatedHeaderFromAllocatedChunk(
  allocated_list_header_properties *AllocatedHeaderProperties,
  size_t ChunkAddress)
{
	allocated_list_header_post_padding *AllocatedHeaderPostPadding =
		(allocated_list_header_post_padding *)
    ((size_t)ChunkAddress - sizeof(allocated_list_header_post_padding));

	allocated_list_header *AllocatedHeader =
		(allocated_list_header *)((size_t)ChunkAddress
                              - *AllocatedHeaderPostPadding
                              - sizeof(allocated_list_header));

	AllocatedHeaderProperties->Address = (size_t)AllocatedHeader;
	AllocatedHeaderProperties->ChunkSize = AllocatedHeader->ChunkSize;
	AllocatedHeaderProperties->PrePadding = AllocatedHeader->PrePadding;
	AllocatedHeaderProperties->Type = AllocatedHeader->Type;
	AllocatedHeaderProperties->PostPadding = *AllocatedHeaderPostPadding;
}

internal void
DeallocateSpaceOnList(list *List, void* Address)
{
	allocated_list_header_properties AllocatedHeaderProperties;
	GetAllocatedHeaderFromAllocatedChunk(&AllocatedHeaderProperties,
                                       (size_t)Address);

  free_list_header *PreviousFreeHeader = NULL;
  free_list_header *FreeHeader = List->Head;

  while(FreeHeader != NULL
        && (size_t)FreeHeader - FreeHeader->Offset
        < AllocatedHeaderProperties.Address
        - AllocatedHeaderProperties.PrePadding)
  {
    PreviousFreeHeader = FreeHeader;
    FreeHeader = FreeHeader->Next;
  }

  // Note: Order of structs: PreviousFreeHeader | AllocatedHeader | FreeHeader
 
  bool AllocationIsAdjacentToNextFreeHeader = FreeHeader != NULL
    && AllocatedHeaderProperties.Address + sizeof(allocated_list_header)
		+ AllocatedHeaderProperties.PostPadding + AllocatedHeaderProperties.ChunkSize
		== (size_t)FreeHeader - FreeHeader->Offset;
  bool AllocationIsAdjacentToPriorFreeHeader = PreviousFreeHeader != NULL
    && (size_t)PreviousFreeHeader - PreviousFreeHeader->Offset
       + PreviousFreeHeader->ChunkSize
    == (size_t)AllocatedHeaderProperties.Address
       - AllocatedHeaderProperties.PrePadding; 

  size_t NewFreeHeaderChunkSize =
    AllocatedHeaderProperties.PrePadding + sizeof(allocated_list_header)
    + AllocatedHeaderProperties.PostPadding + AllocatedHeaderProperties.ChunkSize;
  size_t NewFreeHeaderChunkAddress =
    AllocatedHeaderProperties.Address - AllocatedHeaderProperties.PrePadding;
  size_t NewFreeHeaderAddress =
    AlignAddress(NewFreeHeaderChunkAddress + alignof(free_list_header)-1,
                                             alignof(free_list_header));
  unsigned char NewFreeHeaderOffset =
    (unsigned char)(NewFreeHeaderAddress - NewFreeHeaderChunkAddress);

	size_t AddedSpace = NewFreeHeaderChunkSize;

  if(!AllocationIsAdjacentToNextFreeHeader
     && !AllocationIsAdjacentToPriorFreeHeader)
  {
    free_list_header *NewFreeListHeader =
      (free_list_header *)(NewFreeHeaderAddress);
    
    NewFreeListHeader->ChunkSize = NewFreeHeaderChunkSize;
    NewFreeListHeader->Offset = NewFreeHeaderOffset;
    NewFreeListHeader->Next = FreeHeader;

    if(PreviousFreeHeader == NULL)
    {
      List->Head = NewFreeListHeader;

			if(FreeHeader == NULL)
			{
				// Note: Remaining space was too small for allocation and free list was
        // emptied. That leftover space needs to be accounted for.
				NewFreeListHeader->ChunkSize += List->SpaceRemaining;
			}
    }
    else
    {
      PreviousFreeHeader->Next = NewFreeListHeader;
    }
  }
  else
  {    
    if(AllocationIsAdjacentToNextFreeHeader)
    {
			if(PreviousFreeHeader == NULL)
			{
				List->Head = (free_list_header *)NewFreeHeaderAddress;
				List->Head->ChunkSize = NewFreeHeaderChunkSize + FreeHeader->ChunkSize;
				List->Head->Offset = NewFreeHeaderOffset;
				List->Head->Next = FreeHeader->Next;
			}
			else
			{
				PreviousFreeHeader->Next = (free_list_header *)NewFreeHeaderAddress;
				PreviousFreeHeader->Next->ChunkSize =
          NewFreeHeaderChunkSize + FreeHeader->ChunkSize;
				PreviousFreeHeader->Next->Offset = NewFreeHeaderOffset;
				PreviousFreeHeader->Next->Next = FreeHeader->Next;
			}
      
      // Note: Sets up values for: if (AllocationIsAdjacentToPriorChunk)
			NewFreeHeaderChunkSize += FreeHeader->ChunkSize;
			FreeHeader = FreeHeader->Next;
    }
    if(AllocationIsAdjacentToPriorFreeHeader)
    {
			PreviousFreeHeader->ChunkSize += NewFreeHeaderChunkSize;
			// Note: The following line is for merging with an allocated header that
      // has already been merged with an adjacent-next FreeHeader.
			PreviousFreeHeader->Next = FreeHeader;
    }
  }
	List->SpaceRemaining += AddedSpace;
}
