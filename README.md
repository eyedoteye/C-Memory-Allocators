-Memory-Allocators
===================
A pair of memory allocators, stack & list, allowing for a more rapid and
spacially conservant way of distributing memory within a program. This project
was made with video games in mind, but can be leveraged for use in any program
where the maximum memory need is pre-defined.  
  
### Stack Allocator
This allocator works by assigning memory via a stack-like structure. This
specific implementation is unique in that, it tries to place the location of the
overhead data required for each allocated chunk, within the gaps inbetween each
chunk, that are created when ensuring the chunks are aligned within memory.  
```c
void InitializeStack(stack *Stack, size_t Size) 
void* AllocateSpaceOnStack_(stack *Stack, size_t Size, short Alignment)
void* AllocateSpaceOnStack(Stack, Type)
      // ^ Stack, sizeof(Type), alignof(Type) are passed along to the previous
function.
void DeallocateSpaceOnStack_(stack *Stack, size_t Size)
void DeallocateSpaceOnStack(Stack, Type)
     // ^ Stack, sizeof(Type) are passed along to the previous function.
```
### List Allocator
This allocator works by assigning memory via a list-like structure. This
specific implementation is unique in that, it splits the overhead data
required
for each allocated chunk into two parts. Since the overhead data must also be
aligned, the position offset from the overhead to the corresponding allocation
is stored as a char and packed right before the allocated chunk. This allows
for
a speedy lookup of the overhead data when deallocating, and ultimately
provides
more usable space.
```c
void InitializeList(list *List, size_t Size)
void* AllocateSpaceOnList_(list *List, size_t Size, unsigned char Alignment)
void* AllocateSpaceOnList(List, Type)
      // ^ List, sizeof(Type), alignof(Type) are passed along to the previous
function.
void DeallocateSpaceOnList_(list *List, void* Address)
```
### Notes
This repository includes a testing interface.  
!Note that although the allocators themselves are platform agnostic, the
tester
is windows only.

Testing Interface Installation
------------

A pre-built binary is available within the build folder.
```
./build/test.exe
```

To build a new set of binaries on a Windows environment, first run either the
64-bit or 32-bit vcvarsall.bat, then `./build.bat`.

Usage
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
Center. There I was encouraged and coerced by the prodigy, Mariano (seriously,
[check him out](https://github.com/mtrebi), he's super cool), into exploring
some of the more advanced things that are involved in game programming with more
depth. I'm ever thankful of the time we spent scratching our heads and drawing
_advanced rectangles_ on white boards.
