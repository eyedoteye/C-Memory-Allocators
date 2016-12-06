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
#include "list_allocator.cpp"

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