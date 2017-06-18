#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <limits.h>
#include "BitMassive.h"

#define BITS_IN_BYTE 8
#define SIZE_OF_BITMASS_ELEMENT 1 

extern bitmas_t InitBitmas(size_t size)
{
  return (bitmas_t)malloc((size / BITS_IN_BYTE ) + (size % BITS_IN_BYTE  && 1));
}

extern bitmas_t CInitBitmas(size_t size)
{
  return (bitmas_t)calloc(size / BITS_IN_BYTE + (size % BITS_IN_BYTE  && 1),
                          SIZE_OF_BITMASS_ELEMENT);
}

extern void CopyBitmas(bitmas_t destination, bitmas_t source, size_t size)
{
  unsigned int realSize = size / BITS_IN_BYTE  + (size % BITS_IN_BYTE && 1);

  memcpy((void*)destination, (void*)source, SIZE_OF_BITMASS_ELEMENT * realSize);
}

extern void SetBit(bitmas_t bitmas, unsigned int element)
{  
  bitmas += element / BITS_IN_BYTE ;
  *bitmas = (*bitmas) | (unsigned char)(1 << (element % BITS_IN_BYTE));
}

extern void ClearBit(bitmas_t bitmas, unsigned int element)
{
  unsigned char index = element % BITS_IN_BYTE ;
  
  bitmas += element / BITS_IN_BYTE ;
  *bitmas = ((unsigned char)(*bitmas << (BITS_IN_BYTE - index)) >> (BITS_IN_BYTE - index)) 
          + ((unsigned char)(*bitmas >> (index + 1)) << (index + 1));
}

extern char TestBit(bitmas_t bitmas, unsigned int element)
{
  unsigned char index = element % BITS_IN_BYTE ;

  bitmas += element / BITS_IN_BYTE ;
  return (char)(( (unsigned char)((*bitmas) << (BITS_IN_BYTE - 1 - index)) ) >> (BITS_IN_BYTE - 1));
}

extern void ClearBitmas(bitmas_t bitmas, size_t size)
{
  unsigned int i = size / BITS_IN_BYTE  + (size % BITS_IN_BYTE  && 1);
  
  while (i > 0)
    bitmas[--i] = 0;
  bitmas[0] = 0;
}

extern void ShlBitmas(bitmas_t bitmas, size_t size, unsigned int count)
{
  unsigned int i, bytes = size / BITS_IN_BYTE  + (size % BITS_IN_BYTE && 1);
  unsigned char tmp, tmpNext, k;
  unsigned short forLast;

  count %= (BITS_IN_BYTE + 1);

  tmp = *bitmas;
  tmpNext = *(bitmas + (1 % bytes));
  *bitmas <<= count;
  for (i = 1; i < bytes - 1; i++)
  {
    tmp >>= (BITS_IN_BYTE - count);
    
    *(bitmas + i) <<= count;
    *(bitmas + i) |= tmp;

    tmp = tmpNext;
    tmpNext = *(bitmas + (i + 1) % bytes);
  }

  if ((k = size % BITS_IN_BYTE ) == 0)
    k = BITS_IN_BYTE;

  forLast = ((unsigned short)tmpNext) << count;
  forLast |= (tmp >> (BITS_IN_BYTE - count));
  *(bitmas + bytes - 1) = forLast & UCHAR_MAX;
  *(bitmas + bytes - 1) <<= BITS_IN_BYTE - k;
  *(bitmas + bytes - 1) >>= BITS_IN_BYTE - k;

  *bitmas |= (unsigned char)(forLast >> k);
}

extern unsigned int GetNumberFromBitmas(bitmas_t bitmas, unsigned int start, unsigned int end)
{
  unsigned int res = 0, base = 1, i;

  for (i = start; i < end; i++)
  {
    if (TestBit(bitmas, i))
      res += base;
    base <<= 1;
  }

  return res;
}

extern void DestroyBitmas(bitmas_t bitmas)
{
  free((void*)bitmas);
}
