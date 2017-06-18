#include <vector>
#include <queue>
#include <map>
#include <string>
#include <fstream>
#include <windows.h>
using namespace std;


#include "seditcontrol.h"
#include "Plugin.hpp"

#include "codingspack.h"
#include "encdec.h"       // old code for encode/decode strings from/to single byte codings
#include "adbytecp.h"     // third-party library for determining code pages


/****************************************************************************************************/
/*                                 WIN1251 Encoder                                                  */
/****************************************************************************************************/

struct WIN1251Encoder : Encoder
{
  LPCWSTR GetCodingId() const
  {
    return L"win1251";
  }

  LPCWSTR GetCodingName() const
  {
    return L"Win-1251";
  }

  int GetControlCharacterSize() const
  {
    return 1;  // size of termination symbol, \n and \r symbols
  }

  float DetectCoding(const char *buff, int size)
  {
    int result = m_def_code(reinterpret_cast<const unsigned char*>(buff), size, 10);
    return (1 == result ? 1.0f : 0);
  }

  void DecodeString(__in const char *str, __in size_t length, __inout PluginBuffer<wchar_t> &result, __out size_t &resultLength)
  {
    if (result.GetBufferSize() < length + 1)
      result.IncreaseBufferSize(length - result.GetBufferSize() + 1);

    resultLength = length;
    ::DecodeString(reinterpret_cast<const unsigned char*>(str), length, WIN1251, result.GetBuffer());
  }

  void EncodeString(__in const wchar_t *wstr, __in size_t length, __inout PluginBuffer<char> &result, __out size_t &resultLength)
  {
    if (result.GetBufferSize() < length + 1)
      result.IncreaseBufferSize(length - result.GetBufferSize() + 1);

    resultLength = length;
    ::EncodeString(wstr, length, WIN1251, reinterpret_cast<unsigned char*>(result.GetBuffer()));
  }

  bool WriteBOM(__inout FILE *fOut)
  {
    return false;
  }

  bool SkipBOM(__inout FILE *fIn)
  {
    return false;
  }
};

/****************************************************************************************************/
/*                                 KOI8R Encoder                                                    */
/****************************************************************************************************/

struct KOI8REncoder : Encoder
{
  LPCWSTR GetCodingId() const
  {
    return L"koi8r";
  }

  LPCWSTR GetCodingName() const
  {
    return L"Koi8R";
  }

  int GetControlCharacterSize() const
  {
    return 1;  // size of termination symbol, \n and \r symbols
  }

  float DetectCoding(const char *buff, int size)
  {
    int result = m_def_code(reinterpret_cast<const unsigned char*>(buff), size, 10);
    return (2 == result ? 1.0f : 0);
  }

  void DecodeString(__in const char *str, __in size_t length, __inout PluginBuffer<wchar_t> &result, __out size_t &resultLength)
  {
    if (result.GetBufferSize() < length + 1)
      result.IncreaseBufferSize(length - result.GetBufferSize() + 1);

    resultLength = length;
    ::DecodeString(reinterpret_cast<const unsigned char*>(str), length, KOI8R, result.GetBuffer());
  }

  void EncodeString(__in const wchar_t *wstr, __in size_t length, __inout PluginBuffer<char> &result, __out size_t &resultLength)
  {
    if (result.GetBufferSize() < length + 1)
      result.IncreaseBufferSize(length - result.GetBufferSize() + 1);

    resultLength = length;
    ::EncodeString(wstr, length, KOI8R, reinterpret_cast<unsigned char*>(result.GetBuffer()));
  }

  bool WriteBOM(__inout FILE *fOut)
  {
    return false;
  }

  bool SkipBOM(__inout FILE *fIn)
  {
    return false;
  }
};

/****************************************************************************************************/
/*                                 CP866 Encoder                                                    */
/****************************************************************************************************/

struct CP866Encoder : Encoder
{
  LPCWSTR GetCodingId() const
  {
    return L"cp866";
  }

  LPCWSTR GetCodingName() const
  {
    return L"CP866";
  }

  int GetControlCharacterSize() const
  {
    return 1;  // size of termination symbol, \n and \r symbols (for UTF-16 = 2)
  }

  float DetectCoding(const char *buff, int size)
  {
    int result = m_def_code(reinterpret_cast<const unsigned char*>(buff), size, 10);
    return (0 == result ? 1.0f : 0);
  }

  void DecodeString(__in const char *str, __in size_t length, __inout PluginBuffer<wchar_t> &result, __out size_t &resultLength)
  {
    if (result.GetBufferSize() < length + 1)
      result.IncreaseBufferSize(length - result.GetBufferSize() + 1);

    resultLength = length;
    ::DecodeString(reinterpret_cast<const unsigned char*>(str), length, CP866, result.GetBuffer());
  }

  void EncodeString(__in const wchar_t *wstr, __in size_t length, __inout PluginBuffer<char> &result, __out size_t &resultLength)
  {
    if (result.GetBufferSize() < length + 1)
      result.IncreaseBufferSize(length - result.GetBufferSize() + 1);

    resultLength = length;
    ::EncodeString(wstr, length, CP866, reinterpret_cast<unsigned char*>(result.GetBuffer()));
  }

  bool WriteBOM(__inout FILE *fOut)
  {
    return false;
  }

  bool SkipBOM(__inout FILE *fIn)
  {
    return false;
  }
};

/****************************************************************************************************/
/*                                 UTF-8 Encoder                                                    */
/****************************************************************************************************/

struct Utf8Encoder : Encoder
{
  LPCWSTR GetCodingId() const
  {
    return L"utf8";
  }

  LPCWSTR GetCodingName() const
  {
    return L"UTF-8";
  }

  int GetControlCharacterSize() const
  {
    return 1;  // size of termination symbol, \n and \r symbols (for UTF-16 = 2)
  }

  float DetectCoding(const char *buff, int size)
  {
    if (size >= 3)
    {
      unsigned char bom[3] = {0xEF, 0xBB, 0xBF};
      if (memcmp(buff, bom, 3) == 0)
        return 1;
    }


    int n = 0;

    for (int i = 0; i < size; i++)
    {
      unsigned char chr = toupper((unsigned char)buff[i]);
      if (0xD0 == chr || 0xD1 == chr)
        ++n;
    }

    if (n > 0)
    {
      float param = ((float)size)/n;

      if (param > 1.2 && param < 2.4) // in article was 2.2
        return param <= 2.2 ? param / 2.2f : (2.2f - (param - 2.2f)) / 2.2f;
    }

    return 0;
  }

  void DecodeString(__in const char *str, __in size_t length, __inout PluginBuffer<wchar_t> &result, __out size_t &resultLength)
  {
    if (result.GetBufferSize() < length + 1)
      result.IncreaseBufferSize(length - result.GetBufferSize() + 1);

    wchar_t *buffer = result.GetBuffer();
    MultiByteToWideChar(CP_UTF8, 0, str, -1, buffer, length + 1);    
    buffer[length] = L'\0'; // paranoia
    resultLength = wcslen(buffer);
  }

  void EncodeString(__in const wchar_t *wstr, __in size_t length, __inout PluginBuffer<char> &result, __out size_t &resultLength)
  {
    size_t requiredSize = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);

    if (result.GetBufferSize() < requiredSize)
      result.IncreaseBufferSize(requiredSize - result.GetBufferSize());

    resultLength = requiredSize - 1;

    char *buffer = result.GetBuffer();
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, buffer, requiredSize, NULL, NULL);
  }

  bool WriteBOM(__inout FILE *fOut)
  {
    unsigned char bom[3] = {0xEF, 0xBB, 0xBF};
    fwrite(&bom, 3, 1, fOut);
    return true;
  }

  bool SkipBOM(__inout FILE *fIn)
  {
    unsigned char bom[3] = {0xEF, 0xBB, 0xBF};
    unsigned char buff[3];

    if (fread(buff, 3, 1, fIn) > 0 && memcmp(buff, bom, 3) == 0)
      return true;

    fseek(fIn, 0, SEEK_SET);
    return false;
  }
};


/****************************************************************************************************/
/*                                 Plugin                                                           */
/****************************************************************************************************/


class CodingsPackPlugin : public Plugin 
{
private:

  static CodingsPackPlugin *ourPlugin;


private:

  PluginService *myService;

  Utf8Encoder myUtf8Encoder;

  WIN1251Encoder myWin1251Encoder;

  CP866Encoder myCP866Encoder;

  KOI8REncoder myKOI8REncoder;
  


public:

  static CodingsPackPlugin& GetInstance() 
  {
    if (!ourPlugin) 
      ourPlugin = new CodingsPackPlugin;
    return *ourPlugin;
  }

  static void ReleaseInstance()
  {
    if (ourPlugin)
    {
      delete ourPlugin;
      ourPlugin = 0;
    }
  }


public:

  CodingsPackPlugin() {}  

  LPCWSTR GetGUID() const
  {
    return L"a151378e-cd9e-460d-88db-0c7fc9de31c3";
  }

  int GetVersion() const
  {
    return 1;
  }

  LPCWSTR GetName() const 
  {
    return L"CodingsPack";
  }

  int GetPriority() const 
  {
    return 0x7f;
  }

  bool Init(HMODULE hModule, PluginService *service)
  {
    myService = service;
    myService->RegisterEncoder(&myUtf8Encoder);
    myService->RegisterEncoder(&myWin1251Encoder);
    myService->RegisterEncoder(&myCP866Encoder);
    myService->RegisterEncoder(&myKOI8REncoder);
    return true;    
  }

  void ExecuteCommand(__in LPCWSTR name) { /* not in use */ }

  void ExecuteFunction(__in LPCWSTR name, __inout void *param) { /* not in use */ }

  void ExecuteMenuCommand(__in int itemId) { /* not in use */ }

  int GetSupportedCommandsNumber()
  {
    return 0;
  }

  const PluginCommand* GetSupportedCommands()
  {
    return 0;
  }

  int GetSupportedFunctionsNumber()
  {
    return 0;
  }

  const LPCWSTR* GetSupportedFunctions()
  {
    return 0;
  }

  HMENU GetMenuHandle(int firstItemId)
  {
    return 0;
  }

  int GetMenuItemsNumber()
  {
    return 0;
  }

  void ProcessNotification(const PluginNotification *notification) { /* not in use */ } 

  ~CodingsPackPlugin()
  {
    myService->UnregisterEncoder(myUtf8Encoder.GetCodingId());
    myService->UnregisterEncoder(myWin1251Encoder.GetCodingId());
    myService->UnregisterEncoder(myCP866Encoder.GetCodingId());
    myService->UnregisterEncoder(myKOI8REncoder.GetCodingId());
  }


private:

  bool handleError()
  {
    if (myService->GetLastError() != NO_ERROR)
    {
      VectorPluginBuffer<wchar_t> buffer;
      myService->GetLastErrorMessage(&buffer);
      myService->ShowError(buffer.GetBuffer());
      return true;
    }
    return false;
  }  
};

/****************************************************************************************************/
/*                                 External functions                                               */
/****************************************************************************************************/

extern "C"
Plugin* __stdcall SECreatePlugin()
{
  return &CodingsPackPlugin::GetInstance();
}

extern "C"
void __stdcall SEDeletePlugin(Plugin *plugin)
{
  CodingsPackPlugin::ReleaseInstance();
}

/****************************************************************************************************/
/*                                 Static initialization                                            */
/****************************************************************************************************/

CodingsPackPlugin* CodingsPackPlugin::ourPlugin(0);