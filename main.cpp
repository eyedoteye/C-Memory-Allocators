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

#ifdef VERBOSE
  #define V_PRINTF(...) printf(...)
#else
  #define V_PRINTF(...)  
#endif

#ifdef VERBOSE
  #define V_PRINT_ALLOCATION_INFO(...) PrintAllocationInfo(...)
#else
  #define V_PRINT_ALLOCATION_INFO(...)  
#endif
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

#ifdef VERBOSE
  #define V_PRINT_FREE_LIST(...) PrintFreeList(...)
#else
  #define V_PRINT_FREE_LIST(...)
#endif
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

// only works with debug stuff 
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

internal void
SubtractLargeIntegers(
  LARGE_INTEGER *X,
  LARGE_INTEGER *Y,
  LARGE_INTEGER *Difference)
{
  Difference->QuadPart = X->QuadPart - Y->QuadPart;
}

internal void 
DivideLargeIntegers(
  LARGE_INTEGER *X,
  LARGE_INTEGER *Y,
  LARGE_INTEGER *Quotient,
  int Scale)
{
  Quotient->QuadPart = X->QuadPart * Scale / Y->QuadPart;
}

#define TestCount 100
internal void
ListTest()
{
	unsigned char *CharArray[TestCount];
	int CharArraySize = 0;
	unsigned int *IntArray[TestCount];
	int IntArraySize = 0;
	unsigned long *LongArray[TestCount];
	int LongArraySize = 0;

	list List;
	InitializeList(&List, TestCount * (sizeof(long) + alignof(long)) * 2);
	printf("Total Space Allocated: %zubytes\n", List.Memory.Size);
	V_PRINTF("List Address Start: %zu\n",
         (size_t)List.Memory.AllocatedSpace);
	V_PRINTF("List Address End: %zu\n",
         (size_t)List.Memory.AllocatedSpace + List.Memory.Size);
   
	for(int TestIndex = 0; TestIndex < TestCount * 2; ++TestIndex)
	{
		int TestChoice = rand() % 3;
		bool Allocate = rand() % 2 == 1;
    size_t HighlightAddress;

		switch(TestChoice)
		{
			case 0:
			{
				if(Allocate || CharArraySize == 0)
				{
					CharArray[CharArraySize] =
            (unsigned char *)AllocateSpaceOnList(&List, char);
					if(CharArray[CharArraySize] == NULL) {
						V_PRINTF("Cannot Allocate Char! Memory Full!\n");
						break;
					}
					V_PRINTF("Char Allocated: %u\tAddress: %zu\n  ",
                 *CharArray[CharArraySize], (size_t)CharArray[CharArraySize]);
					V_PRINT_ALLOCATION_INFO(CharArray[CharArraySize]);
					++CharArraySize;
				}
				else
				{
					--CharArraySize;
					V_PRINTF("Char Deallocated: [%u]:%u\tAddress: %zu\n",
                 CharArraySize,
                 *CharArray[CharArraySize],
                 (size_t)CharArray[CharArraySize]);
					DeallocateSpaceOnList(&List, CharArray[CharArraySize]);
				}
			} break;
			case 1:
			{
				if(Allocate || IntArraySize == 0)
				{
					IntArray[IntArraySize] =
            (unsigned int *)AllocateSpaceOnList(&List, int);
					if(IntArray[IntArraySize] == NULL) {
						V_PRINTF("Cannot Allocate Int! Memory Full!\n");
						break;
					}
					V_PRINTF("Int Allocated: %u\tAddress: %zu\n  ",
                 *IntArray[IntArraySize],
                 (size_t)IntArray[IntArraySize]);
					V_PRINT_ALLOCATION_INFO(IntArray[IntArraySize]);
					++IntArraySize;
				}
				else
				{
					--IntArraySize;
					V_PRINTF("Int Deallocated: [%u]:%u\tAddress: %zu\n",
                 IntArraySize,
                 *IntArray[IntArraySize],
                 (size_t)IntArray[IntArraySize]);
					DeallocateSpaceOnList(&List, IntArray[IntArraySize]);
				}
			} break;
			case 2:
			{
				if(Allocate || LongArraySize == 0)
				{
					LongArray[LongArraySize] =
            (unsigned long *)AllocateSpaceOnList(&List, long);
					if(LongArray[LongArraySize] == NULL) {
						V_PRINTF("Cannot Allocate Long! Memory Full!\n");
						break;
					}
					V_PRINTF("Long Allocated: %u\tAddress: %zu\n  ",
                 *LongArray[LongArraySize],
                 (size_t)LongArray[LongArraySize]);
					V_PRINT_ALLOCATION_INFO(LongArray[LongArraySize]);
					++LongArraySize;
				}
				else
				{
					--LongArraySize;
					V_PRINTF("Long Deallocated: [%u]:%u\tAddress: %zu\n",
                 LongArraySize,
                 *LongArray[LongArraySize],
                 (size_t)LongArray[LongArraySize]);
					DeallocateSpaceOnList(&List, LongArray[LongArraySize]);
				}
			} break;
		}

		V_PRINT_FREE_LIST(&List, 0);
    
		for(int CharArrayIndex = 0; CharArrayIndex < CharArraySize; ++CharArrayIndex)
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
                            *CharArray[CharArrayIndex])) {
        {
          V_PRINTF("BORKED\t");
        }
			}
      V_PRINT_FREE_LIST(&List);
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
                            (unsigned char)*IntArray[IntArrayIndex])) {
        {
          V_PRINTF("BORKED\t");
        }
			}
      V_PRINT_FREE_LIST(&List);
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
                            (unsigned char)*LongArray[LongArrayIndex])) {
        {
          V_PRINTF("BORKED\t");
        }
			}
      V_PRINT_FREE_LIST(&List);
		}
		V_PRINTF("Space Remaining: %zu\n", List.SpaceRemaining);
	}

	V_PRINTF("\nTotal Space Reserved: %zu\n", List.Memory.Size);
	V_PRINTF("Space Remaining After Random Testing: %zu\n\n", List.SpaceRemaining);

	V_PRINTF("\nChars:\n");
	for(int CharArrayIndex = 0; CharArrayIndex < CharArraySize; ++CharArrayIndex)
	{
		V_PRINTF("[%i]:%u\tChar Deallocated\n", CharArrayIndex, *CharArray[CharArrayIndex]);
		DeallocateSpaceOnList(&List, CharArray[CharArrayIndex]);
		V_PRINT_FREE_LIST(&List);
		V_PRINTF("Space Remaining: %zu\n", List.SpaceRemaining);
	}
	V_PRINTF("\n\nInts:\n");
	for(int IntArrayIndex = 0; IntArrayIndex < IntArraySize; ++IntArrayIndex)
	{
		V_PRINTF("[%i]:%u\tInt Deallocated\n", IntArrayIndex, *IntArray[IntArrayIndex]);
		DeallocateSpaceOnList(&List, IntArray[IntArrayIndex]);
		V_PRINT_FREE_LIST(&List);
		V_PRINTF("Space Remaining: %zu\n", List.SpaceRemaining);
	}
	V_PRINTF("\n\nLongs:\n");
	for(int LongArrayIndex = 0; LongArrayIndex < LongArraySize; ++LongArrayIndex)
	{
		V_PRINTF("[%i]:%u\tLong Deallocated\n", LongArrayIndex, *LongArray[LongArrayIndex]);
		DeallocateSpaceOnList(&List, LongArray[LongArrayIndex]);
		V_PRINT_FREE_LIST(&List);
		V_PRINTF("Space Remaining: %zu\n", List.SpaceRemaining);
	}

	V_PRINTF("\nTotal Space Reserved: %zu\n", List.Memory.Size);
	V_PRINTF("Space Remaining After Full Deallocating: %zu\n\n", List.SpaceRemaining);

	V_PRINT_FREE_LIST(&List);
}

internal void
RunTests()
{
  LARGE_INTEGER PerformanceCounterFrequency;
  LARGE_INTEGER OldTime;
  LARGE_INTEGER NewTime;

  QueryPerformanceFrequency(&PerformanceCounterFrequency);

  srand((unsigned int)time(NULL));
  stack Stack;

  QueryPerformanceCounter(&OldTime);
  InitializeStack(&Stack, TestCount * (sizeof(long)+alignof(long)));
  QueryPerformanceCounter(&NewTime);

	printf("Total Space Allocated: %zubytes\n", Stack.Memory.Size);
	V_PRINTF("List Address Start: %zu\n",
         (size_t)Stack.Memory.AllocatedSpace);
	V_PRINTF("Stack Address End: %zu\n",
         (size_t)Stack.Memory.AllocatedSpace + Stack.Memory.Size);

  LARGE_INTEGER StackInitializationTime;
  SubtractLargeIntegers(&NewTime, &OldTime, &StackInitializationTime);

  char *CharArray[TestCount];
  int CharArraySize = 0;
  
  int *IntArray[TestCount];
  int IntArraySize = 0;

  long *LongArray[TestCount];
  int LongArraySize = 0;

  int TestChoiceOrder[TestCount];

  QueryPerformanceCounter(&OldTime);

  for(int TestIndex = 0; TestIndex < TestCount; ++TestIndex)
  {
    int TestChoice = rand() % 3;
    TestChoiceOrder[TestIndex] = TestChoice;
    switch(TestChoice)
    {
      case 0:
      {
        CharArray[CharArraySize] = (char *)AllocateSpaceOnStack(&Stack, char);
        *CharArray[CharArraySize] = (char)(CharArraySize);
        ++CharArraySize;
      } break;
      case 1:
      {
        IntArray[IntArraySize] = (int *)AllocateSpaceOnStack(&Stack, int);
        *IntArray[IntArraySize] = (int)(IntArraySize);
        ++IntArraySize;
      } break;
      case 2:
      {
        LongArray[LongArraySize] = (long *)AllocateSpaceOnStack(&Stack, long);
        *LongArray[LongArraySize] = (long)(LongArraySize);
        ++LongArraySize;
      } break;
    }
  }

  QueryPerformanceCounter(&NewTime);
  LARGE_INTEGER StackAllocationTime;
  SubtractLargeIntegers(&NewTime, &OldTime, &StackAllocationTime);

  QueryPerformanceCounter(&OldTime);

  for(int TestIndex = TestCount - 1; TestIndex >= 0; --TestIndex)
  {
    switch(TestChoiceOrder[TestIndex])
    {
      case 0:
      {
        DeallocateSpaceOnStack(&Stack, char);
      } break;
      case 1:
      {
        DeallocateSpaceOnStack(&Stack, int);
      } break;
      case 2:
      {
        DeallocateSpaceOnStack(&Stack, long);
      } break;
    }
  }  

  QueryPerformanceCounter(&NewTime);
  LARGE_INTEGER StackDeallocationTime;
  SubtractLargeIntegers(&NewTime, &OldTime, &StackDeallocationTime);

  LARGE_INTEGER StackInitializationTimeInNS;
  DivideLargeIntegers(&StackInitializationTime, &PerformanceCounterFrequency,
                      &StackInitializationTimeInNS,
                      1000 * 1000);
  LARGE_INTEGER StackAllocationTimeInNS;
  DivideLargeIntegers(&StackAllocationTime, &PerformanceCounterFrequency,
                      &StackAllocationTimeInNS,
                      1000 * 1000);
  LARGE_INTEGER StackDeallocationTimeInNS;
  DivideLargeIntegers(&StackDeallocationTime, &PerformanceCounterFrequency,
                      &StackDeallocationTimeInNS,
                      1000 * 1000);

  printf("Stack Initialization Time: %llins\n",
         StackInitializationTimeInNS.QuadPart); 
  printf("Stack Allocation Time: %llins\n",
         StackAllocationTimeInNS.QuadPart);
  printf("Stack Deallocation Time: %llins\n",
         StackDeallocationTimeInNS.QuadPart);
  printf("\n");

  ListTest();
}

// Note: I can't think of a better way to guarantee performance of the
// non-verbose mode without creating another method.
internal void
RunTestsVerbose()
{
  LARGE_INTEGER PerformanceCounterFrequency;
  LARGE_INTEGER OldTime;
  LARGE_INTEGER NewTime;

  QueryPerformanceFrequency(&PerformanceCounterFrequency);

  srand((unsigned int)time(NULL));
  stack Stack;

  QueryPerformanceCounter(&OldTime);
  InitializeStack(&Stack, TestCount * (sizeof(long)+alignof(long)));
  QueryPerformanceCounter(&NewTime);

  LARGE_INTEGER StackInitializationTime;
  SubtractLargeIntegers(&NewTime, &OldTime, &StackInitializationTime);

  char *CharArray[TestCount];
  int CharArraySize = 0;
  
  int *IntArray[TestCount];
  int IntArraySize = 0;

  long *LongArray[TestCount];
  int LongArraySize = 0;

  int TestChoiceOrder[TestCount];

  printf("Stack Allocation Starting\n");
  V_PRINTF("\t\t\tCurrent Top Address: %zu\t", (size_t)Stack.TopOfMemory);
  V_PRINTF("Space Remaining : %u\n", Stack.SpaceRemaining);

  QueryPerformanceCounter(&OldTime);

  for(int TestIndex = 0; TestIndex < TestCount; ++TestIndex)
  {
    int TestChoice = rand() % 3;
    TestChoiceOrder[TestIndex] = TestChoice;
    switch(TestChoice)
    {
      case 0:
      {
        CharArray[CharArraySize] = (char *)AllocateSpaceOnStack(&Stack, char);
        *CharArray[CharArraySize] = (char)(CharArraySize);
        ++CharArraySize;
        V_PRINTF("-Allocated(char) \t");
      } break;
      case 1:
      {
        IntArray[IntArraySize] = (int *)AllocateSpaceOnStack(&Stack, int);
        *IntArray[IntArraySize] = (int)(IntArraySize);
        ++IntArraySize;
        V_PRINTF("-Allocated(int) \t");
      } break;
      case 2:
      {
        LongArray[LongArraySize] = (long *)AllocateSpaceOnStack(&Stack, long);
        *LongArray[LongArraySize] = (long)(LongArraySize);
        ++LongArraySize;
        V_PRINTF("-Allocated(long) \t");
      } break;
    }
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
  }

  QueryPerformanceCounter(&NewTime);
  LARGE_INTEGER StackAllocationTime;
  SubtractLargeIntegers(&NewTime, &OldTime, &StackAllocationTime);

  V_PRINTF("\nChars:\n");
  for(int CharArrayIndex = 0; CharArrayIndex < CharArraySize; ++CharArrayIndex)
  {
    V_PRINTF("[%i]:%d\t", CharArrayIndex, *CharArray[CharArrayIndex]);
  }
  V_PRINTF("\n\nInts:\n");
  for(int IntArrayIndex = 0; IntArrayIndex < IntArraySize; ++IntArrayIndex)
  {
    V_PRINTF("[%i]:%d\t", IntArrayIndex, *IntArray[IntArrayIndex]);
  }
  V_PRINTF("\n\nLongs:\n");
  for(int LongArrayIndex = 0; LongArrayIndex < LongArraySize; ++LongArrayIndex)
  {
    V_PRINTF("[%i]:%d\t", LongArrayIndex, *LongArray[LongArrayIndex]);
  }

  V_PRINTF("\n\n");
  V_PRINTF("Stack Deallocation Starting\n");
  V_PRINTF("\t\t\tCurrent Top Address: %zu\t", (size_t)Stack.TopOfMemory);
  V_PRINTF("Space Remaining : %u\t", Stack.SpaceRemaining);
  if(Stack.LastAlignmentIsHeader)
  {
    V_PRINTF("Last Alignment Is Header\n");
  }
  else
  {
    V_PRINTF("Last Alignment Is Footer\n");
  }

  QueryPerformanceCounter(&OldTime);

  for(int TestIndex = TestCount - 1; TestIndex >= 0; --TestIndex)
  {
    switch(TestChoiceOrder[TestIndex])
    {
      case 0:
      {
        DeallocateSpaceOnStack(&Stack, char);
        V_PRINTF("-Deallocated(char)\t");
      } break;
      case 1:
      {
        DeallocateSpaceOnStack(&Stack, int);
        V_PRINTF("-Deallocated(int)\t");
      } break;
      case 2:
      {
        DeallocateSpaceOnStack(&Stack, long);
        V_PRINTF("-Deallocated(long)\t");
      } break;
    }
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
  }  

  QueryPerformanceCounter(&NewTime);
  LARGE_INTEGER StackDeallocationTime;
  SubtractLargeIntegers(&NewTime, &OldTime, &StackDeallocationTime);

  LARGE_INTEGER StackInitializationTimeInNS;
  DivideLargeIntegers(&StackInitializationTime, &PerformanceCounterFrequency,
                      &StackInitializationTimeInNS,
                      1000 * 1000);
  LARGE_INTEGER StackAllocationTimeInNS;
  DivideLargeIntegers(&StackAllocationTime, &PerformanceCounterFrequency,
                      &StackAllocationTimeInNS,
                      1000 * 1000);
  LARGE_INTEGER StackDeallocationTimeInNS;
  DivideLargeIntegers(&StackDeallocationTime, &PerformanceCounterFrequency,
                      &StackDeallocationTimeInNS,
                      1000 * 1000);

  printf("Stack Initialization Time: %llins\n",
         StackInitializationTimeInNS.QuadPart); 
  printf("Stack Allocation Time: %llins\n",
         StackAllocationTimeInNS.QuadPart);
  printf("Stack Deallocation Time: %llins\n",
         StackDeallocationTimeInNS.QuadPart);
  printf("QueryPerformanceFrequency: %lli\n",
         PerformanceCounterFrequency.QuadPart);
  printf("\n");

	ListTest();
}

int
main(int argv, char** argc)
{
//  {
//    case 1:
//    {
      RunTests();
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
