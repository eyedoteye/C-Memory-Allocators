struct stack
{
  memory Memory;
  void* TopOfMemory;
  size_t SpaceRemaining;
  bool LastAlignmentIsHeader;
};

// Note(sigmasleep): Alignments default to being a footer.
struct alignment
{
  unsigned char Offset : 4;
  unsigned char LastAlignmentIsHeader : 1;
  unsigned char Extra : 3; // Note(sigmasleep): Maybe I can pack something cool in here
};

internal void
InitializeStack(stack *Stack, size_t Size)
{
  Stack->Memory.Size = Size;
  Stack->Memory.AllocatedSpace = malloc(Size);
  Stack->SpaceRemaining = Size;
  Stack->TopOfMemory = Stack->Memory.AllocatedSpace;
}

internal inline void
_ComputeStackRemainingSpace(stack *Stack)
{
  Stack->SpaceRemaining = (size_t)Stack->Memory.AllocatedSpace + Stack->Memory.Size - (size_t)Stack->TopOfMemory;
}

#define AllocateSpaceOnStack(Stack, Type) AllocateSpaceOnStack_(Stack, sizeof(Type), alignof(Type))
// Returns NULL if no space is available.
internal void*
AllocateSpaceOnStack_(stack *Stack, size_t Size, short Alignment)
{
  short AlignmentOffset = (size_t)Stack->TopOfMemory % Alignment;

  void* Allocation = (void*)((size_t)Stack->TopOfMemory + AlignmentOffset);

  if (Stack->SpaceRemaining < Size + Alignment)
  {
    return NULL;
  }

  if (AlignmentOffset < sizeof(alignment))
  {
    // Note(sigmasleep): This means can't fit alignment struct inbetween last (allocation+alignment struct) and this allocation.  
    alignment *NewTopOfAlignment = (alignment *)((size_t)Allocation + Size);

    NewTopOfAlignment->Offset = AlignmentOffset;
    NewTopOfAlignment->LastAlignmentIsHeader = Stack->LastAlignmentIsHeader;

    Stack->LastAlignmentIsHeader = false;

    Stack->TopOfMemory = (void*)((size_t)NewTopOfAlignment + sizeof(alignment));
  }
  else
  {
    alignment *NewTopOfAlignment = (alignment *)((size_t)Allocation - sizeof(alignment));

    NewTopOfAlignment->Offset = AlignmentOffset;
    NewTopOfAlignment->LastAlignmentIsHeader = Stack->LastAlignmentIsHeader;

    Stack->LastAlignmentIsHeader = true;

    Stack->TopOfMemory = (void*)((size_t)Allocation + Size);
  }

  _ComputeStackRemainingSpace(Stack);

  return Allocation;
}

// Note(sigmasleep): Stack must be deallocated in order.
#define DeallocateSpaceOnStack(Stack, Type) DeallocateSpaceOnStack_(Stack, sizeof(Type))
internal void
DeallocateSpaceOnStack_(stack *Stack, size_t Size)
{
  if (Stack->LastAlignmentIsHeader)
  {
    alignment *Alignment = (alignment *)((size_t)Stack->TopOfMemory - Size - sizeof(alignment));

    Stack->TopOfMemory = (void *)((size_t)Alignment - Alignment->Offset + sizeof(alignment));

    Stack->LastAlignmentIsHeader = Alignment->LastAlignmentIsHeader;
  }
  else
  {
    alignment *Alignment = (alignment *)((size_t)Stack->TopOfMemory - sizeof(alignment));

    Stack->TopOfMemory = (void *)((size_t)Alignment - Size - Alignment->Offset);

    Stack->LastAlignmentIsHeader = Alignment->LastAlignmentIsHeader;
  }

  _ComputeStackRemainingSpace(Stack);
}