struct free_list_chunk;
struct list
{
  memory Memory;
  unsigned int SpaceRemaining;
  free_list_chunk *Head;
};

struct free_list_chunk
{
  free_list_chunk *Next;
  unsigned int Size;
  unsigned char Padding;
};

struct allocated_list_chunk
{
  unsigned int Size;
  unsigned char Padding;
};

internal void
InitializeList(list *List, size_t Size)
{
  List->Memory.AllocatedSpace = malloc(Size);
  List->Memory.Size = Size;
  List->SpaceRemaining = Size;
  List->Head = (free_list_chunk *)List->Memory.AllocatedSpace;
  List->Head->Next = NULL;
  List->Head->Size = Size;
  List->Head->Padding = 0;
}

internal size_t
AlignAddress(size_t Address, unsigned char Alignment)
{
  assert(Alignment > 0);

  int AlignmentRemaining = Alignment;
  int AlignedAddress = Address;

  while (AlignmentRemaining > 1)
  {
    AlignmentRemaining /= 2;
    AlignedAddress = AlignedAddress >> 1;
  }

  AlignmentRemaining = Alignment;

  while (AlignmentRemaining > 1)
  {
    AlignmentRemaining /= 2;
    AlignedAddress = AlignedAddress << 1;
  }

  return AlignedAddress;
}

// Note (sigmasleep): Assumes largest alignment is 8.
// Assumes no packing on header, therefore if header is aligned, the content is aligned.
internal void*
FindAndResizeFittingChunkFromList(list *List, unsigned int Size, unsigned char Alignment)
{
  if (List->Head == NULL)
  {
    return NULL;
  }

  free_list_chunk *Chunk = List->Head;

  size_t FittingChunkAddress = ((size_t)Chunk + Chunk->Size + Chunk->Padding - Size);
  size_t AlignedFittingChunkAddress = AlignAddress(FittingChunkAddress, Alignment);

  size_t HeaderAddress = AlignedFittingChunkAddress - sizeof(allocated_list_chunk);
  size_t AlignedHeaderAddress = AlignAddress(HeaderAddress, alignof(allocated_list_chunk));

  unsigned int AllocationSize = Size + (FittingChunkAddress - AlignedFittingChunkAddress);
  unsigned char AllocationPadding = (unsigned char)(AlignedFittingChunkAddress - AlignedHeaderAddress);

  size_t TotalSize = AllocationSize + AllocationPadding;

  while (TotalSize > Chunk->Size)
  {
    Chunk = Chunk->Next;
    if (Chunk == NULL)
    {
      return NULL;
    }
  
    FittingChunkAddress = ((size_t)Chunk + Chunk->Size - Size);
    AlignedFittingChunkAddress = AlignAddress(FittingChunkAddress, Alignment);

    HeaderAddress = AlignedFittingChunkAddress - sizeof(allocated_list_chunk);
    AlignedHeaderAddress = AlignAddress(HeaderAddress, alignof(allocated_list_chunk));

    AllocationSize = Size + (FittingChunkAddress - AlignedFittingChunkAddress);
    AllocationPadding = (unsigned char)(AlignedFittingChunkAddress - AlignedHeaderAddress);
    
    TotalSize = AllocationSize + AllocationPadding;
  }

  Chunk->Size -= TotalSize;
  List->SpaceRemaining -= TotalSize;

  allocated_list_chunk *Allocation = (allocated_list_chunk *)AlignedHeaderAddress;
  Allocation->Size = AllocationSize;
  Allocation->Padding = AllocationPadding;

  if (List->SpaceRemaining == 0)
  {
    List->Head = NULL;
  }

  return (void *)(AlignedFittingChunkAddress);
}

internal unsigned int
GetSizeOfAllocation(void* Address)
{
  size_t AllocationAddress = AlignAddress((size_t)Address - sizeof(allocated_list_chunk), alignof(allocated_list_chunk));
  allocated_list_chunk *Allocation = (allocated_list_chunk *)(AllocationAddress);

  return Allocation->Size;
}

internal unsigned char
GetPaddingOfAllocation(void* Address)
{
  size_t AllocationAddress = AlignAddress((size_t)Address - sizeof(allocated_list_chunk), alignof(allocated_list_chunk));
  allocated_list_chunk *Allocation = (allocated_list_chunk *)(AllocationAddress);

  return Allocation->Padding;
}

// Todo(sigmasleep): Add alignment
#define AllocateSpaceOnList(Stack, Type) AllocateSpaceOnList_(Stack, sizeof(Type), alignof(Type))
internal void*
AllocateSpaceOnList_(list *List, unsigned int Size, unsigned char Alignment)
{
  return FindAndResizeFittingChunkFromList(List, Size, Alignment);
}

//#define DeallocateSpaceOnList(List, TypedPointer) DeallocateSpaceOnList_(List, (void *)TypedPointer))
internal void
DeallocateSpaceOnList(list *List, void* Address)
{
  size_t AllocationAddress = AlignAddress((size_t)Address - sizeof(allocated_list_chunk), alignof(allocated_list_chunk));
  allocated_list_chunk *Allocation = (allocated_list_chunk *)(AllocationAddress);

  free_list_chunk *Chunk = List->Head;
  free_list_chunk *PreviousChunk = NULL;
  
  if(Chunk == NULL)
  {
    List->Head = (free_list_chunk *)Allocation;
    List->Head->Size = Allocation->Padding + Allocation->Size;
    List->Head->Padding = 0;
    List->Head->Next = NULL;
    List->SpaceRemaining += List->Head->Size;
    return;
  }

  while (Chunk != NULL && (size_t)Chunk < AllocationAddress)
  {
    PreviousChunk = Chunk;
    Chunk = Chunk->Next;
  }
  // Note(sigmasleep): Order of structs: PreviousChunk | Allocation | Chunk
 
  bool AllocationIsAdjacentToNextChunk = (size_t)Allocation + Allocation->Padding + Allocation->Size == (size_t)Chunk;
  bool AllocationIsAdjacentToPriorChunk = PreviousChunk != NULL &&
    (size_t)PreviousChunk + PreviousChunk->Padding + PreviousChunk->Size == (size_t)Allocation;

  //unsigned int AllocationSize = Size + Allocation->Padding;
  //List->SpaceRemaining += AllocationSize;

  if (!(AllocationIsAdjacentToNextChunk || AllocationIsAdjacentToPriorChunk))
  {
    /*free_list_chunk *NewChunk = (free_list_chunk *)((size_t)Allocation + ChunkPadding);
    NewChunk->Size = AllocationSize;
    NewChunk->Padding = ChunkPadding;
    NewChunk->Next = Chunk;*/
  }
  else
  {
    size_t NewFreeChunkAddress = (size_t)Address - sizeof(allocated_list_chunk);
    size_t AlignedNewFreeChunkAddress = AlignAddress(NewFreeChunkAddress, alignof(free_list_chunk));

    unsigned int NewFreeChunkSize = Allocation->Size + Allocation->Padding;
    
    if (AllocationIsAdjacentToNextChunk)
    {
      NewFreeChunkSize = Chunk->Size + Chunk->Padding + Allocation->Size;
      unsigned char NewFreeChunkPadding = (unsigned char)(NewFreeChunkAddress - AlignedNewFreeChunkAddress);

      PreviousChunk->Next = (free_list_chunk *)AlignedNewFreeChunkAddress;
      PreviousChunk->Next->Size = NewFreeChunkSize;
      PreviousChunk->Next->Padding = NewFreeChunkPadding;
      PreviousChunk->Next->Next = Chunk;

      // Note(sigmasleep): Sets up values for: if (AllocationIsAdjacentToPriorChunk)
      NewFreeChunkSize += NewFreeChunkPadding;
      Chunk = Chunk->Next;
    }
    if (AllocationIsAdjacentToPriorChunk)
    {
      PreviousChunk->Size += NewFreeChunkSize; //AllocationSize;
      PreviousChunk->Next = Chunk;
    }

    List->SpaceRemaining += NewFreeChunkSize;
  }
}