#include <stdio.h>

#pragma warning(disable: 4996)

#include "..\..\debug.h"
#include "..\seditcontrol.h"
#include "files.h"

//#define MAX_BUFF_SIZE 0x40000    // 256 килобайт
//#define MIN_BUFF_SIZE 0x1000     // 4 килобайта
#define DEFAULT_BUFF_SIZE 0x1000 // размер буфера по умолчанию
#define OPTIMAL_DISK_USING 50    // оптимальное количество обращений к диску(для рассчёта размера буфера при чтении)

#define isSpecialSym(x) ((char)((x) == '\n' || (x) == '\r' || (x) == '\0'))


static MAXPROGRESS_FUNC s_mpFunc = NULL;
static PROGRESS_FUNC s_pFunc = NULL;
static PROGRESS_END_FUNC s_pEndFunc = NULL;
static DECODE_FUNC s_decoder = NULL;
static ENCODE_FUNC s_encoder = NULL;
static BOM_FUNC s_bomFunc = NULL;
static DETERMINE_CODING_FUNC s_determinCodingFunc = NULL;
static INITIALIZE_CODING_FUNC s_initializeCodingFunc = NULL;
static FINALIZE_CODING_FUNC s_finalizeCodingFunc = NULL;


/* Функция дополняет строку str данными из буфера. *str может быть NULL. 
 * ПАРАМЕТРЫ:
 *   buffer - буфер с информацией
 *   index - индекс в буфере, откуда начинать дополнять
 *   size - размер буфера 
 *   controlCharSize - размер управяющих символов в байтах (размер \0, \r, \n)
 *   lineFeed - последовательность перевода строки (\n)
 *   carriageReturn - последовательность возврата каретки (\r)
 *   str - указатель на строку
 *   length - длина строки
 *   next -  переменная, куда будет записана позиция в которой дополнение остановилось
 *   append - флаг:  0 если строка прочитана полностью,
 *                   1 если строку нужно дополнять (буфер кончился, а строка прочитана не до '\n')
 *                  <0 если произошла ошибка
 *
 * ВОЗВРАЩАЕТ: 1, если в строку было что-то добавлено, 0 иначе
 */
static unsigned int appendString(char *buffer, size_t index, size_t buffSize, unsigned int controlCharSize, const char *lineFeed, const char *carriageReturn,
                                 char **str, size_t *curlen, size_t *strSize, unsigned int *next, char *append)
{
  unsigned int i = index;
  int _length;  // длина подстроки, которой будет дополнена str
  int length = *curlen;
  char *_str = *str;   // для удобства работы и realloc-a
  char lineBreakFound = 0;

  *append = 0;
  if (0 == buffSize)
  {
    *append = 1;
    return 0;
  }

  while (i < buffSize && memcmp(&buffer[i], lineFeed, min(buffSize - i, controlCharSize)) != 0)
    i += controlCharSize;

  if (i == buffSize)
    *append = 1;
  else
    i += controlCharSize;

  _length = i - index;

  if (_length + length + sizeof(wchar_t) > *strSize) 
  {
    _str = realloc(*str, _length + length + sizeof(wchar_t));
    if (!_str)
    {
      *append = FILES_MEMORY_ERROR; 
      return 1;  // всё равно, что возвращать - ошибка установлена
    }
    *strSize = _length + length + sizeof(wchar_t);
    *str = _str;
  }
  memcpy(&_str[length], &buffer[index], min((unsigned int)_length, buffSize - index));

  if (*append == 0) // удалим '\r' и '\n' в конце
  {
    _length -= controlCharSize;
    while (memcmp(&_str[length + _length], carriageReturn, controlCharSize) == 0 || memcmp(&_str[length + _length], lineFeed, controlCharSize) == 0)
    { 
      memset(&_str[length + _length], 0, controlCharSize);
      if (_length + length >= controlCharSize)
        _length -= controlCharSize;
      else
        break;
    }
    if (memcmp(&_str[length + _length], carriageReturn, controlCharSize) != 0 && memcmp(&_str[length + _length], lineFeed, controlCharSize) != 0)
      _length += controlCharSize;
  }

  *next = i;
  *curlen = length + _length;

  return 1;
}

/* Функция читает файл в редактор
 * ПАРАМЕТРЫ:
 *   hwndCP - окно редактора
 *   fIn - дескриптор открытого файла
 *   size - размер файла (0 если не известен)
 *   codePage - кодировка текста
 * 
 * ВОЗВРАЩАЕТ: код ошибки
 */
static char readFile(__in HWND hwndSE,
                     __in FILE *fIn,
                     __in size_t size,
                     __in void *opaqueCoding, 
                     __in char controlCharSize)
{
  int dummy, total = 0, oldNext = 0; // for sending info
  int ret, id, errorCode = FILES_NO_ERROR;
  unsigned int readed, length, i = 0, next = 0, buffSize = DEFAULT_BUFF_SIZE, strSize = 0, wstrLength = 0;
  wchar_t *wstr = NULL;
  char *buffer = NULL, *str = NULL, *lineFeed = NULL, *carriageReturn = NULL;
  char isEnd = 0, append = 0, needNewBlock = 1, readedAll = 0;
  char needNewLine = 0, prevAppend = 1, isFirstLine = 1, isStr = 0;
  // isEnd - чтение и обработка файла закончены
  // append - строка прочитана не полностью, нужен следующий блок для её дополнения
  // needNewBlock - нужно прочитать следующий блок
  // readedAll - файл прочитан полностью
  // needNewLine - добавить в конец новую линию
  // prevAppend - нужно ли было дополнять предыдущую строку(нужно для одного специфического случая)
  // isFirstLine - это первая строка или нет.
  // isStr - есть ли что-то новое в строке str

  _ASSERTE(hwndSE && fIn);

  id = SendMessage(hwndSE, SE_GETID, 0, 0);

  //if (size > 0)
  //{
    //buffSize = (unsigned int)min(size / OPTIMAL_DISK_USING, MAX_BUFF_SIZE);
    //buffSize = max(buffSize, MIN_BUFF_SIZE);
    //if (buffSize % 2 != 0)
    //  ++buffSize;  // размер буфера должен быть чётным
  //}

  if (((buffer = malloc(buffSize)) == NULL) ||
      ((lineFeed = calloc(2*controlCharSize, 1)) == NULL) ||
      ((carriageReturn = calloc(2*controlCharSize, 1)) == NULL))
  {
    errorCode = FILES_MEMORY_ERROR;
    goto label_load_end;
  }

  if ((str = s_encoder(opaqueCoding, L"\n", 1, &dummy)) == NULL)
  {
    errorCode = FILES_MEMORY_ERROR;
    goto label_load_end;
  }
  else 
    memcpy(lineFeed, str, controlCharSize);

  if ((str = s_encoder(opaqueCoding, L"\r", 1, &dummy)) == NULL)
  {
    errorCode = FILES_MEMORY_ERROR;
    goto label_load_end;
  }
  else 
    memcpy(carriageReturn, str, controlCharSize);

  str = 0;

  if (s_mpFunc)
    s_mpFunc(id, (int)size);

  if ((ret = SendMessage(hwndSE, SE_BEGINAPPEND, 0, 0)) != SEDIT_NO_ERROR)
  {
    errorCode = ret;
    goto label_append_end;
  }

  while (!isEnd || needNewLine)
  {
    length = 0;
    append = 1;
    isStr = 0;

    if (isEnd && needNewLine)
    {
      //_ASSERTE(strSize > 0 && (codePage != AD_UTF_16 || strSize > 1));
      memset(str, 0, controlCharSize);
      isStr = 1;
      needNewLine = 0;
      append = 0;
    }

    while (append)
    {
      if (needNewBlock)
      {
        if ((readed = fread(buffer, sizeof(char), buffSize, fIn)) != buffSize)
          readedAll = 1;
        needNewBlock = 0;
      }
      oldNext = next;

      // appendString може вызывается больше одного раза,
      // это значит, что даже если в конце он вернёт 0, то isStr всё равно должен быть
      isStr = appendString(buffer, next, readed, controlCharSize, lineFeed, carriageReturn, &str, &length, &strSize, &next, &append) | isStr;
      if (append < 0)  // произошла ошибка
      {
        errorCode = append;
        goto label_append_end;
      }

      // передадим количество обработанных байт
      //if (pFunc)
      //  (*pFunc)(next - i, 0);
      if (s_pFunc)
        s_pFunc(id, (total += next - oldNext));

      if (next >= readed)
      {
        if (readedAll)
        {
          if (!append || (!prevAppend && 0 == readed))
            needNewLine = 1;   // в самом конце файла есть перевод строки, или весь файл мы прочитали в предыдущий раз и
                               // в конце последней строки стоял символ перевода
          if (isStr)
            memset(&str[length], 0, controlCharSize);
          isEnd = 1;
          append = 0;
        }
        else
        {
          prevAppend = append;   // prevAppend нужен только для случая, когда в предыдущий раз прочитали весь файл
          needNewBlock = 1;
          next = 0;
        }
      }
    }

    if (isStr)
    {
      _ASSERTE(str != NULL);

      if ((wstr = s_decoder(opaqueCoding, str, length, &wstrLength)) == NULL)
      {
        errorCode = FILES_MEMORY_ERROR;
        goto label_append_end;
      }

      SendMessage(hwndSE, SE_APPEND, (WPARAM)wstr, isFirstLine ? SEDIT_REPLACE : SEDIT_APPEND);
      isFirstLine = 0;
    }
  }

label_append_end:

  SendMessage(hwndSE, SE_ENDAPPEND, 0, 0);
  if (s_pFunc && s_pEndFunc)
  {
    s_pFunc(id, 0); // уведомим, что чтение окончено
    s_pEndFunc();
  }

label_load_end:

  if (buffer)
    free(buffer);
  if (lineFeed)
    free(lineFeed);
  if (carriageReturn)
    free(carriageReturn);
  if (str)
    free(str);

  return errorCode;
}

/* Функция читает файл в редактор быстро, но без прогресса. Выделяет себе буфер размером с файл.
 * ПАРАМЕТРЫ:
 *   hwndCP - окно редактора
 *   fIn - дескриптор открытого файла
 *   size - размер файла (0 если не известен)
 *   codePage - кодировка текста
 * 
 * ВОЗВРАЩАЕТ: код ошибки
 */
static char fastReadFile(__in HWND hwndSE,
                         __in FILE *fIn,
                         __in size_t size,
                         __in void *opaqueCoding, 
                         __in char controlCharSize)
{
  int errorCode = FILES_NO_ERROR;
  size_t readed, length;
  char *buffer = NULL;  
  wchar_t *wstr;

  _ASSERTE(hwndSE && fIn);


  if (size > 0)
  {
    doc_info_t di = {0};

    if ((buffer = malloc(size + 1)) == NULL)
    {
      errorCode = FILES_MEMORY_ERROR;
      goto label_load_end;
    }

    readed = fread(buffer, sizeof(char), size, fIn);

    if ((wstr = s_decoder(opaqueCoding, buffer, readed, &length)) == NULL)
    {
      errorCode = FILES_MEMORY_ERROR;
      goto label_load_end;
    }

    free(buffer);
    buffer = NULL;

    SendMessage(hwndSE, SE_PASTEFROMBUFFER, (WPARAM)wstr, 0);

    di.flags = SEDIT_SETCARETREALPOS | SEDIT_SETWINDOWPOS;
    SendMessage(hwndSE, SE_SETPOSITIONS, (WPARAM)&di, 0);
  }

label_load_end:

  if (buffer)
    free(buffer);

  return errorCode;
}

extern char CheckFileExists(__in const wchar_t *path)
{
  FILE *file;
  if ((file = _wfopen(path, L"r")) != NULL)
  {
    fclose(file);
    return 1;
  }
  return 0;
}

extern int RetrieveFileSize(__in const wchar_t *path)
{
  FILE *file;
  int size = -1;

  if ((file = _wfopen(path, L"rb")) != NULL)
  {
#if !defined(_WIN32_WCE)
    {
      struct _stat stat;
      if (_wstat(path, &stat) == 0)
        size = stat.st_size;
    }
#else
    if (fseek(file, 0, SEEK_END) == 0)  
      size = ftell(file);
#endif
    fclose(file);
  }
  return size;
}

/* Функция открывает, определяет кодировку и читает файл
 * ПАРАМЕТРЫ:
 *   hwndSE - окно редактора
 *   isBOM - флаг, была ли метка в файле
 *   error - код ошибки
 * 
 * ВОЗВРАЩАЕТ: кодировку
 */
extern void* LoadFile(__in HWND hwndSE, 
                      __in const wchar_t *path,
                      __in void *opaqueCoding, 
                      __in char useFastReadMode,
                      __out char *isBOM,
                      __out char *error)
{
  FILE *fIn;
  long long int size;

  *error = FILES_NO_ERROR;

  _ASSERTE(path && isBOM);

  if ((fIn = _wfopen(path, L"rb")) != NULL)
  {
    char controlCharSize;

    // попробуем получить размер файла
#if !defined(_WIN32_WCE)
    {
      struct _stat stat;
      if (_wstat(path, &stat) != 0)
        size = 0;
      else
        size = stat.st_size;
    }
#else
    if (fseek(fIn, 0, SEEK_END) == 0)  
    {
      size = ftell(fIn);
      if  (fseek(fIn, 0, SEEK_SET) != 0)
      {
        fclose(fIn);
        *error = FILE_SEEKING_ERROR;
        return opaqueCoding;
      }
    }
#endif

    if (!opaqueCoding) 
      opaqueCoding = s_determinCodingFunc(fIn);

    SendMessage(hwndSE, SE_CLEARTEXT, 0, 0);

    opaqueCoding = s_initializeCodingFunc(opaqueCoding, 1, &controlCharSize);
    *isBOM = s_bomFunc(fIn, opaqueCoding, 1);

    if (useFastReadMode)
      *error = fastReadFile(hwndSE, fIn, (size_t)size, opaqueCoding, controlCharSize);
    else
      *error = readFile(hwndSE, fIn, (size_t)size, opaqueCoding, controlCharSize);

    opaqueCoding = s_finalizeCodingFunc(opaqueCoding, 1);

    fclose(fIn);
  }
  else
    *error = FILES_OPEN_FILE_ERROR;

  return opaqueCoding;
}

/* Функция дополняет буфер строкой
 * ПАРАМЕТРЫ:
 *   buffer - буфер
 *   size -  размер буфера
 *   buffIndex - текущий индекс в буфере
 *   str - строка
 *   length - длина строки
 *   strIndex - текущий индекс в строке
 *   crlf - строка с последовательностью перевода строки
 *   crlfSize - длина crlf
 *
 * ВОЗВРАЩАЕТ: 1 - буфер полностью заполнен, 0 - буфер надо дополнять
 */
static char appendBuffer(char *buffer, size_t size, size_t *buffIndex, 
                         char *str, size_t length, size_t *strIndex,
                         char *crlf, unsigned int crlfSize)
{
  unsigned int i = *strIndex;
  unsigned int total = *buffIndex, copyLength;

  if (size <= total)
    return 1;        // буфер уже заполнен

  copyLength = length - i;
  copyLength = min(copyLength, size - total);
  memcpy(&buffer[total], &str[i], copyLength);
  total += copyLength;
  i += copyLength;
 
  if (i == length)  // строка полностью записана
  {
    memcpy(&buffer[total], crlf, crlfSize);
    total += crlfSize;
    *strIndex = 0;
    *buffIndex = total;
    return 0;
  }

  *strIndex = i;
  *buffIndex = total;
  return 1;
}

/* Функция сохраняет текст из редактора
 * ПАРАМЕТРЫ:
 *   hwndSE - окно редактора
 *   path - путь до файла
 *   codePage - кодировка
 *   needBOM - записывать ли BOM
 */
static char saveText(__in HWND hwndSE, 
                     __in const wchar_t *path,
                     __in void *opaqueCoding,
                     __in const wchar_t *lineBreak,
                     __in char controlCharSize,
                     __in char needBOM)
{
  FILE *fOut;
  int returnCode = FILES_NO_ERROR;

  if ((fOut = _wfopen(path, L"wb")) != NULL)
  {
    wchar_t *wstr = NULL;
    char *str = NULL, *crlf = NULL, *buffer = NULL;
    unsigned int strIndex = 0, buffIndex = 0;
    unsigned int strLen = 0, wstrSize = 0, crlfLength = 0;
    char isBufferFull = 0;    // признак, что буфер заполнен
    int ret, id;              // ret - значение, возвращённое редактором, id - идентификатор редактора
    int stringNumber = 0;     // номер строки

    // wstr - буфер для строк, получаемых из редактора
    // str - перекодированная строка, crlf - перекодированная строка CRLF, buffer - буфер для записи в файл
    // strIndex - текущий индекс в str (инициализирован нулём, меняется функцией appendBuffer)
    // buffIndex - текущий индекс в буфере (количество данных)
    // wstrSize - максимальное количество символов, помещающихся в строку wstr
    // strLen, crlfLength - длины соответствующих строк (меньше или равны размерам)

    id = SendMessage(hwndSE, SE_GETID, 0, 0);

    if (needBOM)
    {
      // write BOM
      s_bomFunc(fOut, opaqueCoding, 0);
    }

    if (((str = s_encoder(opaqueCoding, lineBreak, wcslen(lineBreak), &crlfLength)) == NULL) || 
        (buffer = malloc(DEFAULT_BUFF_SIZE + crlfLength)) == NULL)
    {
      returnCode = FILES_MEMORY_ERROR;
      goto label_save_end;
    }
    else 
    {
      if ((crlf = malloc(crlfLength * sizeof(char) + controlCharSize)) == NULL)
      {
        returnCode = FILES_MEMORY_ERROR;
        goto label_save_end;
      }
      memset(&crlf[crlfLength], 0, controlCharSize);
      memcpy(crlf, str, crlfLength);
    }

    if (s_mpFunc)
    {
      doc_info_t di = {0};
      if ((ret = SendMessage(hwndSE, SE_GETDOCINFO, (WPARAM)&di, 0)) != SEDIT_NO_ERROR)
      {
        returnCode = FILES_SEDIT_ERROR;
        goto label_save_end;
      }

      s_mpFunc(id, (int)di.nRealLines);
    }

    if ((ret = SendMessage(hwndSE, SE_BEGINRECEIVE, 0, 0)) != SEDIT_NO_ERROR)
    {
      returnCode = FILES_SEDIT_ERROR;
      goto label_save_end;
    }

    // ret == SEDIT_NO_ERROR
    while (ret != SEDIT_TEXT_END_REACHED)
    {

      // дополняем буфер строками, пока он не заполнится
      do
      {

        // получим новую строку
        do
        {

          // если буфер полностью заполнен, то старая строка может быть не до конца использована
          if (isBufferFull)
            break;

          ret = SendMessage(hwndSE, SE_GETNEXTLINE, (WPARAM)wstr, (LPARAM)wstrSize);
          if (ret < 0 && ret != SEDIT_TEXT_END_REACHED)
          {            
            returnCode = ret + FILES_SEDIT_ERROR;
            goto label_receive_end;
          }
          else if (ret > 0) // ret - длина строки + 1
          {
            wchar_t *tmp = wstr;
      
            if ((wstr = realloc(wstr, sizeof(wchar_t) * ret)) == NULL)
            {
              returnCode = FILES_MEMORY_ERROR;
              goto label_receive_end;
            }
            wstrSize = ret;
          }
        }
        while (ret > 0);

        _ASSERTE(wstr != NULL);
    
        // если установлен флаг, что буфер полный, значит новая строка не была считана => перекодировать не надо
        if (!isBufferFull)
        {
          // если строка получена успешно, надо перекодировать её
          if (ret == SEDIT_NO_ERROR)
          {
            if ((str = s_encoder(opaqueCoding, wstr, wcslen(wstr), &strLen)) == NULL)
            {
              returnCode = FILES_MEMORY_ERROR;
              goto label_receive_end;
            }            
            else if (s_pFunc)
              s_pFunc(id, ++stringNumber);
          }
          else  // ret == SEDIT_TEXT_END_REACHED, то есть в буфер уже ничего не добавить
          {
            _ASSERTE(buffIndex >= crlfLength);
            buffIndex -= crlfLength;
            break; 
          }
        }
        _ASSERTE(str);

        // isBufferFull - заполнен ли буфер до конца
        // в предельном случае, когда строка как раз поместилась, isBufferFull = 0
        isBufferFull = appendBuffer(buffer, DEFAULT_BUFF_SIZE, &buffIndex, str, strLen, &strIndex, crlf, crlfLength);
      } while (!isBufferFull);
 
      fwrite(buffer, 1, buffIndex, fOut); 
      buffIndex = 0;
    }

  label_receive_end:

    // this is because of the next calls could allocate some memory inside. If we get here through allocation error, we should get some memory at first.
    if (buffer)
    {
      free(buffer);
      buffer = NULL;
    }

    SendMessage(hwndSE, SE_ENDRECEIVE, 0, 0);
    if (s_pFunc && s_pEndFunc)
    {
      s_pFunc(id, 0); // уведомим, что запись окончена
      s_pEndFunc();
    }

  label_save_end:

    fclose(fOut);

    if (wstr)
      free(wstr);
    if (buffer)
      free(buffer);
    if (crlf)
      free(crlf);

  }
  else
    return FILES_SAVE_FILE_ERROR;  

  return FILES_NO_ERROR;
}

extern char SaveFile(__in HWND hwndSE, 
                     __in const wchar_t *path,
                     __in void *opaqueCoding,
                     __in const wchar_t *lineBreak,
                     __in char needBOM)
{
  char ret, controlCharSize;

  _ASSERTE(opaqueCoding);

  opaqueCoding = s_initializeCodingFunc(opaqueCoding, 0, &controlCharSize);
  ret = saveText(hwndSE, path, opaqueCoding, lineBreak, controlCharSize, needBOM);
  s_finalizeCodingFunc(opaqueCoding, 0);

  return ret;
}


extern void SetFileCallbacks(MAXPROGRESS_FUNC mpFunc,
                             PROGRESS_FUNC pFunc,
                             PROGRESS_END_FUNC peFunc,
                             DECODE_FUNC decoder,
                             ENCODE_FUNC encoder,
                             BOM_FUNC bomFunc,
                             DETERMINE_CODING_FUNC detemineCodingFunc,
                             INITIALIZE_CODING_FUNC initializeCoding,
                             FINALIZE_CODING_FUNC finalizeCoding)
{
  s_mpFunc = mpFunc;
  s_pFunc = pFunc;
  s_pEndFunc = peFunc;
  s_decoder = decoder;
  s_encoder = encoder;
  s_bomFunc = bomFunc;
  s_determinCodingFunc = detemineCodingFunc;
  s_initializeCodingFunc = initializeCoding;
  s_finalizeCodingFunc = finalizeCoding;
}