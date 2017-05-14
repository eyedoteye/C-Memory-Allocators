#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <process.h>
#include <Windows.h>

#define boolint int

#define DEFAULT_ALLOCATION_SIZE 1000
#define DEFAULT_TEST_COUNT 1000

int main(int argv, char** argc)
{
  long MemSize = DEFAULT_ALLOCATION_SIZE;
  long TestCount = DEFAULT_TEST_COUNT;
  boolint Verbose = false;

  for(int ArgIndex = 1; ArgIndex < argv; ++ArgIndex)
  {
    char* Arg = argc[ArgIndex];
    if(strcmp(Arg, "-v") == 0)
    {
      Verbose = true;
    }
    else if(strcmp(Arg, "-a") == 0)
    {
      if(ArgIndex + 1 < argv)
        MemSize = strtol(argc[ArgIndex + 1], NULL, 10);
      else
        MemSize = -1;

      if(MemSize < 100 || MemSize > INT_MAX)
      {
        printf("Allocation size must be within 100 - %i.\n", INT_MAX);
        MemSize = -1;
        break;
      }
      else
        ++ArgIndex;
    }
    else if(strcmp(Arg, "-t") == 0)
    {
      if(ArgIndex + 1 < argv)
        TestCount = strtol(argc[ArgIndex + 1], NULL, 10);
      else
        TestCount = -1;

      if(TestCount < 1 || TestCount > INT_MAX)
      {
        printf("Test count must be within 1 - %i.\n", INT_MAX);
        TestCount = -1;
      }
      else
        ++ArgIndex;
    }
    else
    {
      printf("Error: unknown option %s\n", Arg);
      TestCount = -1;
      MemSize = -1;
    }
  }

  if(MemSize > 0 && TestCount > 0)
  {
    char ArgString[100];
    sprintf_s(ArgString, 1000, "%i %i", (int)MemSize, (int)TestCount); 

    // Using this std::string method feels super gross, but it works.
    // I'll let it pass since this tester isn't actually part of the real
    // program.
    char ThisExePath[1000];
    GetModuleFileName(NULL, ThisExePath, 1000); 
    std::string ThisExePathString = std::string(ThisExePath);

    char DirectoryPath[1000];
    sprintf_s(DirectoryPath, 1000, "%s",
              ThisExePathString.substr(0, ThisExePathString.find_last_of("\\"))
              .c_str());

    char FilePath[1000 + 20];
    if(Verbose)
    {
      sprintf_s(FilePath, 1000, "%s/test_v.exe", DirectoryPath);
    }
    else
    {
      sprintf_s(FilePath, 1000, "%s/test_.exe", DirectoryPath);
    }

    printf("Starting Test\n");
    _spawnl(_P_WAIT, FilePath,
            FilePath, ArgString, NULL);
    printf("Test Complete\n");
  }
  else
  {
    printf("Usage:./test.exe [option <value>]\n"
           "\n"
           "The available options are as follows:\n"
           "\n"
           "\t-v\t\t\tPrint memory allocation information.\n");
    printf("\t-a <1 - %i>\tSpecify allocation size (Default: %i).\n",
           INT_MAX, DEFAULT_ALLOCATION_SIZE); 
    printf("\t-t <1 - %i>\tSpecify test count (Default: %i).\n",
           INT_MAX, DEFAULT_TEST_COUNT);
    printf("\n");
  }

  return 0;
}

