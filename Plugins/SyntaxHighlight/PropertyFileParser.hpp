#ifndef PropertyFileParser_h__
#define PropertyFileParser_h__

#include <fstream>
#include <string>

#ifndef _WIN32_WCE
  #include "codecvt.hpp"
#endif


#define DIRECTORY_SEPARATOR L"\\"


class PropertyFileParser 
{
private:

  static const int END_OF_FILE        = 0;
  static const int ENTITY_SKIP        = 1;
  static const int ENTITY_SECTION     = 2;
  static const int ENTITY_PARAMETER   = 3;


public:

  bool ReadFile(const wchar_t *path)
  {
    int section;
    std::wifstream fIn(path, std::ios_base::binary);

    if (fIn.is_open())
    {
#ifndef _WIN32_WCE
      fIn.imbue(std::locale(fIn.getloc(), new ucs2_conversion));
#endif

      if (0xfeff != fIn.get())
      {
        fIn.close();
        return false;
      }

      std::wstring wstr;
      std::wstring name;
      std::wstring value;

      while ((section = readString(fIn, wstr)) != END_OF_FILE)
      {
        switch (section)
        {
        case ENTITY_SECTION:
          if (getSectionName(wstr, name))
            HandleSection(name);
          break;

        case ENTITY_PARAMETER:
          if (getSectionParam(wstr, name, value))
            HandleSectionProperty(name, value);
          break;
        }
      }
      fIn.close();

      HandleEndOfFile();

      return true;
    }
    return false;
  }


protected:

  virtual void HandleSection(const std::wstring &section) = 0;

  virtual void HandleSectionProperty(const std::wstring &name, const std::wstring &value) = 0;

  virtual void HandleEndOfFile() {}

  // function gets integer param from string
  int parseInt(const std::wstring &str, int radix)
  {
    wchar_t *endPtr;
    return wcstol(str.c_str(), &endPtr, radix);
  }


private:

  // function gets string param from string
  bool getSectionParam(const std::wstring &str, std::wstring &name, std::wstring &param)
  {
    size_t paramNameStart = str.find_first_not_of(L"\t ", 0, 2);
    if (paramNameStart == str.npos) 
      return false;   

    size_t paramNameEnd = str.find_first_of(L"\t =", paramNameStart, 3);
    if (paramNameEnd == str.npos)
      return false;

    size_t paramStart = str.find(L'=', paramNameEnd);
    if (paramStart == str.npos)
      return false;

    bool hasParamNamePrefix = (str[paramNameStart] == L'_');  // skip spaces or not

    ++paramStart;
    if (hasParamNamePrefix && (paramStart = str.find_first_not_of(L"\t ", paramStart)) == str.npos)
      return false;

    //if (paramStart >= str.size())
      //return false;

    size_t paramEnd = str.find_first_of(L"\r\n", paramStart);

    name = hasParamNamePrefix ? str.substr(paramNameStart + 1, paramNameEnd - paramNameStart - 1) : str.substr(paramNameStart, paramNameEnd - paramNameStart);
    param = (paramEnd == str.npos) ? str.substr(paramStart) : str.substr(paramStart, paramEnd - paramStart);

    return true;
  }

  bool getSectionName(const std::wstring &str, std::wstring &param) 
  {
    size_t sectionStart, sectionEnd;
    sectionStart = str.find_first_of('[');
    sectionEnd = str.find_first_of(']', sectionStart);

    if (sectionStart != str.npos && sectionEnd != str.npos) 
    {
      param.assign(str, sectionStart + 1, sectionEnd - sectionStart - 1);
      return true;
    }

    return false;
  }

  // function reads string and returns section id
  int readString(std::wifstream &fIn, std::wstring &str)
  {
    if (fIn.eof())
      return END_OF_FILE;
    getline(fIn, str);

    size_t stringStart = str.find_first_not_of(L"\t ", 0, 2);

    size_t commentStart = str.find(L'#', 0);
    if (stringStart == str.npos || (commentStart != str.npos && stringStart == commentStart))
      return ENTITY_SKIP;  // skip this string if it contains only tabs and spaces or it is comment

    size_t sectionStart, sectionEnd;
    sectionStart = str.find_first_of('[');
    sectionEnd = str.find_first_of(']', sectionStart);

    if (sectionStart != str.npos && sectionEnd != str.npos && stringStart == sectionStart) 
      return ENTITY_SECTION;

    return ENTITY_PARAMETER; // it's not a section => it's parameter
  }
};

#endif // PropertyFileParser_h__
