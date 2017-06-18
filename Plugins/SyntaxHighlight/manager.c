#include <limits.h>
#include <memory.h>
#include <string.h>
#include <stdlib.h>

#include "manager.h"


#ifndef _ASSERTE
#define _ASSERTE(...)
#endif


#define MINIMAL_SIZE 12
#define USER_MEM_OFFSET 4

#define CELLSIZE(ptr) (*((int*)(ptr)))
#define CELLPREVIOUS(ptr) (char*)(*((unsigned int*)((char*)ptr + CELLSIZE(ptr) - 2*sizeof(int))))
#define CELLNEXT(ptr) (char*)(*((unsigned int*)((char*)ptr + CELLSIZE(ptr) - sizeof(int))))

#define SETSIZE(ptr, size) (*((int*)ptr)) = (size)
#define SETPREVPTR(p, ptr) (*((unsigned int*)((char*)(p) + CELLSIZE(p) - 2*sizeof(int)))) = (unsigned int)(ptr)
#define SETNEXTPTR(p, ptr) (*((unsigned int*)((char*)(p) + CELLSIZE(p) - sizeof(int)))) = (unsigned int)(ptr)

// we should adjust size of every cell due to alignment
#define ADJUST_CELL_SIZE(size) ((sizeof(long int) - ((size) % sizeof(long int))) % sizeof(long int))

#define NORMAL_BLOCK 0xffffffff

typedef struct tag_list_cell_t
{
  int size;
  char *previous, *next;
} list_cell_t;


/* ============ For tests ============ */
#ifdef _TEST_MODE_

extern int dumpNumber = 0;

extern void dumpAvailableMemory(memory_system_t *pmemSys, char *name)
{
  int *mas = (int*)pmemSys->head, i = 0;
  FILE *fOut;
  
  if (dumpNumber == 0)
    fOut = fopen(name, "w");
  else
    fOut = fopen(name, "a");

  if (fOut != NULL)
  {
    char *ptr = pmemSys->freeSpace;

    fprintf(fOut, "=== Dump number: %i ===\n\n", dumpNumber);
    fprintf(fOut, "Head: %0*X\n\n", 8, pmemSys->head);
    
    while (ptr)
    {
      i++;
      fprintf(fOut, "%0*i cell: %0*X\n", 2, i, 8, ptr);
      fprintf(fOut, "   size: %i\n", CELLSIZE(ptr));
      fprintf(fOut, "   previous: %0*X\n", 8, CELLPREVIOUS(ptr));
      fprintf(fOut, "   next: %0*X\n\n", 8, CELLNEXT(ptr));
      ptr = CELLNEXT(ptr);
    }
    
    fprintf(fOut, "\n\n\n");
    fclose(fOut);
  }
}

#endif
/* ======================================= */


static void excludeCell(memory_system_t *pmemSys, char *ptr)
{
  char *prev = CELLPREVIOUS(ptr), *next = CELLNEXT(ptr);
  if (prev)
    SETNEXTPTR(prev, next);
  else
    pmemSys->freeSpace = next;

  if (next)
    SETPREVPTR(next, prev);
}

extern char MemorySystemOpen(memory_system_t *pmemSys, int size)
{
  if (!pmemSys->head && size > MINIMAL_SIZE)
  {
    size += ADJUST_CELL_SIZE(size);
    if ((pmemSys->head = malloc(size)) != NULL)
    {
      pmemSys->systemSize = size;
      SETSIZE(pmemSys->head, size);
      SETPREVPTR(pmemSys->head, NULL);
      SETNEXTPTR(pmemSys->head, NULL);
      pmemSys->freeSpace = pmemSys->head;
      return 1;
    }
  }

  return 0;
}

extern char MemorySystemClose(memory_system_t *pmemSys)
{
  char isAllFree = (char)(pmemSys->freeSpace && CELLSIZE(pmemSys->head) == pmemSys->systemSize);

  if (pmemSys->head)
    free(pmemSys->head);
  pmemSys->head = NULL;
  pmemSys->freeSpace = NULL;
  pmemSys->systemSize = 0;
  return isAllFree;
}

extern void* MemorySystemAllocate(memory_system_t *pmemSys, size_t _size)
{
  char *ptr = pmemSys->freeSpace;
  char *bestCell = NULL;
  int size = (int)_size;

  if (size <= 0)
    return NULL;

  size += MINIMAL_SIZE;

  size += ADJUST_CELL_SIZE(size); 

  while (ptr)
  {
    if (CELLSIZE(ptr) > size)
    {
      if (bestCell && CELLSIZE(bestCell) > CELLSIZE(ptr))
        bestCell = ptr;
      else if (!bestCell)
        bestCell = ptr;
    }
    else if (CELLSIZE(ptr) == size)
    {
      bestCell = ptr;
      break;
    }

    ptr = CELLNEXT(ptr);
  }

  if (bestCell)
  {
    list_cell_t cell;

    cell.size = CELLSIZE(bestCell);
    cell.next = CELLNEXT(bestCell);
    cell.previous = CELLPREVIOUS(bestCell);

    SETSIZE(bestCell, cell.size - size <= MINIMAL_SIZE ? cell.size : size);
    SETPREVPTR(bestCell, NORMAL_BLOCK);
    SETNEXTPTR(bestCell, NORMAL_BLOCK);

    if (cell.size - size <= MINIMAL_SIZE)
    {
      // we must allocate the whole block

      if (cell.previous)
        SETNEXTPTR(cell.previous, cell.next);
      if (cell.next)
        SETPREVPTR(cell.next, cell.previous);

      if (bestCell == pmemSys->freeSpace)
        pmemSys->freeSpace = cell.next;
    }
    else
    {
      // we can allocate only part of the block

      char *newCell = bestCell + size;

      SETSIZE(newCell, cell.size - size);
      SETPREVPTR(newCell, cell.previous);
      SETNEXTPTR(newCell, cell.next);
      if (cell.previous)
        SETNEXTPTR(cell.previous, newCell);
      if (cell.next)
        SETPREVPTR(cell.next, newCell);

      if (bestCell == pmemSys->freeSpace)
        pmemSys->freeSpace = newCell;
    }

    SETSIZE(bestCell, -CELLSIZE(bestCell));
    bestCell += USER_MEM_OFFSET;
  }

  return (void*)bestCell;
}

extern void* MemorySystemCallocate(memory_system_t *pmemSys, size_t count, size_t size)
{
  void *pcell = MemorySystemAllocate(pmemSys, count * size);

  if (!pcell)
    return NULL;
  memset(pcell, 0, count * size);

  return pcell;
}

extern void* MemorySystemReallocate(memory_system_t *pmemSys, void *p, size_t _size)
{
  char *ptr = (char*)p;

  if (ptr)
  {
    char *pcell, restrictFreeingBeforeAllocation = 0;
    int effectiveSize, size;
    
    ptr -= USER_MEM_OFFSET;

    size = -CELLSIZE(ptr);
    effectiveSize = size - MINIMAL_SIZE;

    _ASSERTE(size > 0);

    pcell = ptr + size;

    if (ptr != pmemSys->head && *(unsigned int*)(ptr - sizeof(int)) != NORMAL_BLOCK)
    { 
      // previous cell is empty,

      if ((pcell = *(char**)(ptr - sizeof(int))) != NULL)
        pcell = CELLPREVIOUS(pcell);
      else if ((pcell = *(char**)(ptr - 2*sizeof(int))) != NULL)
        pcell = CELLNEXT(pcell);
      else
        pcell = pmemSys->freeSpace;

      effectiveSize += CELLSIZE(pcell);
      if ((size_t)effectiveSize > _size)
        restrictFreeingBeforeAllocation = 1; // if the system allocates previous cell, our data will be damaged
    }

    if (((int)(ptr - pmemSys->head) + size < pmemSys->systemSize) && CELLSIZE(pcell) > 0)
      effectiveSize += CELLSIZE(pcell); // next cell is empty

    if (!restrictFreeingBeforeAllocation && (size_t)effectiveSize >= _size)
    {
      // reallocation could be done, so we can free old cell
      MemorySystemFree(pmemSys, p);
      ptr = MemorySystemAllocate(pmemSys, _size);
      _ASSERTE(ptr != NULL);
    }
    else if ((ptr = MemorySystemAllocate(pmemSys, _size)) != NULL)
      MemorySystemFree(pmemSys, p);

    if (ptr != NULL && ptr != (char*)p)
      memcpy(ptr, p, size - MINIMAL_SIZE);
  }
  else
    ptr = MemorySystemAllocate(pmemSys, _size);

  return ptr;
}

extern void MemorySystemFree(memory_system_t *pmemSys, void *p)
{
  char *ptr = (char*)p;

  if (ptr)
  {
    char *pcell;
    int size;

    ptr -= USER_MEM_OFFSET;
    size = -CELLSIZE(ptr);

    _ASSERTE(size > 0);
    SETSIZE(ptr, size);
    pcell = ptr + size;

    _ASSERTE(NORMAL_BLOCK == (unsigned int)CELLPREVIOUS(ptr) && NORMAL_BLOCK == (unsigned int)CELLNEXT(ptr));

    if (((int)(ptr - pmemSys->head) + size < pmemSys->systemSize) && CELLSIZE(pcell) > 0)
    // Next cell exists and it is free
    {
      excludeCell(pmemSys, pcell);
      size += CELLSIZE(pcell);
      SETSIZE(ptr, size);
    }

    if (ptr != pmemSys->head && *(unsigned int*)(ptr - sizeof(int)) != NORMAL_BLOCK)
    // Previous cell exists and it is free
    {
      if ((pcell = *(char**)(ptr - sizeof(int))) != NULL)
        pcell = CELLPREVIOUS(pcell);
      else if ((pcell = *(char**)(ptr - 2*sizeof(int))) != NULL)
        pcell = CELLNEXT(pcell);
      else
        pcell = pmemSys->freeSpace;

      _ASSERTE(pcell);
      excludeCell(pmemSys, pcell);
      SETSIZE(pcell, size + CELLSIZE(pcell));
      ptr = pcell;
    }

    SETPREVPTR(ptr, NULL);
    SETNEXTPTR(ptr, pmemSys->freeSpace);
    if (pmemSys->freeSpace)
      SETPREVPTR(pmemSys->freeSpace, ptr);
    pmemSys->freeSpace = ptr;
  }
}