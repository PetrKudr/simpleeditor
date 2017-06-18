#ifndef __MEM_MANAGER_
#define __MEM_MANAGER_

//#define _CRTDBG_MAP_ALLOC
//#include <stdlib.h>
//#include <crtdbg.h>

//#define _TEST_MODE_

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct tag_memory_system_t
{
  char *head;
  char *freeSpace;
  int systemSize;  
} memory_system_t;

#ifdef _TEST_MODE_
#include <stdio.h>
extern int dumpNumber;
extern void dumpAvailableMemory(memory_system_t *pmemSys, char *name);
#define DUMPLIST dumpAvailableMemory("dump.txt"); dumpNumber++
#endif

extern char MemorySystemOpen(memory_system_t *pmemSys, int size);
extern char MemorySystemClose(memory_system_t *pmemSys);

extern void* MemorySystemAllocate(memory_system_t *pmemSys, size_t _size);
extern void* MemorySystemCallocate(memory_system_t *pmemSys, size_t _count, size_t _size);
extern void* MemorySystemReallocate(memory_system_t *pmemSys, void *ptr, size_t _size);
extern void MemorySystemFree(memory_system_t *pmemSys, void *p);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif
