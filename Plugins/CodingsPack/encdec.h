#ifndef encdec_h__
#define encdec_h__

#include <wchar.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef enum tag_coding {
  CP866,
  WIN1251,
  KOI8R
} coding;


extern void EncodeString(__in const wchar_t *wstr, __in size_t length, __in coding codePage, __out unsigned char *encodedString);

extern void DecodeString(__in const unsigned char *str, __in size_t length, __in coding codePage, __out wchar_t *decodedString);


#ifdef __cplusplus
}
#endif

#endif // encdec_h__