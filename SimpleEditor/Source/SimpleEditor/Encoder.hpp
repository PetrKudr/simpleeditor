#ifndef Encoder_h__
#define Encoder_h__

#include <windows.h>
#include <cstdio>

#include "PluginTypes.hpp"


#define ENCODER_MAX_CODING_ID_LENGTH 32


struct Encoder
{

  /**
   *  Returns coding id
   */
  virtual LPCWSTR GetCodingId() const = 0;

  /**
   *  Returns coding name
   */
  virtual LPCWSTR GetCodingName() const = 0;

  /**
   *  Returns size of control character ('\r', '\n' and '\0'. For example UTF-16 LE/BE have size = 2, but all ANSI codings and UTF-8 have size = 1)
   */
  virtual int GetControlCharacterSize() const = 0;

  /**
   *  Returns probability (value between 0 and 1) that the coding of buffer is the coding of this encoder.
   *  Params:
   *    buff - buffer
   *    size - size of buffer
   */
  virtual float DetectCoding(const char *buff, int size) = 0;

  /**
   *  Decodes string from encoder's coding to the UTF-16LE.
   *  Params:
   *    str - string to be decoded
   *    length - length of the string (in chars)
   *    result - here encoder must place decoded string
   *    resultLength - length of the decoded string (in wchars)
   */
  virtual void DecodeString(__in const char *str, __in size_t length, __inout PluginBuffer<wchar_t> &result, __out size_t &resultLength) = 0;

  /**
   *  Encodes string from UTF-16LE to encoder's coding.
   *  Params:
   *    wstr - string to be decoded
   *    length - length of the string (in wchars)
   *    result - here encoder must place encoded string
   *    resultLength - length of the encoded string (in chars)
   */
  virtual void EncodeString(__in const wchar_t *wstr, __in size_t length, __inout PluginBuffer<char> &result, __out size_t &resultLength) = 0;

  /**
   *  Writes BOM to fOut
   *  Params:
   *    fOut - out stream
   *  Returns true if BOM was written
   */
  virtual bool WriteBOM(__inout FILE *fOut) = 0;

  /**
   *  Skips BOM in fIn
   *  Params:
   *    fIn - in stream
   *  Returns true if BOM was skipped (it means it is in fIn).
   */
  virtual bool SkipBOM(__inout FILE *fIn) = 0;

  virtual ~Encoder() {}
};


#endif // Encoder_h__