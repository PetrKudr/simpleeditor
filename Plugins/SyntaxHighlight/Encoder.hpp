#ifndef Encoder_h__
#define Encoder_h__

#include <windows.h>
#include <cstdio>

#include "PluginTypes.hpp"


#define ENCODER_MAX_CODING_ID_LENGTH 32


struct Encoder
{
  virtual LPCWSTR GetCodingId() const = 0;

  virtual LPCWSTR GetCodingName() const = 0;

  virtual int GetControlCharacterSize() = 0;

  virtual float DetectCoding(const char *buff, int size) = 0;

  virtual void DecodeString(__in const char *str, __in size_t length, __inout PluginBuffer<wchar_t> &result, __out size_t &resultLength) = 0;

  virtual void EncodeString(__in const wchar_t *wstr, __in size_t length, __inout PluginBuffer<char> &result, __out size_t &resultLength) = 0;

  virtual bool WriteBOM(__inout FILE *fOut) = 0;

  virtual bool SkipBOM(__inout FILE *fIn) = 0;

  virtual ~Encoder() {}
};


#endif // Encoder_h__