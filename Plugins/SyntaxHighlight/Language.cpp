#include "HighlightParsersSource.hpp"
#include "TagWarehousesSource.hpp"
#include "MessageCollector.hpp"
#include "Language.hpp"


bool Language::Init()
{
  myCachedTagWarehouse = TagWarehousesSource::GetInstance().GetTagWarehouse(myTagWarehouseName);
  myCachedHighlightParser = HighlightParsersSource::getInstance().GetParser(myHighlightParserName);

  if (myCachedTagWarehouse == 0)
  {
    MessageCollector::GetInstance().AddMessage(error, std::wstring(L"Tag warehouse '").append(myTagWarehouseName).append(L"' not found!").c_str());
    return false;
  }
  if (myCachedHighlightParser == 0)
  {
    MessageCollector::GetInstance().AddMessage(error, std::wstring(L"Parser '").append(myHighlightParserName).append(L"' not found!").c_str());
    return false;
  }
  return true;
}

void Language::SetName(const std::wstring &name)
{
  myName = name;
}

const std::wstring& Language::GetName() const
{
  return myName;
}

void Language::SetHighlighParserName(const std::wstring &parserName)
{
  myHighlightParserName = parserName;
}

const HighlightParser& Language::GetHighlightParser() const 
{
  return *myCachedHighlightParser;
}

void Language::SetTagWarehouseName(const std::wstring &warehouseName)
{
  myTagWarehouseName = warehouseName;
}

const TagWarehouse& Language::GetTagWarehouse() const
{
  return *myCachedTagWarehouse;
}

void Language::SetExtensions(const std::vector<std::wstring> &extensions)
{
  myExtensions = extensions;
}

const std::vector<std::wstring>& Language::GetExtensions() const
{
  return myExtensions;
}

void Language::SetExported(bool exported)
{
  myExported = exported;
}

bool Language::IsExported() const
{
  return myExported;
}

void Language::SetExportName(const std::wstring &exportName)
{
  myExportName = exportName;
}

const std::wstring& Language::GetExportName() const
{
  return myExportName;
}