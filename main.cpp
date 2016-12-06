#include <stdio.h>
#include <assert.h>
#include <malloc.h>

#define internal static
#define global static
#define local_persist static

struct memory
{
  unsigned int Size;
  void *AllocatedSpace;
};

#include "stack_allocator.cpp"

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
};

struct allocated_list_header
{
  unsigned int Size;
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

// Todo (sigmasleep): Alignment
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

  while (Chunk != NULL && TotalSize > Chunk->Size)
  {
    PreviousChunk = Chunk;
    Chunk = Chunk->Next;
  }

  if (Chunk == NULL)
  {
    return NULL;
  }

  List->SpaceRemaining -= TotalSize;

  int ChunkSize = Chunk->Size;

  allocated_list_header *AllocatedHeader = (allocated_list_header *) Chunk;
  AllocatedHeader->Size = ChunkSize;

  if (PreviousChunk == NULL)
  {
    // Note(sigmasleep): Head is fitting chunk
    List->Head = (list_chunk *)((size_t)Chunk + TotalSize);
    List->Head->Size = ChunkSize - TotalSize;
  }
  else
  {
    PreviousChunk->Next = (list_chunk *)((size_t)Chunk + TotalSize);
    PreviousChunk->Next->Size = ChunkSize - TotalSize;
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

  List->SpaceRemaining += Size;

  bool AllocationIsAdjacentToNextChunk = (size_t)Allocation + Size == (size_t)Chunk;
  bool AllocationIsAdjacentToPriorChunk = PreviousChunk != NULL &&
    (size_t)Allocation - PreviousChunk->Size == (size_t)PreviousChunk;

  unsigned int AllocationSize = Size;

  if (!(AllocationIsAdjacentToNextChunk && AllocationIsAdjacentToPriorChunk))
  {
    list_chunk *NewChunk = (list_chunk *)Allocation;
    NewChunk->Size = AllocationSize;
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
      
      // Note(sigmasleep): Set up values for: if (AllocationIsAdjacentToPriorChunk)
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

int main()
{
  stack Stack;
  InitializeStack(&Stack, 30);
  int *Test = (int *)AllocateSpaceOnStack(&Stack, int);
  *Test = 4;
  printf("Hello\n");
  printf("%i\n", *Test);
  DeallocateSpaceOnStack(&Stack, int);

  list List;
  InitializeList(&List, 30);
  Test = (int *)AllocateSpaceOnList(&List, int);
  *Test = 4;
  printf("Hello\n");
  printf("%i\n", *Test);
  DeallocateSpaceOnList(&List, Test);

  return 0;
}