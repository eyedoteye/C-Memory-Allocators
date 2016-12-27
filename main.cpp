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
  unsigned int Size;
  void *AllocatedSpace;
};

#include "stack_allocator.cpp"
#include "list_allocator.cpp"

#include <time.h>
#include <limits.h>

internal void
PrintAllocationInfo(void *Address)
{
	allocated_list_header_properties AllocatedHeaderProperties;
	GetAllocatedHeaderFromAllocatedChunk(&AllocatedHeaderProperties, (size_t)Address);
  
	printf("AllocatedHeader Address:%u\t", AllocatedHeaderProperties.Address);
  printf("ChunkSize Allocated: %u\t", AllocatedHeaderProperties.ChunkSize);
	printf("PrePadding: %u\t", AllocatedHeaderProperties.PrePadding);
	printf("PostPadding: %u\n", AllocatedHeaderProperties.PostPadding);
}

internal bool
IsAllocationValid(void *Address, unsigned char Value)
{
	allocated_list_header_properties AllocatedHeaderProperties;
	GetAllocatedHeaderFromAllocatedChunk(&AllocatedHeaderProperties, (size_t)Address);

	return (AllocatedHeaderProperties.Type == 0 && AllocatedHeaderProperties.ChunkSize == sizeof(char) + Value)
		|| (AllocatedHeaderProperties.Type == 1 && AllocatedHeaderProperties.ChunkSize == sizeof(int) + Value)
		|| (AllocatedHeaderProperties.Type == 2 && AllocatedHeaderProperties.ChunkSize == sizeof(long) + Value);
}
//
//internal void
//PrintDeallocationInfo(void *Address)
//{
//  printf("Deallocation Address:%u\t", (size_t)Address);
//  printf("ChunkSize Deallocated : %u\t", GetSizeOfFreeChunk(Address));
//  printf("Offset : %u\t", GetPaddingOfFreeChunk(Address));
//}

internal void
PrintList(list *List)
{
  free_list_header *FreeHeader = List->Head;

  while(FreeHeader != NULL)
  {
    printf("FreeHeader Address:%u\t", (size_t)FreeHeader);
    printf("ChunkSize: %u\t", FreeHeader->ChunkSize);
    printf("Offset : %u\n", FreeHeader->Offset);
    FreeHeader = FreeHeader->Next;
  }
}

internal void
SubtractLargeIntegers(LARGE_INTEGER *X, LARGE_INTEGER *Y, LARGE_INTEGER *Result)
{
  Result->HighPart = X->HighPart - Y->HighPart;
  Result->LowPart = X->LowPart - Y->LowPart;

  //if((Y->LowPart < X->LowPart || Y->LowPart == X->LowPart) && Y->HighPart < X->HighPart
  //   && Result->LowPart < )
  //{

  //}
}

#define TestCount 300

internal bool
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
	printf("Total Space Allocated: %u\n", List.Memory.Size);
	printf("List Address Start: %u\n", (size_t)List.Memory.AllocatedSpace);
	printf("List Address End: %u\n", (size_t)List.Memory.AllocatedSpace + List.Memory.Size);

	for(int TestIndex = 0; TestIndex < TestCount * 2; ++TestIndex)
	{
		int TestChoice = rand() % 3;
		bool Allocate = rand() % 2 == 1;

		switch(TestChoice)
		{
			case 0:
			{
				if(Allocate || CharArraySize == 0)
				{
					CharArray[CharArraySize] = (unsigned char *)AllocateSpaceOnList(&List, char);
					if(CharArray[CharArraySize] == NULL) {
						printf("Cannot Allocate Char! Memory Full!\n");
						break;
					}
					printf("Char Allocated: %u\n", *CharArray[CharArraySize]);
					PrintAllocationInfo(CharArray[CharArraySize]);
					++CharArraySize;
				}
				else
				{
					--CharArraySize;
					printf("Char Deallocated: %u\tAddress: %u\n", *CharArray[CharArraySize], (size_t)CharArray[CharArraySize]);
					DeallocateSpaceOnList(&List, CharArray[CharArraySize]);
				}
			} break;
			case 1:
			{
				if(Allocate || IntArraySize == 0)
				{
					IntArray[IntArraySize] = (unsigned int *)AllocateSpaceOnList(&List, int);
					if(IntArray[IntArraySize] == NULL) {
						printf("Cannot Allocate Int! Memory Full!\n");
						break;
					}
					printf("Int Allocated: %u\n", *IntArray[IntArraySize]);
					PrintAllocationInfo(IntArray[IntArraySize]);
					++IntArraySize;
				}
				else
				{
					--IntArraySize;
					printf("Int Deallocated: %u\tAddress: %u\n", *IntArray[IntArraySize], (size_t)IntArray[IntArraySize]);
					DeallocateSpaceOnList(&List, IntArray[IntArraySize]);
				}
			} break;
			case 2:
			{
				if(Allocate || LongArraySize == 0)
				{
					LongArray[LongArraySize] = (unsigned long *)AllocateSpaceOnList(&List, long);
					if(LongArray[LongArraySize] == NULL) {
						printf("Cannot Allocate Long! Memory Full!\n");
						break;
					}
					printf("Long Allocated: %u\n", *LongArray[LongArraySize]);
					PrintAllocationInfo(LongArray[LongArraySize]);
					++LongArraySize;
				}
				else
				{
					--LongArraySize;
					printf("Long Deallocated: %u\tAddress: %u\n", *LongArray[LongArraySize], (size_t)LongArray[LongArraySize]);
					DeallocateSpaceOnList(&List, LongArray[LongArraySize]);
				}
			} break;
		}
		PrintList(&List);
		for(int CharArrayIndex = 0; CharArrayIndex < CharArraySize; ++CharArrayIndex)
		{
			printf("C[%u]:%u\t", CharArrayIndex, *CharArray[CharArrayIndex]);
			if(!IsAllocationValid(CharArray[CharArrayIndex], *CharArray[CharArrayIndex])) {
				allocated_list_header_properties AllocatedHeaderProperties;
				GetAllocatedHeaderFromAllocatedChunk(&AllocatedHeaderProperties, (size_t)CharArray[CharArrayIndex]);
				printf("BORKED\n");
			}
		}
		for(int IntArrayIndex = 0; IntArrayIndex < IntArraySize; ++IntArrayIndex)
		{
			printf("I[%u]:%u\t", IntArrayIndex, *(unsigned char*)IntArray[IntArrayIndex]);
			if(!IsAllocationValid(IntArray[IntArrayIndex], *IntArray[IntArrayIndex])) {
				allocated_list_header_properties AllocatedHeaderProperties;
				GetAllocatedHeaderFromAllocatedChunk(&AllocatedHeaderProperties, (size_t)IntArray[IntArrayIndex]);
				printf("BORKED\n");
			}
		}
		for(int LongArrayIndex = 0; LongArrayIndex < LongArraySize; ++LongArrayIndex)
		{
			printf("L[%u]:%u\t", LongArrayIndex, *(unsigned char*)LongArray[LongArrayIndex]);
			if(!IsAllocationValid(LongArray[LongArrayIndex],*LongArray[LongArrayIndex])) {
				allocated_list_header_properties AllocatedHeaderProperties;
				GetAllocatedHeaderFromAllocatedChunk(&AllocatedHeaderProperties, (size_t)LongArray[LongArrayIndex]);
				printf("BORKED\n");
			}
		}
		printf("Space Remaining: %d\n", List.SpaceRemaining);
	}

	printf("\nTotal Space Reserved: %u\n", List.Memory.Size);
	printf("Space Remaining After Random Testing: %d\n\n", List.SpaceRemaining);

	printf("\nChars:\n");
	for(int CharArrayIndex = 0; CharArrayIndex < CharArraySize; ++CharArrayIndex)
	{
		printf("[%i]:%u\tChar Deallocated\n", CharArrayIndex, *CharArray[CharArrayIndex]);
		DeallocateSpaceOnList(&List, CharArray[CharArrayIndex]);
		PrintList(&List);
		printf("Space Remaining: %d\n", List.SpaceRemaining);
	}
	printf("\n\nInts:\n");
	for(int IntArrayIndex = 0; IntArrayIndex < IntArraySize; ++IntArrayIndex)
	{
		printf("[%i]:%u\tInt Deallocated\n", IntArrayIndex, *IntArray[IntArrayIndex]);
		DeallocateSpaceOnList(&List, IntArray[IntArrayIndex]);
		PrintList(&List);
		printf("Space Remaining: %d\n", List.SpaceRemaining);
	}
	printf("\n\nLongs:\n");
	for(int LongArrayIndex = 0; LongArrayIndex < LongArraySize; ++LongArrayIndex)
	{
		printf("[%i]:%u\tLong Deallocated\n", LongArrayIndex, *LongArray[LongArrayIndex]);
		DeallocateSpaceOnList(&List, LongArray[LongArrayIndex]);
		PrintList(&List);
		printf("Space Remaining: %d\n", List.SpaceRemaining);
	}

	printf("\nTotal Space Reserved: %u\n", List.Memory.Size);
	printf("Space Remaining After Full Deallocating: %d\n\n", List.SpaceRemaining);

	bool WhatTheDuck = List.Memory.Size != List.SpaceRemaining || List.Head->Next != NULL;

	PrintList(&List);

	return WhatTheDuck;
}

int
main()
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

  unsigned char *CharArray[TestCount];
  int CharArraySize = 0;
  unsigned int *IntArray[TestCount];
  int IntArraySize = 0;
  unsigned long *LongArray[TestCount];
  int LongArraySize = 0;

  int TestChoiceOrder[TestCount];

  printf("Stack Allocation Starting\n");
  printf("\t\t\tCurrent Top Address: %u\t", (size_t)Stack.TopOfMemory);
  printf("Space Remaining : %d\n", Stack.SpaceRemaining);

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
        //*CharArray[CharArraySize] = (char)(rand() % UCHAR_MAX);
        //*CharArray[CharArraySize] = (unsigned char)(CharArraySize);
        ++CharArraySize;
        printf("-Allocated(char) \t");
      } break;
      case 1:
      {
        IntArray[IntArraySize] = (unsigned int *)AllocateSpaceOnStack(&Stack, int);
        //*IntArray[IntArraySize] = (int)(rand() % UINT_MAX);
        //*IntArray[IntArraySize] = (unsigned int)(IntArraySize);
        ++IntArraySize;
        printf("-Allocated(int) \t");
      } break;
      case 2:
      {
        LongArray[LongArraySize] = (unsigned long *)AllocateSpaceOnStack(&Stack, long);
        //*LongArray[LongArraySize] = (long)(rand() % ULONG_MAX);
        //*LongArray[LongArraySize] = (unsigned long)(LongArraySize);
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

  QueryPerformanceCounter(&NewTime);
  LARGE_INTEGER StackDeallocationTime;
  SubtractLargeIntegers(&NewTime, &OldTime, &StackDeallocationTime);

  //double StackInitializationTimeInMS = (StackInitializationTime * 1000.f) / PerformanceCounterFrequency;
  //double StackAllocationTimeInMS = (StackAllocationTime * 1000.f) / PerformanceCounterFrequency);
  //double StackDeallocationTimeInMS = (StackDeallocationTime * 1000.f) / PerformanceCounterFrequency);

  printf("Stack Initialization Time: %fms [%u %u]\n", StackInitializationTime.LowPart * 1000.f / PerformanceCounterFrequency.LowPart,
         StackInitializationTime.HighPart, StackInitializationTime.LowPart);
  printf("Stack Allocation Time: %fms [%u %u]\n", StackAllocationTime.LowPart * 1000.f / PerformanceCounterFrequency.LowPart,
         StackAllocationTime.HighPart, StackAllocationTime.LowPart);
  printf("Stack Deallocation Time: %fms [%u %u]\n", StackDeallocationTime.LowPart * 1000.f / PerformanceCounterFrequency.LowPart,
         StackDeallocationTime.HighPart, StackDeallocationTime.LowPart);
  printf("QueryPerformanceFrequency: [%u %u]\n", PerformanceCounterFrequency.HighPart, PerformanceCounterFrequency.LowPart);
  printf("\n\n\n");

  

 // list List;
 // InitializeList(&List, 34);
	//PrintList(&List);
 // printf("Space Remaining : %d\n", List.SpaceRemaining);
 // 
 // CharArray[0] = (unsigned char *)AllocateSpaceOnList(&List, char);
 // *CharArray[0] = 0;
 // PrintAllocationInfo(CharArray[0]);
	//PrintList(&List);
 // printf("Space Remaining: %d\n", List.SpaceRemaining);

 // CharArray[1] = (unsigned char *)AllocateSpaceOnList(&List, char);
 // *CharArray[1] = 1;
 // PrintAllocationInfo(CharArray[1]);
	//PrintList(&List);
 // printf("Space Remaining: %d\n", List.SpaceRemaining);

 // CharArray[2] = (unsigned char *)AllocateSpaceOnList(&List, char);
 // *CharArray[2] = 2;
 // PrintAllocationInfo(CharArray[2]);
	//PrintList(&List);
 // printf("Space Remaining: %d\n", List.SpaceRemaining);

 // DeallocateSpaceOnList(&List, CharArray[0]);
 // PrintList(&List);
 // printf("Space Remaining: %d\n", List.SpaceRemaining);

 // DeallocateSpaceOnList(&List, CharArray[2]);
 // PrintList(&List);
 // printf("Space Remaining: %d\n", List.SpaceRemaining);

 // DeallocateSpaceOnList(&List, CharArray[1]);
 // PrintList(&List);
 // printf("Space Remaining: %d\n", List.SpaceRemaining);

 // CharArray[1] = (unsigned char *)AllocateSpaceOnList(&List, char);
 // *CharArray[1] = 10;
 // PrintAllocationInfo(CharArray[1]);
 // printf("Space Remaining: %d\n", List.SpaceRemaining);


	CharArraySize = 0;
	IntArraySize = 0;
	LongArraySize = 0;

	ListTest();

  return 0;
}
