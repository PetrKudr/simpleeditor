#include "hlalloc.hpp"

#include "manager.h"


#ifdef HL_SELF_MEMORY_SYSTEM


static int s_nativeAllocateCount = 0;
static int s_nativeFreeCount = 0;
static memory_system_t s_memorySystem = {0};


static void* hlAllocate(size_t size)
{
  void *ptr = MemorySystemAllocate(&s_memorySystem, size);

  if (!ptr)
  {
    ptr = malloc(size);
    s_nativeAllocateCount++;
  }

  return ptr;
}

static void hlFree(void *ptr)
{
  if ((unsigned int)ptr >= (unsigned int)s_memorySystem.head && (unsigned int)ptr < ((unsigned int)s_memorySystem.head + s_memorySystem.systemSize))
    MemorySystemFree(&s_memorySystem, ptr);
  else
  {
    free(ptr);
    s_nativeFreeCount++;
  }
}


/*
***************************************************************************
*   External functions
***************************************************************************
*/

extern void InitMemorySystem()
{
  MemorySystemOpen(&s_memorySystem, 16777216); // 16 MB
}

extern void CloseMemorySystem()
{
  MemorySystemClose(&s_memorySystem);
}

extern int GetNativeAllocateCount()
{
  return s_nativeAllocateCount;
}

extern int GetNativeFreeCount()
{
  return s_nativeFreeCount;
}

/*
***************************************************************************
*   Overloading new/delete
***************************************************************************
*/

extern void* __cdecl operator new(size_t size)
{
  return hlAllocate(size);
}

extern void __cdecl operator delete(void *p)
{
  hlFree(p);
}

extern void* __cdecl operator new[](size_t size)
{
  return hlAllocate(size);
}

extern void __cdecl operator delete[](void *p)
{
  hlFree(p);
}


#endif // HL_SELF_MEMORY_SYSTEM
