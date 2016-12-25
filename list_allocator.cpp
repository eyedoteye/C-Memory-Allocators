#define internal static
#define global static
#define local_persist static

struct free_list_header;
struct list
{
  memory Memory;
  unsigned int SpaceRemaining;
  free_list_header *Head;
};

struct free_list_header
{
  free_list_header *Next;
  unsigned int ChunkSize;
  signed char Offset;
};

struct allocated_list_header
{
  unsigned int ChunkSize;
  unsigned char PrePadding;
};

// Note(sigmasleep): Having the post padding value be seperate allows it to be packed right before the allocated chunk,
// This allows for easier lookup of the allocated header position given only the allocated chunk position.
// The type must be the smallest type available. This ensures it is always aligned if the allocated chunk is aligned.
typedef unsigned char allocated_list_header_post_padding;

internal size_t
AlignAddress(size_t Address, unsigned char Alignment)
{
  assert(Alignment > 0);

  int AlignmentShift = Alignment;

  while(AlignmentShift > 1)
  {
    AlignmentShift /= 2;
  }

  return (Address >> AlignmentShift) << AlignmentShift;
}

// Note(sigmasleep): This struct groups together the return values for the GetAllocatedHeaderFromFreeHeader method.
struct allocated_list_header_properties
{
  size_t Address;
  unsigned int ChunkSize;
  unsigned char PrePadding;
  allocated_list_header_post_padding PostPadding;
};

internal void
GetAllocatedHeaderFromFreeHeader(allocated_list_header_properties *AllocatedHeaderProperties,
                                 free_list_header *FreeHeader, unsigned char RequiredAlignment)
{
  size_t BaseAddress = (size_t)FreeHeader + FreeHeader->Offset;
  size_t AllocatedHeaderAddress = AlignAddress(BaseAddress + alignof(allocated_list_header) - 1,
                                               alignof(allocated_list_header));
  size_t ChunkAddress = AlignAddress(AllocatedHeaderAddress + sizeof(allocated_list_header) + sizeof(allocated_list_header_post_padding) + RequiredAlignment - 1,
                                     RequiredAlignment);
  AllocatedHeaderProperties->PrePadding = (unsigned char)(AllocatedHeaderAddress - BaseAddress);
  AllocatedHeaderProperties->PostPadding = (unsigned char)(ChunkAddress - (AllocatedHeaderAddress + sizeof(allocated_list_header)));
  AllocatedHeaderProperties->ChunkSize = (BaseAddress + FreeHeader->ChunkSize) - ChunkAddress;
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

// Note (sigmasleep): Assumes largest alignment is 8.
// Assumes no packing on header, therefore if header is aligned, the content is aligned.
internal void*
FindAndResizeFittingChunkFromList(list *List, unsigned int RequestedSize, unsigned char Alignment)
{
  if (List->Head == NULL)
  {
    return NULL;
  }

  free_list_header *PreviousFreeHeader = NULL;
  free_list_header *FreeHeader = List->Head;

  allocated_list_header_properties PotentialAllocatedHeaderProperties;
  GetAllocatedHeaderFromFreeHeader(&PotentialAllocatedHeaderProperties, FreeHeader, Alignment);

  size_t AllocatedChunkAddress = PotentialAllocatedHeaderProperties.Address + sizeof(allocated_list_header) + PotentialAllocatedHeaderProperties.PostPadding;
  size_t RequiredSize = PotentialAllocatedHeaderProperties.PrePadding + sizeof(allocated_list_header) + PotentialAllocatedHeaderProperties.PostPadding 
    + RequestedSize; // The AllocatedHeader's ChunkSize needs to be re-written to match the requested size.

  while (RequiredSize > FreeHeader->ChunkSize)
  {
    PreviousFreeHeader = FreeHeader;
    FreeHeader = FreeHeader->Next;
    if (FreeHeader == NULL)
    {
      return NULL;
    }
  
    GetAllocatedHeaderFromFreeHeader(&PotentialAllocatedHeaderProperties, FreeHeader, Alignment);

    AllocatedChunkAddress = PotentialAllocatedHeaderProperties.Address + sizeof(allocated_list_header) + PotentialAllocatedHeaderProperties.PostPadding;
    RequiredSize = PotentialAllocatedHeaderProperties.PrePadding + sizeof(allocated_list_header) + PotentialAllocatedHeaderProperties.PostPadding
      + RequestedSize;
  }
  // Move FreeHeader Pointer To Appropriate Location And Reduce Size

  // Todo(sigmasleep): Rewrite this to factor in alignment
  if(FreeHeader->ChunkSize - RequiredSize < sizeof(allocated_list_header) + 1)
  {
    // Header Needs To Be Removed And Remaining Space Pushed To Adjacent Allocations.

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
    signed char UpdatedOffset = signed char(UpdatedFreeHeaderAddress - UpdatedFreeChunkAddress);

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

  List->SpaceRemaining -= RequiredSize;

  allocated_list_header *AllocatedHeader = (allocated_list_header *)PotentialAllocatedHeaderProperties.Address;
  AllocatedHeader->PrePadding = PotentialAllocatedHeaderProperties.PrePadding; 
  AllocatedHeader->ChunkSize = PotentialAllocatedHeaderProperties.ChunkSize;

  allocated_list_header_post_padding *AllocatedHeaderPostPadding = (allocated_list_header_post_padding *)
	  (AllocatedChunkAddress - sizeof(allocated_list_header_post_padding));
  *AllocatedHeaderPostPadding = PotentialAllocatedHeaderProperties.PostPadding;

  return (void *)(AllocatedChunkAddress);
}

// Todo(sigmasleep): Add alignment
#define AllocateSpaceOnList(Stack, Type) AllocateSpaceOnList_(Stack, sizeof(Type), alignof(Type))
internal void*
AllocateSpaceOnList_(list *List, unsigned int Size, unsigned char Alignment)
{
  return FindAndResizeFittingChunkFromList(List, Size, Alignment);
}

internal size_t
GetAllocatedHeaderAddress(list List, size_t ChunkAddress)
{
  allocated_list_header_post_padding AllocatedHeaderPostPadding =
    (allocated_list_header_post_padding)(ChunkAddress - sizeof(allocated_list_header_post_padding));
  return ChunkAddress - AllocatedHeaderPostPadding - sizeof(allocated_list_header);
}

internal void
DeallocateSpaceOnList(list *List, void* Address)
{
  allocated_list_header_post_padding *AllocatedHeaderPostPadding = 
    (allocated_list_header_post_padding *)((size_t)Address - sizeof(allocated_list_header_post_padding));

  allocated_list_header *AllocatedHeader =
    (allocated_list_header *)((size_t)Address - *AllocatedHeaderPostPadding - sizeof(allocated_list_header));

  free_list_header *PreviousFreeHeader = NULL;
  free_list_header *FreeHeader = List->Head;

  while (FreeHeader != NULL && (size_t)FreeHeader < ((size_t)AllocatedHeader + *AllocatedHeaderPostPadding))
  {
    PreviousFreeHeader = FreeHeader;
    FreeHeader = FreeHeader->Next;
  }

  // Note(sigmasleep): Order of structs: PreviousFreeHeader | AllocatedHeader | FreeHeader
 
  bool AllocationIsAdjacentToNextChunk =
    (size_t)AllocatedHeader + *AllocatedHeaderPostPadding + AllocatedHeader->ChunkSize == (size_t)FreeHeader;
  bool AllocationIsAdjacentToPriorChunk = PreviousFreeHeader != NULL &&
    (size_t)PreviousFreeHeader + PreviousFreeHeader->ChunkSize == (size_t)AllocatedHeader;



  unsigned int NewFreeHeaderChunkSize = AllocatedHeader->PrePadding + sizeof(AllocatedHeader)
    + *AllocatedHeaderPostPadding + AllocatedHeader->ChunkSize;
  size_t NewFreeHeaderChunkAddress = (size_t)AllocatedHeader - AllocatedHeader->PrePadding;
  size_t NewFreeHeaderAddress = AlignAddress(NewFreeHeaderChunkAddress + alignof(free_list_header)-1,
                                             alignof(free_list_header));
  signed char NewFreeHeaderOffset = (signed char)(NewFreeHeaderAddress - NewFreeHeaderChunkAddress);

  if (!AllocationIsAdjacentToNextChunk && !AllocationIsAdjacentToPriorChunk)
  {
    free_list_header *NewFreeListHeader = (free_list_header *)(NewFreeHeaderAddress);
    
    NewFreeListHeader->ChunkSize = NewFreeHeaderChunkSize;
    NewFreeListHeader->Offset = NewFreeHeaderOffset;
    NewFreeListHeader->Next = FreeHeader;

    if(PreviousFreeHeader == NULL)
    {
      List->Head = NewFreeListHeader;
    }
    else
    {
      PreviousFreeHeader->Next = FreeHeader;
    }
  }
  else
  {    
    if (AllocationIsAdjacentToNextChunk)
    {
      PreviousFreeHeader->Next = (free_list_header *)NewFreeHeaderAddress;
      PreviousFreeHeader->Next->ChunkSize = NewFreeHeaderChunkSize;
      PreviousFreeHeader->Next->Offset = NewFreeHeaderOffset;
      PreviousFreeHeader->Next->Next = FreeHeader;

      // Note(sigmasleep): Sets up values for: if (AllocationIsAdjacentToPriorChunk)
      FreeHeader = FreeHeader->Next;
    }
    if (AllocationIsAdjacentToPriorChunk)
    {
      PreviousFreeHeader->ChunkSize += NewFreeHeaderChunkSize;
      // Note(sigmasleep): The following line is for merging with an allocated header
      // that has already been merged with an adjacent-next FreeHeader.
      PreviousFreeHeader->Next = FreeHeader;
    }
  }
  List->SpaceRemaining += NewFreeHeaderChunkSize;
}
