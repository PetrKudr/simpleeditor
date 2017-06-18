#ifndef __FILES_
#define __FILES_

#include <windows.h>
#include <wchar.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FILES_NO_ERROR          0
#define FILES_MEMORY_ERROR     -1
#define FILES_OPEN_FILE_ERROR  -2
#define FILES_SAVE_FILE_ERROR  -3
#define FILES_SEDIT_ERROR      -4
#define FILE_SEEKING_ERROR     -5

#define PATH_SIZE 512


typedef void (_cdecl *MAXPROGRESS_FUNC)(int index, int max);
typedef void (_cdecl *PROGRESS_FUNC)(int index, int progress);
typedef void (_cdecl *PROGRESS_END_FUNC)();


// Next four functions are hack for access to encoders. TODO: rewrite files.c

/** 
 * Decode function. 
 * PARAMETERS:
 *   opaqueCoding - parameter which represents coding. It will be the same as passed to LoadFile function
 *   str - string to decode with coding
 *   length - length of str (in chars)
 *   resultLength - length of result (in wchars)
 *
 * Returns string which must be alive until the next call to this function or the end of function LoadFile
 */
typedef wchar_t* (_cdecl *DECODE_FUNC)(__in void* opaqueCoding, __in const char *str, __in size_t length, __out size_t *resultLength);

/** 
 * Encode function. 
 * PARAMETERS:
 *   opaqueCoding - parameter which represents coding. It will be the same as passed to SaveFile function
 *   wstr - string to encode into coding
 *   length - length of wstr (in wchars)
 *   resultLength - length of result (in chars)
 *
 * Returns string which must be alive until the next call to this function or the end of function SaveFile
 */
typedef char* (_cdecl *ENCODE_FUNC)(__in void* opaqueCoding, __in const wchar_t *wstr, __in size_t length, __out size_t *resultLength);

/** 
 * Preprocessor for opaque coding.
 * PARAMETERS:
 *   opaqueCoding - parameter which represents coding
 *   isLoadProcess - true if it is LoadFile, false otherwise
 *   controlCharSize - size of control character
 *
 * Returns value which must be used as opaque coding in all functions, except DETERMINE_CODING_FUNC.
 */
typedef void* (_cdecl *INITIALIZE_CODING_FUNC)(__in void* opaqueCoding, __in char isLoadProcess, __out char *controlCharSize);

/** 
 * Post processor for opaque coding 
 * PARAMETERS:
 *   opaqueCoding - preprocessed coding
 *   isLoadProcess - true if it is LoadFile, false otherwise
 *
 * Returns value of coding before preprocessor
 */
typedef void* (_cdecl *FINALIZE_CODING_FUNC)(__in void* opaqueCoding, char isLoadProcess);

/** 
 * BOM function. Writes or skips BOM for the specified coding.
 * PARAMETERS:
 *   file - opened file descriptor
 *   opaqueCoding - parameter which represents coding. It will be the same as passed to LoadFile/SaveFile function
 *   isLoadProcess - true if it is LoadFile, false otherwise
 *
 * Returns any value for Save File, and for LoadFile: 1 if there was BOM in file, 0 otherwise
 */
typedef char (_cdecl *BOM_FUNC)(__in FILE *file, __in void* opaqueCoding, char isLoadProcess);

/** 
 * Function for determining coding
 * PARAMETERS:
 *   file - opened file descriptor 
 *
 * Returns coding
 */
typedef void* (_cdecl *DETERMINE_CODING_FUNC)(__in FILE *file);



extern void SetFileCallbacks(MAXPROGRESS_FUNC mpFunc, 
                             PROGRESS_FUNC pFunc, 
                             PROGRESS_END_FUNC peFunc,
                             DECODE_FUNC decoder,
                             ENCODE_FUNC encoder,
                             BOM_FUNC bomFunc,
                             DETERMINE_CODING_FUNC detemineCodingFunc,
                             INITIALIZE_CODING_FUNC initializeCoding,
                             FINALIZE_CODING_FUNC finalizeCoding);


extern char CheckFileExists(__in const wchar_t *path);

extern int RetrieveFileSize(__in const wchar_t *path);

extern void* LoadFile(__in HWND hwndSE, 
                      __in const wchar_t *path,
                      __in void *opaqueCoding, 
                      __in char useFastReadMode,
                      __out char *isBOM,
                      __out char *error);

extern char SaveFile(__in HWND hwndSE, 
                     __in const wchar_t *path,
                     __in void *opaqueCoding,
                     __in const wchar_t *lineBreak,
                     __in char needBOM);


#ifdef __cplusplus
}
#endif

#endif /* __FILES_ */