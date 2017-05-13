#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
//#ifdef WINDOWS
#include <windows.h>

//#endif
#define internal static
#define global static
#define local_persist static

struct memory
{
  size_t Size;
  void *AllocatedSpace;
};

#include "stack_allocator.cpp"
#include "list_allocator.cpp"

#include <time.h>
#include <limits.h>

#ifndef DEBUG
  #define IF_DEBUG(...)
#else
  #define IF_DEBUG(...) __VA_ARGS__
// Todo: Integrate with list_allocator and stabilize
internal bool
IsAllocationValid(void *Address, unsigned char Value)
{
	allocated_list_header_properties AllocatedHeaderProperties;
	GetAllocatedHeaderFromAllocatedChunk(&AllocatedHeaderProperties,
                                       (size_t)Address);

	return (AllocatedHeaderProperties.Type == 0 &&
          AllocatedHeaderProperties.ChunkSize == sizeof(char) + Value)
		|| (AllocatedHeaderProperties.Type == 1 &&
        AllocatedHeaderProperties.ChunkSize == sizeof(int) + Value)
		|| (AllocatedHeaderProperties.Type == 2 &&
        AllocatedHeaderProperties.ChunkSize == sizeof(long) + Value);
}
#endif

#ifndef VERBOSE
  #define IF_VERBOSE(...)
#else
  #define IF_VERBOSE(...) __VA_ARGS__
internal void
PrintAllocationInfo(void *Address)
{
	allocated_list_header_properties AllocatedHeaderProperties;
	GetAllocatedHeaderFromAllocatedChunk(&AllocatedHeaderProperties,
                                       (size_t)Address);
  
	printf("HeaderAddress: %zu\t", AllocatedHeaderProperties.Address);
  printf("ChunkSize: %zu\t", AllocatedHeaderProperties.ChunkSize);
	printf("PrePadding: %u\t", AllocatedHeaderProperties.PrePadding);
	printf("PostPadding: %u\n", AllocatedHeaderProperties.PostPadding);
}

internal void
PrintFreeList(list *List, size_t HighlightHeaderAddress)
{
  free_list_header *ListIterator = List->Head;

	size_t Size = 0;
	size_t ListLength = 0;

  printf("FreeList Status:\n  ");
	while(ListIterator != NULL)
	{
    if(HighlightHeaderAddress == (size_t)ListIterator)
    {
      printf("|%zu[%zu]| ", (size_t)ListIterator, ListIterator->ChunkSize);
    }
    else
    {
      printf("%zu[%zu] ", (size_t)ListIterator, ListIterator->ChunkSize);
    }

		Size += ListIterator->ChunkSize;
		ListIterator = ListIterator->Next;
		++ListLength;
	}
  printf("=> %zu\n", Size);
}
#endif
#define V_PRINTF(...) IF_VERBOSE(printf(__VA_ARGS__))

internal void 
DivideLargeIntegers(
  LARGE_INTEGER *X,
  LARGE_INTEGER *Y,
  LARGE_INTEGER *Quotient,
  int Scale)
{
  Quotient->QuadPart = X->QuadPart * Scale / Y->QuadPart;
}

global LARGE_INTEGER PrePerformanceCount, PostPerformanceCount;
global LARGE_INTEGER AllocationPerformanceCount, DeallocationPerformanceCount;
global int AllocationCount, DeallocationCount;

internal void
ComputeAndPrintAndResetMetrics()
{
  LARGE_INTEGER PerformanceCounterFrequency;
  QueryPerformanceFrequency(&PerformanceCounterFrequency);

  LARGE_INTEGER AllocationTimeInNS;
  DivideLargeIntegers(&AllocationPerformanceCount, &PerformanceCounterFrequency,
                      &AllocationTimeInNS,
                      1000 * 1000);
  LARGE_INTEGER DeallocationTimeInNS;
  DivideLargeIntegers(&DeallocationPerformanceCount,
                      &PerformanceCounterFrequency,
                      &DeallocationTimeInNS,
                      1000 * 1000);

  printf("# Of Allocations: %i\n", AllocationCount);
  printf("Total Allocation Time: %llins\n", AllocationTimeInNS.QuadPart);
  printf("\t=> ~%fns Per Allocation\n",
         AllocationTimeInNS.QuadPart / (long double)AllocationCount);
  printf("# Of Deallocations: %i\n", DeallocationCount);
  printf("Total Deallocation Time: %llins\n", DeallocationTimeInNS.QuadPart);
  printf("\t=> ~%fns Per Deallocation\n",
         DeallocationTimeInNS.QuadPart / (long double)DeallocationCount);

  V_PRINTF("QueryPerformanceFrequency: %lli\n",
         PerformanceCounterFrequency.QuadPart);

  AllocationPerformanceCount.QuadPart = DeallocationPerformanceCount.QuadPart
    = AllocationCount = DeallocationCount = 0; 
}

#define Metrics_AllocateSpaceOnStack(Stack, Type) \
        Metrics_AllocateSpaceOnStack_(Stack, sizeof(Type), alignof(Type))
internal void*
Metrics_AllocateSpaceOnStack_(stack *Stack, size_t Size, short Alignment)
{
  QueryPerformanceCounter(&PrePerformanceCount);
  void* Allocation = AllocateSpaceOnStack_(Stack, Size, Alignment);
  QueryPerformanceCounter(&PostPerformanceCount);

  if(Allocation != NULL)
  {
    AllocationPerformanceCount.QuadPart +=
      PostPerformanceCount.QuadPart - PrePerformanceCount.QuadPart;
    ++AllocationCount;
  }

  return Allocation;
}

#define Metrics_DeallocateSpaceOnStack(Stack, Type) \
        Metrics_DeallocateSpaceOnStack_(Stack, sizeof(Type))

internal void
Metrics_DeallocateSpaceOnStack_(stack *Stack, size_t Size)
{
  QueryPerformanceCounter(&PrePerformanceCount);
  DeallocateSpaceOnStack(Stack, Size);
  QueryPerformanceCounter(&PostPerformanceCount);
  DeallocationPerformanceCount.QuadPart +=
    PostPerformanceCount.QuadPart - PrePerformanceCount.QuadPart;
  ++DeallocationCount;
}

#define Metrics_AllocateSpaceOnList(Stack, Type) \
        Metrics_AllocateSpaceOnList_(Stack, sizeof(Type), alignof(Type))
internal void*
Metrics_AllocateSpaceOnList_(list *List, size_t Size, unsigned char Alignment)
{
  QueryPerformanceCounter(&PrePerformanceCount);
  void* Allocation = AllocateSpaceOnList_(List, Size, Alignment);
  QueryPerformanceCounter(&PostPerformanceCount);

  if(Allocation != NULL)
  {
    AllocationPerformanceCount.QuadPart +=
      PostPerformanceCount.QuadPart - PrePerformanceCount.QuadPart;
    ++AllocationCount;
  }

  return Allocation; 
}

internal void
Metrics_DeallocateSpaceOnList_(list *List, void* Address)
{
  QueryPerformanceCounter(&PrePerformanceCount);
  DeallocateSpaceOnList(List, Address);
  QueryPerformanceCounter(&PostPerformanceCount);
  DeallocationPerformanceCount.QuadPart +=
    PostPerformanceCount.QuadPart - PrePerformanceCount.QuadPart;
  ++DeallocationCount;
}

struct char_pile
{
  char* Pile;
  int Size;
};

#define boolint int
#define PILE_COUNT 100

internal void
StackTest(int AllocationSizeInChars, int TestCount)
{ 
  stack Stack;

  InitializeStack(&Stack, AllocationSizeInChars * sizeof(char));

  char_pile CharPiles[PILE_COUNT];

  V_PRINTF("Stack Allocation Starting\n");
  V_PRINTF("\t\t\tCurrent Top Address: %zu\t", (size_t)Stack.TopOfMemory);
  V_PRINTF("Space Remaining : %u\n", Stack.SpaceRemaining);

  int NextFreePileIndex = 0;
  for(int Iteration = 0; Iteration < TestCount; ++Iteration)
  {

    boolint Push = rand() % 2;
    if(Push && NextFreePileIndex < PILE_COUNT && Stack.SpaceRemaining)
    {
      int CharCount = (rand() % Stack.SpaceRemaining - 1) + 1;

      char_pile *TopPile = &CharPiles[NextFreePileIndex];

      TopPile->Pile = 
        (char *)Metrics_AllocateSpaceOnStack_(&Stack,
                                              sizeof(char) * CharCount,
                                              alignof(char)); 
      if(TopPile->Pile != NULL)
      {
        TopPile->Size = CharCount; 
        ++NextFreePileIndex;

        IF_VERBOSE(
          V_PRINTF("Allocated Char Pile: %d Chars\n", CharCount);
          V_PRINTF("  ");
          for(int CharIndex = 0; CharIndex < CharCount; ++CharIndex)
          {
            if(CharIndex % 10 == 9) 
              V_PRINTF("\n  ");

            V_PRINTF("%d\t", *(TopPile->Pile + CharIndex));
          }
          V_PRINTF("\n");
        )
      }  
      IF_VERBOSE(else
      {
        V_PRINTF("Allocation Failure: %d Chars\n", CharCount); 
        V_PRINTF("  Stack SpaceRemaining: %u Bytes\n", Stack.SpaceRemaining);
      })
    }
    else if(NextFreePileIndex > 0)
    {
      char_pile *TopPile = &CharPiles[NextFreePileIndex - 1]; 
      Metrics_DeallocateSpaceOnStack_(&Stack, TopPile->Size * sizeof(char));
      --NextFreePileIndex;

      //Note: Deallocation Needs Undefined Behavior Work
      IF_VERBOSE(
        V_PRINTF("Deallocation Successful: %d Chars\n", TopPile->Size);
        V_PRINTF("  ");
        for(int CharIndex = 0; CharIndex < TopPile->Size; ++CharIndex)
        {
          if(CharIndex % 10 == 9) 
            V_PRINTF("\n  ");

          V_PRINTF("%d\t", *(TopPile->Pile + CharIndex));
        }
        V_PRINTF("\n");
      )
    }

    IF_VERBOSE(
      V_PRINTF("Current Top Address: %zu\t", (size_t)Stack.TopOfMemory);
      V_PRINTF("Space Remaining : %u\t", Stack.SpaceRemaining);
      if(Stack.LastAlignmentIsHeader)
      {
        V_PRINTF("Last Alignment Is Header\n");
      }
      else
      {
        V_PRINTF("Last Alignment Is Footer\n");
      }
    )
  }

  printf("Stack Allocator Metrics:\n");
  ComputeAndPrintAndResetMetrics();
  printf("\n");
}

#define AllocationSize 10
internal void
ListTest(int AllocationSizeInChars, int TestCount)
{
	list List;
	InitializeList(&List, AllocationSizeInChars * sizeof(char));

  char_pile CharPiles[PILE_COUNT];

  V_PRINTF("List Allocation Starting\n");
	V_PRINTF("List Address Start: %zu\n", (size_t)List.Memory.AllocatedSpace);
  V_PRINTF("Space Remaining : %zu\n", List.SpaceRemaining);
  
  int NextFreePileIndex = 0;
	for(int Iteration = 0; Iteration < TestCount; ++Iteration)
	{
    IF_VERBOSE(size_t HighlightAddress = 0;)

    boolint Allocate = rand() % 2;
    if(Allocate && NextFreePileIndex < PILE_COUNT && List.SpaceRemaining)
    {
      int CharCount = (rand() % List.SpaceRemaining -  1) + 1;

      char_pile *TopPile = &CharPiles[NextFreePileIndex];

      TopPile->Pile =
        (char *)Metrics_AllocateSpaceOnList_(&List,
                                            sizeof(char) * CharCount,
                                            alignof(char));
      if(TopPile->Pile != NULL)
      {
        TopPile->Size = CharCount;
        ++NextFreePileIndex;

        IF_VERBOSE(
          HighlightAddress = (size_t)TopPile->Pile;
          V_PRINTF("Allocated Char Pile: %d Chars\n", CharCount);
          V_PRINTF("  ");
          for(int CharIndex = 0; CharIndex < CharCount; ++CharIndex)
          {
            if(CharIndex % 10 == 9) 
              V_PRINTF("\n  ");

            V_PRINTF("%d\t", *(TopPile->Pile + CharIndex));
          }
          V_PRINTF("\n");
        )
      }
      IF_VERBOSE(else
      {
        V_PRINTF("Allocation Failure: %d Chars\n", CharCount);
      })
    }
    else if(NextFreePileIndex > 0)
    {
      int RandomCharPileIndex = rand() % NextFreePileIndex;
      char_pile *RandomCharPile = &CharPiles[RandomCharPileIndex];
      Metrics_DeallocateSpaceOnList_(&List, RandomCharPile->Pile);
      --NextFreePileIndex;
      CharPiles[RandomCharPileIndex] = CharPiles[NextFreePileIndex];
      
      IF_VERBOSE(
        V_PRINTF("Deallocation Successful: %d Chars\n", RandomCharPile->Size);
        V_PRINTF("  ");
        for(int CharIndex = 0; CharIndex < RandomCharPile->Size; ++CharIndex)
        {
          if(CharIndex % 10 == 9) 
            V_PRINTF("\n  ");

          V_PRINTF("%d\t", *(RandomCharPile->Pile + CharIndex));
        }
        V_PRINTF("\n");
      )
    }

    IF_VERBOSE(PrintFreeList(&List, HighlightAddress));
  
  }

  printf("List Allocator Metrics:\n");
  ComputeAndPrintAndResetMetrics();
  printf("\n");
}

int
main(int argv, char** argc)
{
//  {
//    case 1:
//    {
srand((unsigned int)time(NULL));
StackTest(100, 1000);
ListTest(100, 1000);
//    } break;
//    case 2:
//    {
//      if (strcmp("-verbose", argc[1]) == 0 ||
//          strcmp("-v", argc[1]) == 0)
//      {
//        RunTestsVerbose();
//        break;
//      }
//    } // Note: Intentional pass
//    default: 
//    {
//      printf("Error: unknown option %s\n", argc[1]);
//      printf("Usage: main [option]\n"
//             "\n"
//             "The available options are as follows:\n"
//             "\n"
//             "\t-v , -verbose\tPrint memory allocation information.\n"
//             "\n");
//    }
//  }

  return 0;
}
