C-Memory-Allocators
===================
A pair of memory allocators, stack & list, allowing for a more rapid and
spacially conservant way of distributing memory within a program. This project
was made with video games in mind, but can be leveraged for use in any program
where the maximum memory need is pre-defined.  
  
### Stack Allocator
This allocator works by assigning memory via a stack-like structure. This
implementation places the overhead data of each allocated chunk within the
alignment padding.  
```c
void InitializeStack(stack *Stack, size_t Size);

void* AllocateSpaceOnStack_(stack *Stack, size_t Size, short Alignment);
#define AllocateSpaceOnStack(Stack, Type) \
        AllocateSpaceOnStack_(Stack, sizeof(Type), alignof(Type))

void DeallocateSpaceOnStack_(stack *Stack, size_t Size);
#define DeallocateSpaceOnStack(Stack, Type) \
        DeallocateSpaceOnStack_(Stack, sizeof(Type))
```
### List Allocator
This allocator works by assigning memory via a list-like structure. This
implementation stores the offset from the aligned header and the aligned data
as a char right before the data itself. This saves space while ensuring all
data is aligned.
```c
void InitializeList(list *List, size_t Size);

void* AllocateSpaceOnList_(list *List, size_t Size, unsigned char Alignment);
#define AllocateSpaceOnList(List, Type) \
        AllocateSpaceOnList_(List, sizeof(Type), alignof(Type))
        
void DeallocateSpaceOnList(list *List, void* Address);
```
### Notes
This repository includes a tester.  
!Note that although the allocators themselves are platform agnostic,
the tester is windows only.

Tester Build
------------

A pre-built binary is available within the build folder.
```
./build/test.exe
```

To build a new set of binaries on a Windows environment, first run either the
64-bit or 32-bit vcvarsall.bat, then `./build.bat`.

Tester Usage
-----

```
Usage: ./build/test.exe [option <value>]

The available options are as follows:

-v          Print memory allocation information.
-a <#####>  Specify allocation size.
-t <#####>  Specify test count.
```

Motivation
----------
I was first introduced to memory allocators during my batch at the Recurse
Center where I was encouraged and coerced into exploring a deeper level
of game programming by, my now excellent friend, Mariano (seriously,
[check him out](https://github.com/mtrebi), he's super cool).
I'm ever thankful of the time we spent scratching our heads and drawing
_advanced rectangles_ on white boards.
