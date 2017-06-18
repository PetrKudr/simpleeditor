#include "..\debug.h"
#include "stringmas.h"

#pragma warning(disable:4996)

#define START_SIZE 4
#define STRING_MASSIVE_COEF 2


/* default implementation of memory functions             */
/*                                                        */
/* memory - parameter for the memory system.              */
/* if defualt memory functions are used, it can be null   */
/**********************************************************/
static void* __cdecl _mas_malloc(void* memory, size_t size)
{
  return malloc(size);
}

static void* __cdecl _mas_calloc(void* memory, size_t count, size_t size)
{
  return calloc(count, size);
}

static void __cdecl _mas_free(void* memory, void *ptr)
{
  free(ptr);
}

static mas_malloc_func mas_malloc = _mas_malloc;
static mas_calloc_func mas_calloc = _mas_calloc;
static mas_free_func mas_free = _mas_free;

/**********************************************************/


static void destroyMassive(void *memory, string_massive_t *pstrMas)
{
  size_t i;

  for (i = 0; i < pstrMas->n; i++)
  {
    _ASSERTE(pstrMas->mas[i].str != NULL);
    mas_free(memory, pstrMas->mas[i].str);
  }
  if (pstrMas->mas)
    mas_free(memory, pstrMas->mas);
  pstrMas->size = pstrMas->n = 0;
}

static int compareStrings(const wchar_t *str1, size_t len1, const wchar_t *str2, size_t len2)
{
  if (len1 < len2)
    return -1;
  else if (len1 > len2)
    return 1;

  return wcsncmp(str1, str2, len1);
}

static unsigned int findPlace(string_massive_t *pstrMas, const wchar_t *str, size_t len, char *isEqual)
{
#ifdef UNSORTED
  unsigned int i;

  *isEqual = 0;

  if (pstrMas->n == 0)
    return 0;

  for (i = 0; i < pstrMas->n; i++)
  {
    if (compareStrings(pstrMas->mas[i].str, pstrMas->mas[i].len, str, len) == 0)
    {
      *isEqual = 1;
      return i;
    }
  }
  return pstrMas->n;

#else
  unsigned int begin, end, i;
  int cmp = 0;

  *isEqual = 0;

  if (pstrMas->n == 0)
    return 0;

  begin = 0;
  end = pstrMas->n - 1;
  i = (begin + end) / 2;

  while (begin < i)
  {
    cmp = compareStrings(pstrMas->mas[i].str, pstrMas->mas[i].len, str, len);
    if (cmp > 0)
      end = i;
    else if (cmp < 0)
      begin = i;
    else
    {
      *isEqual = 1;
      return i;
    }
    i = (begin + end) / 2;
  }

  cmp = compareStrings(pstrMas->mas[begin].str, pstrMas->mas[begin].len, str, len);
  if (cmp > 0)
    return begin;
  else if (cmp < 0 && end == begin)
    return begin + 1;
  else if (cmp == 0)
  {
    *isEqual = 1;
    return begin;
  }

  _ASSERTE(end - begin == 1);
  cmp = compareStrings(pstrMas->mas[end].str, pstrMas->mas[end].len, str, len);
  if (cmp > 0)
    return end;
  else if (cmp < 0)
    return end + 1;
  else 
  {
    *isEqual = 1;
    return end;
  }
#endif
}

static void insertString(string_massive_t *pstrMas, wchar_t *str, size_t len, unsigned int place)
{
  unsigned int i;

  for (i = pstrMas->n; i > place; i--)
    pstrMas->mas[i] = pstrMas->mas[i - 1];
  pstrMas->mas[place].str = str;
  pstrMas->mas[place].len = len;
  pstrMas->n++;
}


extern string_massive_t* MasInitMassive(void *memory)
{
  return (string_massive_t*)mas_calloc(memory, 1, sizeof(string_massive_t));
}


// function returns pointer to the added string
extern wchar_t* MasAddString(void *memory, string_massive_t *pstrMas, const wchar_t *str, size_t len)
{
  wchar_t *element;
  char isEqual;
  unsigned int place = findPlace(pstrMas, str, len, &isEqual);

  _ASSERTE(pstrMas != NULL);

  // if we already have such string, return success
  if (isEqual)
    return pstrMas->mas[place].str; 

  // allocate or reallocate memory for the massive, if necessary
  if (pstrMas->size == 0)
  {
    _ASSERTE(pstrMas->n == 0);
    if ((pstrMas->mas = mas_malloc(memory, sizeof(string_t) * START_SIZE)) == NULL)
      return NULL;
    pstrMas->size = START_SIZE;
  }
  else if (pstrMas->n + 1 == pstrMas->size)
  {
    string_t *tmp = pstrMas->mas;

    pstrMas->size *= STRING_MASSIVE_COEF;
    if ((pstrMas->mas = realloc(pstrMas->mas, sizeof(string_t) * pstrMas->size)) == NULL)
    {
      pstrMas->mas = tmp;
      destroyMassive(memory, pstrMas);
      return NULL;
    }
  }
  
  // allocate memory for the new element and insert it into the massive
  if ((element = mas_malloc(memory, sizeof(wchar_t) * (len + 1))) == NULL)
  {
    destroyMassive(memory, pstrMas);
    return NULL;
  }
  wcsncpy(element, str, len);
  element[len] = L'\0';
  insertString(pstrMas, element, len, place);

  return pstrMas->mas[place].str;
}


extern void MasDestroyMassive(void *memory, string_massive_t *pstrMas)
{
  _ASSERTE(pstrMas);
  destroyMassive(memory, pstrMas);
  mas_free(memory, pstrMas);
}

extern void MasSetMemoryFunctions(mas_malloc_func malloc_func, 
                                  mas_calloc_func calloc_func,
                                  mas_free_func free_func)
{
  mas_malloc = malloc_func;
  mas_calloc = calloc_func;
  mas_free = free_func;
}