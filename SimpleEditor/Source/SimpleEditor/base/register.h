#ifndef __REGISTER_
#define __REGISTER_

#ifdef __cplusplus
extern "C" {
#endif

#include <wchar.h>


#define REG_NO_ERROR           0
#define REG_REGISTER_ERROR    -1
#define REG_UNREGISTER_ERROR  -2


typedef struct tag_extension_t
{
  wchar_t name[8];
  char exist;
  int id;
} extension_t;


extern char TestCrossPadKey();
extern char TestAssociation(extension_t *ext);
extern char Associate(const extension_t *ext);
extern char Deassociate(const extension_t *ext);


#ifdef __cplusplus
}
#endif

#endif /* __REGISTER */
