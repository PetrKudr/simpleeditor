#ifndef codingmanager_h__
#define codingmanager_h__


#include <windows.h>

#include <vector>

#include "Encoder.hpp"


class CCodingManager
{
public:

  static const wchar_t LINE_BREAK[];

  static const wchar_t CODING_UNKNOWN_NAME[];

  static const wchar_t CODING_UNKNOWN_ID[];

  static const wchar_t CODING_DEFAULT_NAME[];

  static const wchar_t CODING_DEFAULT_ID[];

  static const wchar_t CODING_UTF_16_LE_NAME[];

  static const wchar_t CODING_UTF_16_LE_ID[];


private:

  std::vector<Encoder*> myEncoders;


public:

  static wchar_t* _cdecl DecodeFunction(__in void* opaqueCoding, __in const char *str, __in size_t length, __out size_t *resultLength);

  static char* _cdecl EncodeFunction(__in void* opaqueCoding, __in const wchar_t *wstr, __in size_t length, __out size_t *resultLength);

  static char _cdecl BomFunction(__in FILE *file, __in void* opaqueCoding, char isLoadProcess);

  static void* _cdecl DeterminCodingFunction(__in FILE *file);

  static void* _cdecl InitializeCodingFunction(__in void *opaqueCoding, __in char isLoadProcess, __out char *controlCharSize);

  static void* _cdecl FinalizeCodingFunction(__in void *opaqueCoding, __in char isLoadProcess);


public:

  CCodingManager();

  const std::wstring& GetDefaultCodingId() const;

  const std::wstring& GetDefaultCodingName() const;

  const std::vector<Encoder*>& GetEncoders() const;

  Encoder* GetEncoderById(LPCWSTR id) const;

  Encoder* GetEncoderByName(LPCWSTR name) const;
  
  bool RegisterEncoder(Encoder *encoder);

  bool UnregisterEncoder(LPCWSTR id);

  void FillComboboxWithCodings(HWND hcombobox, LPCWSTR selectedId, bool addUnknownItem);


private:

  bool checkEncoderOnUsage(Encoder *encoder);

};

#endif // codingmanager_h__