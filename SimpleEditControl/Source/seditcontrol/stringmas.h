#ifndef __STRING_MASSIVE_
#define __STRING_MASSIVE_

// Temporary module. Implements array of strings
// TODO: maybe it's better to replace array by something like tree (by incomplete suffix tree)

#include <stdio.h>
#include <wchar.h>

#define UNSORTED  // massive will be unsorted

typedef struct tag_string_t
{
  wchar_t *str;
  size_t len;
} string_t;

typedef struct tag_string_massive_t
{
  unsigned int n;
  unsigned int size;
  string_t *mas;
} string_massive_t;

typedef void* (__cdecl *mas_malloc_func)(void *memory, size_t size);
typedef void* (__cdecl *mas_calloc_func)(void *memory, size_t count, size_t size);
typedef void (__cdecl *mas_free_func)(void *memory, void *ptr);


extern string_massive_t* MasInitMassive(void *memory);
extern wchar_t *MasAddString(void *memory, string_massive_t *pstrMas, const wchar_t *str, size_t len);
extern void MasDestroyMassive(void *memory, string_massive_t *pstrMas);

extern void MasSetMemoryFunctions(mas_malloc_func malloc_func, 
                                  mas_calloc_func calloc_func,
                                  mas_free_func free_func);

#define MasGetLength(pstrMas) ((pstrMas)->n)
#define MasGetString(pstrMas, i) ((pstrMas)->mas[i].str)
#define MasGetStringLength(pstrMas, i) ((pstrMas)->mas[i].len)


#endif /* __STRING_MASSIVE_ */
