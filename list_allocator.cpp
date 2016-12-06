struct list_chunk;
struct list
{
  memory Memory;
  unsigned int SpaceRemaining;
  list_chunk *Head;
};

struct list_chunk
{
  list_chunk *Next;
  unsigned int Size;
  unsigned char AlignmentOffset;
};

struct allocated_list_header
{
  unsigned int Size;
  unsigned char AlignmentOffset;
};

internal void
InitializeList(list *List, size_t Size)
{
  List->Memory.AllocatedSpace = malloc(Size);
  List->Memory.Size = Size;
  List->SpaceRemaining = Size;
  List->Head = (list_chunk *)List->Memory.AllocatedSpace;
  List->Head->Next = NULL;
  List->Head->Size = Size;
}

// Note (sigmasleep): Assumes largest alignment is 8.
// Assumes no packing on header, therefore if header is aligned, the content is aligned.
internal void*
FindAndResizeFittingChunkFromList(list *List, unsigned int Size)
{
  assert(List->Head != NULL);

  unsigned int TotalSize = Size + sizeof(allocated_list_header);
  
  if (TotalSize > List->SpaceRemaining)
  {
    return NULL;
  }

  list_chunk *Chunk = List->Head;
  list_chunk *PreviousChunk = NULL;

  unsigned char AllocationAlignment = alignof(allocated_list_header);

  while (Chunk != NULL &&
    TotalSize + (size_t)Chunk % AllocationAlignment > Chunk->Size + Chunk->AlignmentOffset)
  {
    PreviousChunk = Chunk;
    Chunk = Chunk->Next;
  }

  if (Chunk == NULL)
  {
    return NULL;
  }

  unsigned char AllocationAlignmentOffset = (size_t)Chunk % AllocationAlignment;

  TotalSize += AllocationAlignmentOffset;
  
  if (TotalSize > List->SpaceRemaining)
  {
    return NULL;
  }

  int ChunkSize = Chunk->Size;

  allocated_list_header *Allocation = (allocated_list_header *)Chunk;
  Allocation->Size = ChunkSize;
  Allocation->AlignmentOffset = AllocationAlignmentOffset;

  unsigned char ChunkAlignmentOffset = ((size_t)Chunk + TotalSize) % alignof(list_chunk);

  List->SpaceRemaining -= TotalSize + AllocationAlignmentOffset;

  if (PreviousChunk == NULL)
  {
    // Note(sigmasleep): Head is fitting chunk
    List->Head = (list_chunk *)((size_t)Chunk + TotalSize + ChunkAlignmentOffset);
    List->Head->Size = ChunkSize - TotalSize - ChunkAlignmentOffset;
    List->Head->AlignmentOffset = ChunkAlignmentOffset;
  }
  else
  {
    PreviousChunk->Next = (list_chunk *)((size_t)Chunk + TotalSize + ChunkAlignmentOffset);
    PreviousChunk->Next->Size = ChunkSize - TotalSize - ChunkAlignmentOffset;
    PreviousChunk->Next->AlignmentOffset = ChunkAlignmentOffset;
  }

  return (void *)((size_t)Chunk + sizeof(allocated_list_header));
}

// Todo(sigmasleep): Add alignment
#define AllocateSpaceOnList(Stack, Type) AllocateSpaceOnList_(Stack, sizeof(Type))
internal void*
AllocateSpaceOnList_(list *List, unsigned int Size)
{
  return FindAndResizeFittingChunkFromList(List, Size);
}

#define DeallocateSpaceOnList(List, TypedPointer) DeallocateSpaceOnList_(List, (void *)TypedPointer, sizeof(*TypedPointer))
internal void
DeallocateSpaceOnList_(list *List, void* Address, size_t Size)
{
  allocated_list_header *Allocation = (allocated_list_header *)((size_t)Address - sizeof(allocated_list_header));

  list_chunk *Chunk = List->Head;
  list_chunk *PreviousChunk = NULL;
  while (Chunk != NULL && (size_t)Chunk < (size_t)Allocation)
  {
    PreviousChunk = Chunk;
    Chunk = Chunk->Next;
  }
  // Note(sigmasleep): Order of structs: PreviousChunk | Allocation | Chunk

 
  bool AllocationIsAdjacentToNextChunk = (size_t)Allocation + Size == (size_t)Chunk;
  bool AllocationIsAdjacentToPriorChunk = PreviousChunk != NULL &&
    (size_t)Allocation - PreviousChunk->Size == (size_t)PreviousChunk;

  unsigned int AllocationSize = Size + Allocation->AlignmentOffset;
  List->SpaceRemaining += AllocationSize;

  if (!(AllocationIsAdjacentToNextChunk && AllocationIsAdjacentToPriorChunk))
  {
    unsigned char ChunkAlignmentOffset = (size_t)Allocation % alignof(list_chunk);
    
    list_chunk *NewChunk = (list_chunk *)((size_t)Allocation + ChunkAlignmentOffset);
    NewChunk->Size = AllocationSize;
    NewChunk->AlignmentOffset = ChunkAlignmentOffset;
    NewChunk->Next = Chunk;

    if (PreviousChunk == NULL)
    {
      List->Head = NewChunk;
    }
    else
    {
      PreviousChunk->Next = NewChunk;
    }
  }
  else
  {
    if (AllocationIsAdjacentToNextChunk)
    {
      PreviousChunk->Next = (list_chunk *)Allocation;
      PreviousChunk->Next->Size = AllocationSize + Chunk->Size;
      PreviousChunk->Next->Next = Chunk->Next;

      // Note(sigmasleep): Sets up values for: if (AllocationIsAdjacentToPriorChunk)
      AllocationSize += Chunk->Size;
      Chunk = Chunk->Next;
    }
    if (AllocationIsAdjacentToPriorChunk)
    {
      PreviousChunk->Size += AllocationSize;
      PreviousChunk->Next = Chunk;
    }
  }
}