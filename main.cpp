#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#define internal static
#define global static
#define local_persist static

struct memory
{
  unsigned int Size;
  void *AllocatedSpace;
};

#include "stack_allocator.cpp"
#include "list_allocator.cpp"


#include <time.h>
#include <limits.h>

int main()
{
  srand((unsigned int)time(NULL));

  stack Stack;
  InitializeStack(&Stack, 200);

  char *CharArray[30];
  int CharArraySize = 0;
  int *IntArray[30];
  int IntArraySize = 0;
  long *LongArray[30];
  int LongArraySize = 0;

#define TestCount 15
  int TestChoiceOrder[TestCount];

  printf("Stack Allocation Starting\n");
  printf("\t\t\tCurrent Top Address: %u\t", (size_t)Stack.TopOfMemory);
  printf("Space Remaining : %d\n", Stack.SpaceRemaining);

  for(int TestIndex = 0; TestIndex < TestCount; ++TestIndex)
  {
    int TestChoice = rand() % 3;
    TestChoiceOrder[TestIndex] = TestChoice;
    switch(TestChoice)
    {
      case 0:
      {
        CharArray[CharArraySize] = (char *)AllocateSpaceOnStack(&Stack, char);
        *CharArray[CharArraySize] = (char)(rand() % UCHAR_MAX);
        ++CharArraySize;
        printf("-Allocated(char) \t");
      } break;
      case 1:
      {
        IntArray[IntArraySize] = (int *)AllocateSpaceOnStack(&Stack, int);
        *IntArray[IntArraySize] = (int)(rand() % UINT_MAX);
        ++IntArraySize;
        printf("-Allocated(int) \t");
      } break;
      case 2:
      {
        LongArray[LongArraySize] = (long *)AllocateSpaceOnStack(&Stack, long);
        *LongArray[LongArraySize] = (long)(rand() % ULONG_MAX);
        ++LongArraySize;
        printf("-Allocated(long) \t");
      } break;
    }
    printf("Current Top Address: %u\t", (size_t)Stack.TopOfMemory);
    printf("Space Remaining : %d\t", Stack.SpaceRemaining);
    if(Stack.LastAlignmentIsHeader)
    {
      printf("Last Alignment Is Header\n");
    }
    else
    {
      printf("Last Alignment Is Footer\n");
    }
  }

  printf("\nChars:\n");
  for(int CharArrayIndex = 0; CharArrayIndex < CharArraySize; ++CharArrayIndex)
  {
    printf("[%i]:%d ", CharArrayIndex, *CharArray[CharArrayIndex]);
  }
  printf("\n\nInts:\n");
  for(int IntArrayIndex = 0; IntArrayIndex < IntArraySize; ++IntArrayIndex)
  {
    printf("[%i]:%d ", IntArrayIndex, *IntArray[IntArrayIndex]);
  }
  printf("\n\nLongs:\n");
  for(int LongArrayIndex = 0; LongArrayIndex < LongArraySize; ++LongArrayIndex)
  {
    printf("[%i]:%d ", LongArrayIndex, *LongArray[LongArrayIndex]);
  }

  printf("\n\n");
  printf("Stack Deallocation Starting\n");
  printf("\t\t\tCurrent Top Address: %u\t", (size_t)Stack.TopOfMemory);
  printf("Space Remaining : %d\t", Stack.SpaceRemaining);
  if(Stack.LastAlignmentIsHeader)
  {
    printf("Last Alignment Is Header\n");
  }
  else
  {
    printf("Last Alignment Is Footer\n");
  }
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
    printf("Current Top Address: %u\t", (size_t)Stack.TopOfMemory);
    printf("Space Remaining : %d\t", Stack.SpaceRemaining);
    if(Stack.LastAlignmentIsHeader)
    {
      printf("Last Alignment Is Header\n");
    }
    else
    {
      printf("Last Alignment Is Footer\n");
    }
  }
  printf("\n\n\n");
  
  list List;
  InitializeList(&List, 30);
  int *Test = (int *)AllocateSpaceOnList(&List, int);
  *Test = 4;
  printf("Hello\n");
  printf("%i\n", *Test);
  DeallocateSpaceOnList(&List, Test);

  return 0;
}
