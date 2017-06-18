#ifndef Language_h__
#define Language_h__

#include <string>
#include <vector>

#include "HighlightParser.hpp"
#include "TagWarehouse.hpp"


class Language 
{
private:

  // name of the language (id)
  std::wstring myName;

  // highlighter for text
  std::wstring myHighlightParserName;

  // container with all tags descriptions
  std::wstring myTagWarehouseName;

  // file extensions
  std::vector<std::wstring> myExtensions;

  // true if this is an exported language
  bool myExported;

  // export name
  std::wstring myExportName;


  // cached instances of parser and tags warehouse

  const HighlightParser *myCachedHighlightParser;

  const TagWarehouse *myCachedTagWarehouse;


public:

  Language() : myExported(false) {}

  //Language(const std::wstring &name, const std::wstring &tagWarehouseName, const std::wstring &highlightParserName) 
  //  : myName(name), myTagWarehouseName(tagWarehouseName), myHighlightParserName(highlightParserName), myExported(false) {}


  bool Init();

  void SetName(const std::wstring &name);
    
  const std::wstring& GetName() const;

  void SetHighlighParserName(const std::wstring &parserName);

  const HighlightParser& GetHighlightParser() const;

  void SetTagWarehouseName(const std::wstring &warehouseName);

  const TagWarehouse& GetTagWarehouse() const;

  void SetExtensions(const std::vector<std::wstring> &extensions);

  const std::vector<std::wstring>& GetExtensions() const;

  void SetExported(bool exported);

  bool IsExported() const;

  void SetExportName(const std::wstring &exportName);

  const std::wstring& GetExportName() const;
};

#endif // Language_h__
