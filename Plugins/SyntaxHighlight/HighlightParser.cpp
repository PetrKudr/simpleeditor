#include "HighlightParser.hpp"

const std::wstring HighlightParser::SECTION_DEFAULT_WORDGROUP_NAME(L"DEFAULT");
const std::wstring HighlightParser::SECTION_WORDGROUP_NAME(L"WORDGROUP");


void HighlightParser::sectionWordgroup(const std::wstring &name, const std::wstring &value) 
{
  if (name.compare(L"Word") == 0)
    myAhoWords.AddString(value.c_str(), value.length(), myWordGroups.size() - 1);
  else if (name.compare(L"TextColor") == 0) 
    myWordGroups[myWordGroups.size() - 1]->textColor = parseInt(value, 16);
  else if (name.compare(L"BgColor") == 0) 
    myWordGroups[myWordGroups.size() - 1]->bgColor = parseInt(value, 16);
  else if (name.compare(L"IsLeftDelimitersMode") == 0) 
    myWordGroups[myWordGroups.size() - 1]->leftDelimitersMode = parseInt(value, 10) != 0; 
  else if (name.compare(L"IsRightDelimitersMode") == 0) 
    myWordGroups[myWordGroups.size() - 1]->rightDelimitersMode = parseInt(value, 10) != 0; 
  else if (name.compare(L"IsLineBreakDelimiter") == 0) 
  {
    if (parseInt(value, 10) != 0) 
    {
      myWordGroups[myWordGroups.size() - 1]->delimiters.insert('\r');
      myWordGroups[myWordGroups.size() - 1]->delimiters.insert('\n');
    }
  }
  else if (name.compare(L"IsLineBreakLeftDelimiter") == 0) 
  {
    if (parseInt(value, 10) != 0) 
    {
      myWordGroups[myWordGroups.size() - 1]->leftDelimiters.insert('\r');
      myWordGroups[myWordGroups.size() - 1]->leftDelimiters.insert('\n');
    }
  }
  else if (name.compare(L"IsLineBreakRightDelimiter") == 0) 
  {
    if (parseInt(value, 10) != 0) 
    {
      myWordGroups[myWordGroups.size() - 1]->rightDelimiters.insert('\r');
      myWordGroups[myWordGroups.size() - 1]->rightDelimiters.insert('\n');
    }
  }
  else if (name.compare(L"Delimiters") == 0) 
  {
    for (size_t i = 0; i < value.length(); ++i)
      myWordGroups[myWordGroups.size() - 1]->delimiters.insert(value[i]);
  }
  else if (name.compare(L"LeftDelimiters") == 0) 
  {
    for (size_t i = 0; i < value.length(); ++i)
      myWordGroups[myWordGroups.size() - 1]->leftDelimiters.insert(value[i]);
  }
  else if (name.compare(L"RightDelimiters") == 0) 
  {
    for (size_t i = 0; i < value.length(); ++i)
      myWordGroups[myWordGroups.size() - 1]->rightDelimiters.insert(value[i]);
  }
}

void HighlightParser::sectionDefaultWordGroup(const std::wstring &name, const std::wstring &value) 
{
  if (name.compare(L"Name") == 0)
    myName = value;
  else if (name.compare(L"TextColor") == 0) 
    myWordGroups[DEFAULT_WORD_GROUP]->textColor = parseInt(value, 16);
  else if (name.compare(L"BgColor") == 0) 
    myWordGroups[DEFAULT_WORD_GROUP]->bgColor = parseInt(value, 16);
}
