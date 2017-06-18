#include "notepad.hpp"


#include <vector>
#include <string>

/*
********************************************************************************************************
*                                          Built-in encoders                                           *
********************************************************************************************************
*/

struct DefaultEncoder : Encoder
{
  LPCWSTR GetCodingId() const
  {
    return CCodingManager::CODING_DEFAULT_ID;
  }

  LPCWSTR GetCodingName() const
  {
    return CCodingManager::CODING_DEFAULT_NAME;
  }

  int GetControlCharacterSize() const
  {
    return 1;
  }

  float DetectCoding(const char *buff, int size)
  {
    return 0;
  }

  void DecodeString(__in const char *str, __in size_t length, __inout PluginBuffer<wchar_t> &result, __out size_t &resultLength)
  {
    if (result.GetBufferSize() < length + 1)
      result.IncreaseBufferSize(length - result.GetBufferSize() + 1);

    resultLength = length;
    
    wchar_t *buffer = result.GetBuffer();
    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, str, -1, buffer, length + 1);
    buffer[length] = L'\0';
  }

  void EncodeString(__in const wchar_t *wstr, __in size_t length, __inout PluginBuffer<char> &result, __out size_t &resultLength)
  {
    size_t requiredSize = WideCharToMultiByte(CP_ACP, 0, wstr, -1, NULL, 0, NULL, NULL);

    if (result.GetBufferSize() < requiredSize)
      result.IncreaseBufferSize(requiredSize - result.GetBufferSize());

    resultLength = requiredSize - 1;

    char *buffer = result.GetBuffer();
    WideCharToMultiByte(CP_ACP, 0, wstr, -1, buffer, requiredSize, NULL, NULL);
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

struct Utf16LEEncoder : Encoder
{
  LPCWSTR GetCodingId() const
  {
    return CCodingManager::CODING_UTF_16_LE_ID;
  }

  LPCWSTR GetCodingName() const
  {
    return CCodingManager::CODING_UTF_16_LE_NAME;
  }

  int GetControlCharacterSize() const
  {
    return 2;
  }

  float DetectCoding(const char *buff, int size)
  {
    // try to read bom
    if (size >= 2 && (*(unsigned short*)buff) == 0xFEFF)
      return 1;


    // try to detect UTF16

    #define ZERO_ENOUGH 3

    int i, c = 0, n = min(max(size / 4, ZERO_ENOUGH), ZERO_ENOUGH);

    for (i = 0; i < size; i++)
    {
      if (0 == buff[i])
        c++;
    }

    return ((float)min(c, n)) / n;

    #undef ZERO_ENOUGH
  }

  void DecodeString(__in const char *str, __in size_t length, __inout PluginBuffer<wchar_t> &result, __out size_t &resultLength)
  {
    size_t wstrLength = length / 2;

    if (wstrLength + 1 > result.GetBufferSize())
      result.IncreaseBufferSize(wstrLength - result.GetBufferSize() + 1);

    resultLength = wstrLength;

    wchar_t *buffer = result.GetBuffer();
    wcsncpy(buffer, (wchar_t*)str, resultLength);
    buffer[resultLength] = L'\0';
  }

  void EncodeString(__in const wchar_t *wstr, __in size_t length, __inout PluginBuffer<char> &result, __out size_t &resultLength)
  {
    if (result.GetBufferSize() < length * 2)
      result.IncreaseBufferSize(length * 2 - result.GetBufferSize());

    resultLength = length * 2;

    char *buffer = result.GetBuffer();
    memcpy(buffer, wstr, resultLength); 
  }

  bool WriteBOM(__inout FILE *fOut)
  {
    unsigned short bom = 0xFEFF;
    fwrite(&bom, 2, 1, fOut);
    return true;
  }

  bool SkipBOM(__inout FILE *fIn)
  {
    unsigned short bom = 0xFEFF;
    unsigned short buff;

    if (fread(&buff, 2, 1, fIn) > 0 && buff == bom)
      return true;

    fseek(fIn, 0, SEEK_SET);
    return false;
  }
};

/*
********************************************************************************************************
*                                           Coding manager                                             *
********************************************************************************************************
*/

const wchar_t CCodingManager::LINE_BREAK[] = L"\r\n";

const wchar_t CCodingManager::CODING_UNKNOWN_NAME[] = L"Auto";

const wchar_t CCodingManager::CODING_UNKNOWN_ID[] = L"coding_unknown";

const wchar_t CCodingManager::CODING_DEFAULT_NAME[] = L"Ansi";

const wchar_t CCodingManager::CODING_DEFAULT_ID[] = L"ansi";

const wchar_t CCodingManager::CODING_UTF_16_LE_NAME[] = L"UTF-16LE";

const wchar_t CCodingManager::CODING_UTF_16_LE_ID[] = L"utf16le";


CCodingManager::CCodingManager()
{
  static DefaultEncoder ansiEncoder;
  static Utf16LEEncoder utf16leEncoder;

  RegisterEncoder(&ansiEncoder);
  RegisterEncoder(&utf16leEncoder);
}

const std::vector<Encoder*>& CCodingManager::GetEncoders() const
{
  return myEncoders;
}

Encoder* CCodingManager::GetEncoderById(LPCWSTR id) const
{
  if (id)
  {
    for (std::vector<Encoder*>::const_iterator iter = myEncoders.begin(); iter != myEncoders.end(); iter++)
    {
      if (wcscmp((*iter)->GetCodingId(), id) == 0)
        return *iter;
    }
  }
  return 0;
}

Encoder* CCodingManager::GetEncoderByName(LPCWSTR name) const
{
  for (std::vector<Encoder*>::const_iterator iter = myEncoders.begin(); iter != myEncoders.end(); iter++)
  {
    if (wcscmp((*iter)->GetCodingName(), name) == 0)
      return *iter;
  }
  return 0;
}

bool CCodingManager::RegisterEncoder(Encoder *encoder)
{
  if (!GetEncoderById(encoder->GetCodingId()))
  {
    myEncoders.push_back(encoder);
    return true;
  }
  return false;
}

bool CCodingManager::UnregisterEncoder(LPCWSTR id)
{
  for (std::vector<Encoder*>::const_iterator iter = myEncoders.begin(); iter != myEncoders.end(); iter++)
  {
    if (wcscmp((*iter)->GetCodingId(), id) == 0)
    {
      if (!checkEncoderOnUsage(*iter)) 
      {
        myEncoders.erase(iter);
        return true;
      }
    }
  }
  return false;
}

void CCodingManager::FillComboboxWithCodings(HWND hcombobox, LPCWSTR selectedId, bool addUnknownItem)
{
  int item = 0, selectedItem = -1;

  if (addUnknownItem)
  {
    SendMessage(hcombobox, CB_ADDSTRING, 0, (LPARAM)CCodingManager::CODING_UNKNOWN_NAME);
    SendMessage(hcombobox, CB_SETITEMDATA, 0, (LPARAM)CCodingManager::CODING_UNKNOWN_ID);
    ++item;
  }

  for (std::vector<Encoder*>::const_iterator iter = myEncoders.begin(); iter != myEncoders.end(); iter++)
  {
    if (selectedId && selectedItem < 0 && wcscmp((*iter)->GetCodingId(), selectedId) == 0)
      selectedItem = item;
    SendMessage(hcombobox, CB_ADDSTRING, 0, (LPARAM)(*iter)->GetCodingName());
    SendMessage(hcombobox, CB_SETITEMDATA, item++, (LPARAM)(*iter)->GetCodingId());
  }

  if (-1 == selectedItem)
    selectedItem = 0;

  SendMessage(hcombobox, CB_SETCURSEL, selectedItem, 0); 
}


bool CCodingManager::checkEncoderOnUsage(Encoder *encoder)
{
  CNotepad &Notepad = CNotepad::Instance();

  std::vector<int> docIds;
  Notepad.Documents.GetDocIds(docIds);

  LPCWSTR encoderCoding = encoder->GetCodingId();

  for (std::vector<int>::const_iterator iter = docIds.begin(); iter != docIds.end(); iter++)
  {
    if (wcscmp(Notepad.Documents.GetCoding(Notepad.Documents.GetIndexFromID(*iter)), encoderCoding) == 0)
      return true;
  }

  return false;
}


/************************************************************************/
/*     Functions and classes for access to encoders from files.c        */
/************************************************************************/

struct EncoderWrapper
{
  Encoder *encoder;
  VectorPluginBuffer<char> encodeBuffer;
  VectorPluginBuffer<wchar_t> decodeBuffer;

  EncoderWrapper(Encoder *_encoder) : encoder(_encoder) {}
};



wchar_t* _cdecl CCodingManager::DecodeFunction(__in void* opaqueCoding, __in const char *str, __in size_t length, __out size_t *resultLength)
{
  EncoderWrapper *wrapper = reinterpret_cast<EncoderWrapper*>(opaqueCoding);
  wrapper->encoder->DecodeString(str, length, wrapper->decodeBuffer, *resultLength);
  return wrapper->decodeBuffer.GetBuffer();
}

char* _cdecl CCodingManager::EncodeFunction(__in void* opaqueCoding, __in const wchar_t *wstr, __in size_t length, __out size_t *resultLength)
{
  EncoderWrapper *wrapper = reinterpret_cast<EncoderWrapper*>(opaqueCoding);
  wrapper->encoder->EncodeString(wstr, length, wrapper->encodeBuffer, *resultLength);
  return wrapper->encodeBuffer.GetBuffer();
}

char _cdecl CCodingManager::BomFunction(__in FILE *file, __in void* opaqueCoding, char isLoadProcess)
{
  if (isLoadProcess)
  {
    Encoder *encoder = reinterpret_cast<EncoderWrapper*>(opaqueCoding)->encoder;
    return encoder->SkipBOM(file) ? 1 : 0;
  }
  else 
  {
    Encoder *encoder = reinterpret_cast<EncoderWrapper*>(opaqueCoding)->encoder;
    return encoder->WriteBOM(file) ? 1 : 0;
  }
}

void* _cdecl CCodingManager::DeterminCodingFunction(__in FILE *file)
{
  CNotepad &Notepad = CNotepad::Instance();
  CCodingManager &CodingManager = Notepad.Documents.CodingManager;

  const std::vector<Encoder*> &encoders = CodingManager.GetEncoders();
  Encoder *selectedEncoder = 0;
  float maxProbability = 0;

  char buffer[512];
  size_t size = fread(buffer, 1, 512, file);
  fseek(file, 0, SEEK_SET);

  for (std::vector<Encoder*>::const_iterator iter = encoders.begin(); iter != encoders.end(); iter++)
  {
    float encoderPobability = (*iter)->DetectCoding(buffer, size);
    if (encoderPobability > maxProbability) 
    {
      selectedEncoder = *iter;
      maxProbability = encoderPobability;
    }
  }

  if (maxProbability < 0.4f)
  {
    selectedEncoder = CodingManager.GetEncoderById(Notepad.Options.DefaultCP().c_str());
    if (!selectedEncoder)
      selectedEncoder = CodingManager.GetEncoderById(CODING_DEFAULT_ID);
  }
  return selectedEncoder;
}

void* _cdecl CCodingManager::InitializeCodingFunction(__in void *opaqueCoding, __in char isLoadProcess, __out char *controlCharSize)
{
  *controlCharSize = reinterpret_cast<Encoder*>(opaqueCoding)->GetControlCharacterSize();
  return new EncoderWrapper(reinterpret_cast<Encoder*>(opaqueCoding));  
}

void* _cdecl CCodingManager::FinalizeCodingFunction(__in void *opaqueCoding, __in char isLoadProcess)
{
  Encoder *encoder = reinterpret_cast<EncoderWrapper*>(opaqueCoding)->encoder;
  delete reinterpret_cast<EncoderWrapper*>(opaqueCoding);
  return encoder;
}