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
	GetAllocatedHeaderFromAllocatedChunk(&AllocatedHeaderProperties, (size_t)Address);
  
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
  AllocationPerformanceCount.QuadPart +=
    PostPerformanceCount.QuadPart - PrePerformanceCount.QuadPart;
  ++AllocationCount;
  return Allocation; 
}

internal void
Metrics_DeallocateSpaceOnList(list *List, void* Address)
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

internal void
StackTest(int AllocationSizeInChars, int TestCount)
{ 
  srand((unsigned int)time(NULL));
  stack Stack;

  InitializeStack(&Stack, AllocationSizeInChars * sizeof(char));

#define PILE_COUNT 100
  char_pile CharPiles[PILE_COUNT];

  V_PRINTF("Stack Allocation Starting\n");
  V_PRINTF("\t\t\tCurrent Top Address: %zu\t", (size_t)Stack.TopOfMemory);
  V_PRINTF("Space Remaining : %u\n", Stack.SpaceRemaining);

  int TopOfPileIndex = 0;
  for(int Iteration = 0; Iteration < TestCount; ++Iteration)
  {

    boolint Push = rand() % 2;
    if(Push && Stack.SpaceRemaining)
    {
      int CharCount = (rand() % Stack.SpaceRemaining - 1) + 1;
      if(CharCount == 0)
        break;

      char_pile *TopPile = &CharPiles[TopOfPileIndex];

      TopPile->Pile = 
        (char *)Metrics_AllocateSpaceOnStack_(&Stack,
                                              sizeof(char) * CharCount,
                                              alignof(char)); 
      if(TopPile->Pile != NULL)
      {
        TopPile->Size = CharCount; 
        ++TopOfPileIndex;

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
      IF_VERBOSE(
        else
        {
          V_PRINTF("Allocation Failure: %d Chars\n", CharCount); 
          V_PRINTF("  Stack SpaceRemaining: %u Bytes\n", Stack.SpaceRemaining);
        }
      )
    }
    else if(TopOfPileIndex > 0)
    {
      char_pile *TopPile = &CharPiles[TopOfPileIndex - 1]; 
      Metrics_DeallocateSpaceOnStack_(&Stack, TopPile->Size * sizeof(char));
      --TopOfPileIndex;

      //Note: Deallocation Needs Undefined Behavior Work
      V_PRINTF("Deallocation Successful: %d Chars\n", TopPile->Size);
      IF_VERBOSE(
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

#if 0
  V_PRINTF("Stack Deallocation Starting\n");
  V_PRINTF("\t\t\tCurrent Top Address: %zu\t", (size_t)Stack.TopOfMemory);
  V_PRINTF("Space Remaining : %u\t", Stack.SpaceRemaining);
  V_PRINTF(Stack.LastAlignmentIsHeader ? "Last Alignment Is Header\n"
           : "Last Alignment Is Footer\n");
  
  for(int TestIndex = AllocationSize - 1; TestIndex >= 0; --TestIndex)
  {
    switch(TestChoiceOrder[TestIndex])
    {
      case 0:
      {
        Metrics_DeallocateSpaceOnStack(&Stack, char);
        V_PRINTF("-Deallocated(char)\t");
      } break;
      case 1:
      {
        Metrics_DeallocateSpaceOnStack(&Stack, int);
        V_PRINTF("-Deallocated(int)\t");
      } break;
      case 2:
      {
        Metrics_DeallocateSpaceOnStack(&Stack, long);
        V_PRINTF("-Deallocated(long)\t");
      } break;
    }

    V_PRINTF("Current Top Address: %zu\t", (size_t)Stack.TopOfMemory);
    V_PRINTF("Space Remaining : %u\t", Stack.SpaceRemaining);
    V_PRINTF(Stack.LastAlignmentIsHeader ? "Last Alignment Is Header\n"
             : "Last Alignment Is Footer\n");
  }  
#endif

  printf("Stack Allocator Metrics:\n");
  ComputeAndPrintAndResetMetrics();
  printf("\n");
}

#define AllocationSize 10
internal void
ListTest()
{
	list List;
	InitializeList(&List, AllocationSize * (sizeof(long) + alignof(long)) * 2);

	V_PRINTF("Total Space Allocated: %zubytes\n", List.Memory.Size);

	unsigned char *CharArray[AllocationSize];
	int CharArraySize = 0;

	unsigned int *IntArray[AllocationSize];
	int IntArraySize = 0;

	unsigned long *LongArray[AllocationSize];
	int LongArraySize = 0;

  V_PRINTF("List Allocation Starting\n");
	V_PRINTF("List Address Start: %zu\n", (size_t)List.Memory.AllocatedSpace);

	for(int TestIndex = 0; TestIndex < AllocationSize * 2; ++TestIndex)
	{
		int TestChoice = rand() % 3;
		bool Allocate = rand() % 2 == 1;
    IF_VERBOSE(size_t HighlightAddress = 0;)

		switch(TestChoice)
		{
			case 0:
			{
				if(Allocate || CharArraySize == 0)
				{
					CharArray[CharArraySize] =
            (unsigned char *)Metrics_AllocateSpaceOnList(&List, char);
					if(CharArray[CharArraySize] == NULL) {
						V_PRINTF("Cannot Allocate Char! Memory Full!\n");
						break;
					}
          IF_VERBOSE(
            V_PRINTF("Char Allocated: %u\tAddress: %zu\n  ",
                     *CharArray[CharArraySize],
                     (size_t)CharArray[CharArraySize]);
            PrintAllocationInfo(CharArray[CharArraySize]);
            HighlightAddress = (size_t)CharArray[CharArraySize];
          )

					++CharArraySize;

				}
				else
				{
					--CharArraySize;
					V_PRINTF("Char Deallocated: [%u]:%u\tAddress: %zu\n",
                   CharArraySize,
                   *CharArray[CharArraySize],
                   (size_t)CharArray[CharArraySize]);
					Metrics_DeallocateSpaceOnList(&List, CharArray[CharArraySize]);
				}
			} break;
			case 1:
			{
				if(Allocate || IntArraySize == 0)
				{
					IntArray[IntArraySize] =
            (unsigned int *)Metrics_AllocateSpaceOnList(&List, int);
					if(IntArray[IntArraySize] == NULL) {
						V_PRINTF("Cannot Allocate Int! Memory Full!\n");
						break;
					}
          IF_VERBOSE(
            V_PRINTF("Int Allocated: %u\tAddress: %zu\n  ",
                     *IntArray[IntArraySize],
                     (size_t)IntArray[IntArraySize]);
            PrintAllocationInfo(IntArray[IntArraySize]);
            HighlightAddress = (size_t)(IntArray[IntArraySize]);
          )
					++IntArraySize;
				}
				else
				{
					--IntArraySize;
					V_PRINTF("Int Deallocated: [%u]:%u\tAddress: %zu\n",
                   IntArraySize,
                   *IntArray[IntArraySize],
                   (size_t)IntArray[IntArraySize]);
					Metrics_DeallocateSpaceOnList(&List, IntArray[IntArraySize]);
				}
			} break;
			case 2:
			{
				if(Allocate || LongArraySize == 0)
				{
					LongArray[LongArraySize] =
            (unsigned long *)Metrics_AllocateSpaceOnList(&List, long);
					if(LongArray[LongArraySize] == NULL) {
						V_PRINTF("Cannot Allocate Long! Memory Full!\n");
						break;
					}
          IF_VERBOSE(
            V_PRINTF("Long Allocated: %u\tAddress: %zu\n  ",
                     *LongArray[LongArraySize],
                     (size_t)LongArray[LongArraySize]);
            PrintAllocationInfo(LongArray[LongArraySize]);
            HighlightAddress = (size_t)(LongArray[LongArraySize]);
          )
            
					++LongArraySize;
				}
				else
				{
					--LongArraySize;
					V_PRINTF("Long Deallocated: [%u]:%u\tAddress: %zu\n",
                   LongArraySize,
                   *LongArray[LongArraySize],
                   (size_t)LongArray[LongArraySize]);
					Metrics_DeallocateSpaceOnList(&List, LongArray[LongArraySize]);
				}
			} break;
		}

		IF_VERBOSE(PrintFreeList(&List, HighlightAddress));
    
    IF_DEBUG(
      for(int CharArrayIndex = 0;
          CharArrayIndex < CharArraySize;
          ++CharArrayIndex)
      {
        allocated_list_header_properties AllocatedHeaderProperties;
        GetAllocatedHeaderFromAllocatedChunk(&AllocatedHeaderProperties,
                                             (size_t)CharArray[CharArrayIndex]);
        V_PRINTF("C[%u](%zu,%u->%zu->%u,%u):%u\t",
                 CharArrayIndex,
                 (size_t)CharArray[CharArrayIndex],
                 AllocatedHeaderProperties.PrePadding,
                 AllocatedHeaderProperties.ChunkSize,
                 AllocatedHeaderProperties.PostPadding,
                 AllocatedHeaderProperties.Type,
                 *CharArray[CharArrayIndex]);
        if(!IsAllocationValid(CharArray[CharArrayIndex],
                              *CharArray[CharArrayIndex]))
        {
          V_PRINTF("BORKED\t");
        }
        IF_VERBOSE(PrintFreeList(&List, 0));
      }
      for(int IntArrayIndex = 0; IntArrayIndex < IntArraySize; ++IntArrayIndex)
      {
        allocated_list_header_properties AllocatedHeaderProperties;
        GetAllocatedHeaderFromAllocatedChunk(&AllocatedHeaderProperties,
                                             (size_t)IntArray[IntArrayIndex]);
        V_PRINTF("I[%u](%zu,%u->%zu->%u,%u):%u\t",
                 IntArrayIndex,
                 (size_t)IntArray[IntArrayIndex],
                 AllocatedHeaderProperties.PrePadding,
                 AllocatedHeaderProperties.ChunkSize,
                 AllocatedHeaderProperties.PostPadding,
                 AllocatedHeaderProperties.Type,
                 *IntArray[IntArrayIndex]);
        if(!IsAllocationValid(IntArray[IntArrayIndex],
                              (unsigned char)*IntArray[IntArrayIndex]))
        {
          V_PRINTF("BORKED\t");
        }
        IF_VERBOSE(PrintFreeList(&List, 0));
      }
      for(int LongArrayIndex = 0; LongArrayIndex < LongArraySize; ++LongArrayIndex)
      {
        allocated_list_header_properties AllocatedHeaderProperties;
        GetAllocatedHeaderFromAllocatedChunk(&AllocatedHeaderProperties,
                                             (size_t)LongArray[LongArrayIndex]);
        V_PRINTF("L[%u](%zu,%u->%zu->%u,%u):%u\t",
                 LongArrayIndex,
                 (size_t)LongArray[LongArrayIndex],
                 AllocatedHeaderProperties.PrePadding,
                 AllocatedHeaderProperties.ChunkSize,
                 AllocatedHeaderProperties.PostPadding,
                 AllocatedHeaderProperties.Type,
                 *LongArray[LongArrayIndex]);
        if(!IsAllocationValid(LongArray[LongArrayIndex],
                              (unsigned char)*LongArray[LongArrayIndex]))
        {
          V_PRINTF("BORKED\t");
        }
        IF_VERBOSE(PrintFreeList(&List, 0));
      }
    )
    V_PRINTF("Space Remaining: %zu\n", List.SpaceRemaining);
  }
  

	V_PRINTF("\nTotal Space Reserved: %zu\n", List.Memory.Size);
	V_PRINTF("Space Remaining After Random Testing: %zu\n\n", List.SpaceRemaining);

	V_PRINTF("\nChars:\n");
	for(int CharArrayIndex = 0; CharArrayIndex < CharArraySize; ++CharArrayIndex)
	{
		V_PRINTF("[%i]:%u\tChar Deallocated\n", CharArrayIndex, *CharArray[CharArrayIndex]);
		Metrics_DeallocateSpaceOnList(&List, CharArray[CharArrayIndex]);
		IF_VERBOSE(PrintFreeList(&List, 0));
		V_PRINTF("Space Remaining: %zu\n", List.SpaceRemaining);
	}
	V_PRINTF("\n\nInts:\n");
	for(int IntArrayIndex = 0; IntArrayIndex < IntArraySize; ++IntArrayIndex)
	{
		V_PRINTF("[%i]:%u\tInt Deallocated\n", IntArrayIndex, *IntArray[IntArrayIndex]);
		Metrics_DeallocateSpaceOnList(&List, IntArray[IntArrayIndex]);
		IF_VERBOSE(PrintFreeList(&List, 0));
		V_PRINTF("Space Remaining: %zu\n", List.SpaceRemaining);
	}
	V_PRINTF("\n\nLongs:\n");
	for(int LongArrayIndex = 0; LongArrayIndex < LongArraySize; ++LongArrayIndex)
	{
		V_PRINTF("[%i]:%u\tLong Deallocated\n", LongArrayIndex, *LongArray[LongArrayIndex]);
		Metrics_DeallocateSpaceOnList(&List, LongArray[LongArrayIndex]);
		IF_VERBOSE(PrintFreeList(&List, 0));
		V_PRINTF("Space Remaining: %zu\n", List.SpaceRemaining);
	}

	V_PRINTF("\nTotal Space Reserved: %zu\n", List.Memory.Size);
	V_PRINTF("Space Remaining After Full Deallocating: %zu\n\n", List.SpaceRemaining);

  IF_VERBOSE(PrintFreeList(&List, 0));

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
StackTest(1000, 1000);
//ListTest();
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
