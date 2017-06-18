/* ==========================================================================
   Описание: библиотека для автоматического определения кодировки текста
   Описание алгоритма: http://ivr.webzone.ru/articles/defcod_2/
   (c) Иван Рощин, Москва, 2004.
 ========================================================================= */

#ifndef __ADBYTECP_
#define __ADBYTECP_

#ifdef __cplusplus
extern "C" {
#endif

extern int m_def_code (const unsigned char *p, int len, int n);

#ifdef __cplusplus
}
#endif

#endif