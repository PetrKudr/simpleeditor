#ifndef Options_h__
#define Options_h__

#include <Windows.h>

#include "PropertyFileParser.hpp"


class Options : protected PropertyFileParser
{
private:

  enum Section 
  {
    common,
    undefined
  };


private:

  static const std::wstring FILE_NAME;

  static const std::wstring SECTION_COMMON;


private:

  // parameters

  std::wstring myRootPath;

  bool myOptionsChanged;


  // options

  bool myDetectLanguageByExtension;

  COLORREF *myCaretLineTextColor;

  COLORREF *myCaretLineBackgroundColor;

  // for loading:

  Section section;



public:

  Options(const std::wstring rootPath) : myRootPath(rootPath), myOptionsChanged(false), section(undefined) 
  {
    myDetectLanguageByExtension = true;
    myCaretLineTextColor = 0;
    myCaretLineBackgroundColor = 0;
  }

  ~Options() 
  { 
    if (myCaretLineTextColor)
      delete myCaretLineTextColor;

    if (myCaretLineBackgroundColor)
      delete myCaretLineBackgroundColor;
  }

  void ReadOptions();

  void SaveOptions();

  // Getters and setters

  bool IsDetectLanguageByExtension() const
  {
    return myDetectLanguageByExtension;
  }

  void SetDetectLanguageByExtension(bool value)
  {
    if (myDetectLanguageByExtension != value)
    {
      myOptionsChanged = true;
      myDetectLanguageByExtension = value;
    }
  }

  const COLORREF* GetCaretLineTextColor() const
  {
    return myCaretLineTextColor;
  }

  const COLORREF* GetCaretLineBackgroundColor() const
  {
    return myCaretLineBackgroundColor;
  }

protected:

  void HandleSection(const std::wstring &name)
  {
    if (name.compare(SECTION_COMMON) == 0)
      section = common;
    else
      section = undefined;
  }

  void HandleSectionProperty(const std::wstring &name, const std::wstring &value)
  {
    switch (section)
    {
    case common:
      sectionCommon(name, value);
      break;
    }
  }

  void HandleEndOfFile() 
  {
    // do nothing
  }


private:

  void sectionCommon(const std::wstring &propertyName, const std::wstring &propertyValue) 
  {
    if (propertyName.compare(L"DetectLanguageByExtension") == 0) 
      myDetectLanguageByExtension = propertyValue.compare(L"true") == 0;    
    else if (propertyName.compare(L"CaretLineTextColor") == 0) 
    {
      if (!myCaretLineTextColor)
        myCaretLineTextColor = new COLORREF;
      *myCaretLineTextColor = parseInt(propertyValue, 16);
    }
    else if (propertyName.compare(L"CaretLineBackgroundColor") == 0) 
    {
      if (!myCaretLineBackgroundColor)
        myCaretLineBackgroundColor = new COLORREF;
      *myCaretLineBackgroundColor = parseInt(propertyValue, 16);
    }
  }
};


#endif // Options_h__