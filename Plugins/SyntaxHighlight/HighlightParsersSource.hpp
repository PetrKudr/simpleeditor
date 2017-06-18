#ifndef HighlightParsersSource_h__
#define HighlightParsersSource_h__

#include <map>

#include "HighlightParser.hpp"
#include "PropertyFileParser.hpp"


class HighlightParsersSource : public PropertyFileParser
{
private:

  enum Section 
  {
    parsers,
    undefined
  };


private:

  static const std::wstring FILE_NAME;

  static const std::wstring SECTION_PARSERS;


private:
  
  static HighlightParsersSource ourInstance;

  std::map<std::wstring, HighlightParser*> myParsers;

  std::wstring myRootPath;


  // for loading:

  Section section;


public:

  static HighlightParsersSource& getInstance() 
  {
    return ourInstance;
  }


public:

  const HighlightParser* GetParser(const std::wstring& name) const
  {
    std::map<std::wstring, HighlightParser*>::const_iterator iter = myParsers.find(name);
    return iter != myParsers.end() ? iter->second : 0;
  }

  void Clear()
  {
    for (std::map<std::wstring, HighlightParser*>::const_iterator iter = myParsers.begin(); iter != myParsers.end(); ++iter)
      delete iter->second;
    myParsers.clear();
  }

  const std::wstring& GetRootPath() const
  {
    return myRootPath;
  }

  void SetRootPath(const std::wstring &rootPath)
  {
    myRootPath = rootPath;
  }


protected:

  void HandleSection(const std::wstring &name)
  {
    if (name.compare(SECTION_PARSERS) == 0)
      section = parsers;
    else
      section = undefined;
  }

  void HandleSectionProperty(const std::wstring &name, const std::wstring &value)
  {
    switch (section)
    {
    case parsers:
      sectionParsers(name, value);
      break;
    }
  }

private:

  void sectionParsers(const std::wstring &name, const std::wstring &value) 
  {
    if (name.compare(L"Parser") == 0) 
    {
      HighlightParser *parser = new HighlightParser();
      if (parser->ReadFile((myRootPath + DIRECTORY_SEPARATOR + value).c_str())) 
      {
        if (!parser->GetName().empty())
        {
          myParsers.insert(std::pair<std::wstring, HighlightParser*>(parser->GetName(), parser));
          return;
        }
      }
      delete parser;
    }
  }

  HighlightParsersSource() : section(undefined) {}

  ~HighlightParsersSource()
  {
    Clear();
  }

};

#endif // HighlightParsersSource_h__