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

int
main()
{
  LARGE_INTEGER PerformanceCounterFrequency;
  LARGE_INTEGER OldTime;
  LARGE_INTEGER NewTime;

  QueryPerformanceFrequency(&PerformanceCounterFrequency);

  srand((unsigned int)time(NULL));
#define TestCount 200
  stack Stack;

  QueryPerformanceCounter(&OldTime);
  InitializeStack(&Stack, TestCount * (sizeof(long)+alignof(long)));
  QueryPerformanceCounter(&NewTime);

  LARGE_INTEGER StackInitializationTime;
  SubtractLargeIntegers(&NewTime, &OldTime, &StackInitializationTime);

  unsigned char *CharArray[TestCount];
  int CharArraySize = 0;
  unsigned int *IntArray[TestCount];
  int IntArraySize = 0;
  unsigned long *LongArray[TestCount];
  int LongArraySize = 0;

  int TestChoiceOrder[TestCount];

  printf("Stack Allocation Starting\n");
  printf("\t\t\tCurrent Top Address: %zu\t", (size_t)Stack.TopOfMemory);
  printf("Space Remaining : %zu\n", Stack.SpaceRemaining);

  QueryPerformanceCounter(&OldTime);

  for(int TestIndex = 0; TestIndex < TestCount; ++TestIndex)
  {
    int TestChoice = rand() % 3;
    TestChoiceOrder[TestIndex] = TestChoice;
    switch(TestChoice)
    {
      case 0:
      {
        CharArray[CharArraySize] = (unsigned char *)AllocateSpaceOnStack(&Stack, char);
        *CharArray[CharArraySize] = (unsigned char)(CharArraySize);
        ++CharArraySize;
        printf("-Allocated(char) \t");
      } break;
      case 1:
      {
        IntArray[IntArraySize] = (unsigned int *)AllocateSpaceOnStack(&Stack, int);
        *IntArray[IntArraySize] = (unsigned int)(IntArraySize);
        ++IntArraySize;
        printf("-Allocated(int) \t");
      } break;
      case 2:
      {
        LongArray[LongArraySize] = (unsigned long *)AllocateSpaceOnStack(&Stack, long);
        *LongArray[LongArraySize] = (unsigned long)(LongArraySize);
        ++LongArraySize;
        printf("-Allocated(long) \t");
      } break;
    }
    printf("Current Top Address: %zu\t", (size_t)Stack.TopOfMemory);
    printf("Space Remaining : %zu\t", Stack.SpaceRemaining);
    if(Stack.LastAlignmentIsHeader)
    {
      printf("Last Alignment Is Header\n");
    }
    else
    {
      printf("Last Alignment Is Footer\n");
    }
  }

  QueryPerformanceCounter(&NewTime);
  LARGE_INTEGER StackAllocationTime;
  SubtractLargeIntegers(&NewTime, &OldTime, &StackAllocationTime);

  printf("\nChars:\n");
  for(int CharArrayIndex = 0; CharArrayIndex < CharArraySize; ++CharArrayIndex)
  {
    printf("[%i]:%d\t", CharArrayIndex, *CharArray[CharArrayIndex]);
  }
  printf("\n\nInts:\n");
  for(int IntArrayIndex = 0; IntArrayIndex < IntArraySize; ++IntArrayIndex)
  {
    printf("[%i]:%d\t", IntArrayIndex, *IntArray[IntArrayIndex]);
  }
  printf("\n\nLongs:\n");
  for(int LongArrayIndex = 0; LongArrayIndex < LongArraySize; ++LongArrayIndex)
  {
    printf("[%i]:%d\t", LongArrayIndex, *LongArray[LongArrayIndex]);
  }

  printf("\n\n");
  printf("Stack Deallocation Starting\n");
  printf("\t\t\tCurrent Top Address: %zu\t", (size_t)Stack.TopOfMemory);
  printf("Space Remaining : %zu\t", Stack.SpaceRemaining);
  if(Stack.LastAlignmentIsHeader)
  {
    printf("Last Alignment Is Header\n");
  }
  else
  {
    printf("Last Alignment Is Footer\n");
  }

  QueryPerformanceCounter(&OldTime);

  for(int TestIndex = TestCount - 1; TestIndex >= 0; --TestIndex)
  {
    switch(TestChoiceOrder[TestIndex])
    {
      case 0:
      {
        DeallocateSpaceOnStack(&Stack, char);
        printf("-Deallocated(char)\t");
      } break;
      case 1:
      {
        DeallocateSpaceOnStack(&Stack, int);
        printf("-Deallocated(int)\t");
      } break;
      case 2:
      {
        DeallocateSpaceOnStack(&Stack, long);
        printf("-Deallocated(long)\t");
      } break;
    }
    printf("Current Top Address: %zu\t", (size_t)Stack.TopOfMemory);
    printf("Space Remaining : %zu\t", Stack.SpaceRemaining);
    if(Stack.LastAlignmentIsHeader)
    {
      printf("Last Alignment Is Header\n");
    }
    else
    {
      printf("Last Alignment Is Footer\n");
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
  printf("\n\n\n");

  CharArraySize = 0;
  IntArraySize = 0;
  LongArraySize = 0;

///  list List;
///  InitializeList(&List, TestCount * (sizeof(long) + alignof(long)) * 2);
///  printf("Current Top Address: %u\n", (size_t)List.Head);
///
///  for (int TestIndex = 0; TestIndex < TestCount * 2; ++TestIndex)
///  {
///    int TestChoice = rand() % 3;
///    bool Allocate = rand() % 2 == 1;
///
///    switch (TestChoice)
///    {
///      case 0:
///      {
///        if (Allocate || CharArraySize == 0)
///        {
///          CharArray[CharArraySize] = (unsigned char *)AllocateSpaceOnList(&List, char);
///          if (CharArray[IntArraySize] == NULL) {
///            printf("Memory Full!\t");
///            break;
///          }
///          *CharArray[CharArraySize] = (unsigned char)(CharArraySize);
///          printf("-Allocated(char)\tA:%u\t", (size_t)CharArray[CharArraySize]);
///          printf("Size Allocated : %u\t", GetSizeOfAllocation(CharArray[CharArraySize]));
///          printf("Padding : %u\t", GetPaddingOfAllocation(CharArray[CharArraySize]));
///          ++CharArraySize;
///        }
///        else
///        {
///          DeallocateSpaceOnList(&List, CharArray[CharArraySize - 1]);
///          printf("-Deallocated(char): %u\t", *CharArray[CharArraySize - 1]);
///          --CharArraySize;
///        }
///      } break;
///      case 1:
///      {        
///        if (Allocate || IntArraySize == 0)
///        {
///          IntArray[IntArraySize] = (unsigned int *)AllocateSpaceOnList(&List, int);
///          if (IntArray[IntArraySize] == NULL) {
///            printf("Memory Full!");
///            break;
///          }
///          *IntArray[IntArraySize] = (unsigned int)(IntArraySize);
///          printf("-Allocated(int) \tA:%u\t", (size_t)IntArray[IntArraySize]);
///          printf("Size Allocated : %u\t", GetSizeOfAllocation(IntArray[IntArraySize]));
///          printf("Padding : %u\t", GetPaddingOfAllocation(IntArray[IntArraySize]));
///          ++IntArraySize;
///        }
///        else
///        {
///          DeallocateSpaceOnList(&List, IntArray[IntArraySize - 1]);
///          printf("-Deallocated(int): %u\t", *IntArray[IntArraySize - 1]);
///          --IntArraySize;
///        }
///      } break;
///      case 2:
///      {
///        if (LongArray[LongArraySize] == NULL) {
///          printf("Memory Full!\t");
///          break;
///        }
///
///        if (Allocate || LongArraySize == 0)
///        {
///          LongArray[LongArraySize] = (unsigned long *)AllocateSpaceOnList(&List, long);
///          *LongArray[LongArraySize] = (unsigned long)(LongArraySize);
///          printf("-Allocated(long)\tA:%u\t", (size_t)LongArray[LongArraySize]);
///          printf("Size Allocated : %u\t", GetSizeOfAllocation(LongArray[LongArraySize]));
///          printf("Padding : %u\t", GetPaddingOfAllocation(LongArray[LongArraySize]));
///          ++LongArraySize;
///        }
///        else
///        {
///          DeallocateSpaceOnList(&List, LongArray[LongArraySize - 1]);
///          printf("-Deallocated(long): %u\t", *LongArray[LongArraySize - 1]);
///          --LongArraySize;
///        }
///      } break;
///    }
///    printf("Space Remaining : %d\n", List.SpaceRemaining);
///  }
///
///  printf("\nSpace Remaining : %d\n", List.SpaceRemaining);
///
///  while (CharArraySize > 0)
///  {
///    DeallocateSpaceOnList(&List, CharArray[CharArraySize - 1]);
///    printf("-Deallocated(char): %u\t", *CharArray[CharArraySize - 1]);
///    printf("Space Remaining : %d\n", List.SpaceRemaining);
///    --CharArraySize;
///  }
///
///  while (IntArraySize > 0)
///  {
///    DeallocateSpaceOnList(&List, IntArray[IntArraySize - 1]);
///    printf("-Deallocated(int): %u\t", *IntArray[IntArraySize - 1]);
///    printf("Space Remaining : %d\n", List.SpaceRemaining);
///    --IntArraySize;
///  }
///
///  while (LongArraySize > 0)
///  {
///    DeallocateSpaceOnList(&List, LongArray[LongArraySize - 1]);
///    printf("-Deallocated(long): %u\t", *LongArray[LongArraySize - 1]);
///    printf("Space Remaining : %d\n", List.SpaceRemaining);
///    --LongArraySize;
///  }

  return 0;
}
