#include "MessageCollector.hpp"
#include "LanguagesSource.hpp"
#include "TagWarehouse.hpp"

const std::wstring TagWarehouse::SECTION_COMMON_NAME(L"COMMON");
const std::wstring TagWarehouse::SECTION_TAG_NAME(L"TAG");
const std::wstring TagWarehouse::SECTION_TAG_HEADER_PAIR_NAME(L"HEADER_PAIR");
const std::wstring TagWarehouse::SECTION_TAG_BODY_PAIR_NAME(L"BODY_PAIR");


const std::wstring TagWarehouse::TAG_EMBEDDED(L"");

const std::wstring TagWarehouse::TAG_CR_NAME(L"$tag_cr$");
const std::wstring TagWarehouse::TAG_CR(L"\r");

const std::wstring TagWarehouse::TAG_LF_NAME(L"$tag_lf$");
const std::wstring TagWarehouse::TAG_LF(L"\n");

const std::wstring TagWarehouse::TAG_CRLF_NAME(L"$tag_crlf$");
const std::wstring TagWarehouse::TAG_CRLF(L"\r\n");

const std::wstring TagWarehouse::LINE_BREAK_NAME(L"$line_break$");



TagWarehouse::~TagWarehouse()
{
  Clear();
}


bool TagWarehouse::Init()
{
  myTagsParser.Prepare(); 

  for (std::vector<TagDescription*>::const_iterator iter = myTags.begin(); iter != myTags.end(); ++iter)
  {
    if ((*iter)->headerLanguage && !LanguagesSource::GetInstance().GetLanguage(*(*iter)->headerLanguage)) 
    {
      MessageCollector::GetInstance().AddMessage(error, std::wstring(L"Language '").append(*(*iter)->headerLanguage).append(L"' not found!").c_str());
      return false;
    }
    if ((*iter)->bodyLanguage && !LanguagesSource::GetInstance().GetLanguage(*(*iter)->bodyLanguage)) 
    {
      MessageCollector::GetInstance().AddMessage(error, std::wstring(L"Language '").append(*(*iter)->bodyLanguage).append(L"' not found!").c_str());
      return false;
    }
  }

  return true;
}

void TagWarehouse::SetName(const std::wstring &name) 
{
  myName = name;
}

const std::wstring& TagWarehouse::GetName() 
{
  return myName;
}

void TagWarehouse::AddTagDescription(TagDescription *tagDescription) 
{
  if (!GetTagDescription(tagDescription->sequence))
  {
    myTags.push_back(tagDescription);
    //myTagsMap.insert(std::make_pair(*tagDescription->sequence, tagDescription));
    myTagsParser.AddString(tagDescription->sequence->c_str(), tagDescription->sequence->size(), myCounter++);

    if (tagDescription->CanBeOpen()) 
    {
      std::vector<wchar_t> emptyVector;
      AddHeaderTagPair(tagDescription, GetTagDescription(&TAG_EMBEDDED), false, emptyVector);
      AddBodyTagPair(tagDescription, GetTagDescription(&TAG_EMBEDDED), false, emptyVector);
    }
  }

  /*if (tagDescription->IsComplex() && !GetTagDescription(tagDescription->suffix)) 
  {
    myTags.push_back(tagDescription);
    myTagsParser.AddString(tagDescription->suffix->c_str(), tagDescription->sequence->size(), myCounter++);
  }*/
}

void TagWarehouse::AddHeaderTagPair(TagDescription *openTag, TagDescription *closeTag, bool isCorrectPair, const std::vector<wchar_t> &escapeCharacters) const 
{
  if (!openTag->CanBeOpen()) 
    throw std::logic_error("Wrong tag types");

  openTag->headerPairs.push_back(TagPair(closeTag, isCorrectPair, escapeCharacters));
}

void TagWarehouse::AddBodyTagPair(TagDescription *openTag, TagDescription *closeTag, bool isCorrectPair, const std::vector<wchar_t> &escapeCharacters) const 
{
  if (!openTag->CanBeOpen()) 
    throw std::logic_error("Wrong tag types");

  openTag->bodyPairs.push_back(TagPair(closeTag, isCorrectPair, escapeCharacters));
}

TagDescription* TagWarehouse::GetTagDescription(int index) const
{
  return myTags[index];
  /*int i = 0;
  for (std::list<TagDescription*>::const_iterator iter = myTags.begin(); iter != myTags.end(); ++iter)
  {
    if (i == index)
      return *iter;
    ++i;
  }
  return 0;*/
}

TagDescription* TagWarehouse::GetTagDescription(const std::wstring *name) const
{
  //stdext::hash_map<std::wstring, TagDescription*>::const_iterator iter = myTagsMap.find(*name);
  //return iter != myTagsMap.end() ? iter->second : 0;
  for (std::vector<TagDescription*>::const_iterator iter = myTags.begin(); iter != myTags.end(); ++iter) 
  {
    if (0 == (*iter)->sequence->compare(*name))
      return *iter;
  }
  return 0;
}

TagDescription* TagWarehouse::GetTagDescription(const wchar_t *name, int length) const
{
  //stdext::hash_map<std::wstring, TagDescription*>::const_iterator iter = myTagsMap.find(std::wstring(name, length));
  //return iter != myTagsMap.end() ? iter->second : 0;
  for (std::vector<TagDescription*>::const_iterator iter = myTags.begin(); iter != myTags.end(); ++iter) 
  {
    if (0 == (*iter)->sequence->compare(0, (*iter)->sequence->size(), name, length))
      return *iter;
  }
  return 0;
}

const AhoSearch& TagWarehouse::GetTagsParser() const {
  return myTagsParser;
}

void TagWarehouse::Clear()
{
  for (std::vector<TagDescription*>::const_iterator iter = myTags.begin(); iter != myTags.end(); ++iter)
    delete (*iter);
  myTags.clear();
}


void TagWarehouse::sectionCommon(const std::wstring &propertyName, const std::wstring &propertyValue)
{
  if (propertyName.compare(L"Name") == 0)
  {
    SetName(propertyValue);
  }
  else if (propertyName.compare(L"LineBreak") == 0)
  {
    if (myLineBreak.empty())
    {
      myLineBreak = determineLineBreak(propertyValue);
      if (!myLineBreak.empty())
        AddTagDescription(new TagDescription(myLineBreak));
    }
  }
}

void TagWarehouse::sectionTag(const std::wstring &propertyName, const std::wstring &propertyValue) 
{
  if (propertyName.compare(L"TagName") == 0) 
  {
    myTagDescription = new TagDescription(propertyValue);
  } 
  else if (propertyName.compare(L"TagBodyLanguage") == 0)
  {
    if (0 != myTagDescription)
      myTagDescription->bodyLanguage = new std::wstring(propertyValue);
  }
  else if (propertyName.compare(L"TagHeaderLanguage") == 0)
  {
    if (0 != myTagDescription)
      myTagDescription->headerLanguage = new std::wstring(propertyValue);
  }
  else if (propertyName.compare(L"IsLeftDelimitersMode") == 0) 
  {
    myTagDescription->isLeftDelimitersMode = parseInt(propertyValue, 10) != 0; 
  }
  else if (propertyName.compare(L"IsRightDelimitersMode") == 0) 
  {
    myTagDescription->isRightDelimitersMode = parseInt(propertyValue, 10) != 0; 
  }
  else if (propertyName.compare(L"IsLineBreakDelimiter") == 0) 
  {
    if (parseInt(propertyValue, 10) != 0) 
    {
      myTagDescription->delimiters.insert('\r');
      myTagDescription->delimiters.insert('\n');
    }
  }
  else if (propertyName.compare(L"IsLineBreakLeftDelimiter") == 0) 
  {
    if (parseInt(propertyValue, 10) != 0) 
    {
      myTagDescription->leftDelimiters.insert('\r');
      myTagDescription->leftDelimiters.insert('\n');
    }
  }
  else if (propertyName.compare(L"IsLineBreakRightDelimiter") == 0) 
  {
    if (parseInt(propertyValue, 10) != 0) 
    {
      myTagDescription->rightDelimiters.insert('\r');
      myTagDescription->rightDelimiters.insert('\n');
    }
  }
  else if (propertyName.compare(L"Delimiters") == 0) 
  {
    for (size_t i = 0; i < propertyValue.length(); ++i)
      myTagDescription->delimiters.insert(propertyValue[i]);
  }
  else if (propertyName.compare(L"LeftDelimiters") == 0) 
  {
    for (size_t i = 0; i < propertyValue.length(); ++i)
      myTagDescription->leftDelimiters.insert(propertyValue[i]);
  }
  else if (propertyName.compare(L"RightDelimiters") == 0) 
  {
    for (size_t i = 0; i < propertyValue.length(); ++i)
      myTagDescription->rightDelimiters.insert(propertyValue[i]);
  }
}

void TagWarehouse::sectionTagBodyPair(const std::wstring &propertyName, const std::wstring &propertyValue) {
  if (propertyName.compare(L"OpenTag") == 0) 
    openTag = propertyValue;
  else if (propertyName.compare(L"CloseTag") == 0)
  {
    if (propertyValue.compare(LINE_BREAK_NAME) == 0)
      closeTag = myLineBreak;
    else
      closeTag = propertyValue;
  }
  else if (propertyName.compare(L"Correct") == 0)
    correctPair = (parseInt(propertyValue, 10) != 0);
  else if (propertyName.compare(L"EscapeCharacters") == 0)
  {
    for (size_t i = 0; i < propertyValue.length(); i++)
      escapeCharacters.push_back(propertyValue[i]);
  }
}

void TagWarehouse::sectionTagHeaderPair(const std::wstring &propertyName, const std::wstring &propertyValue) {
  if (propertyName.compare(L"OpenTag") == 0) 
    openTag = propertyValue;
  else if (propertyName.compare(L"CloseTag") == 0)
  {
    if (propertyValue.compare(LINE_BREAK_NAME) == 0)
      closeTag = myLineBreak;
    else
      closeTag = propertyValue;
  }
  else if (propertyName.compare(L"Correct") == 0)
    correctPair = (parseInt(propertyValue, 10) != 0);
  else if (propertyName.compare(L"EscapeCharacters") == 0)
  {
    for (size_t i = 0; i < propertyValue.length(); i++)
      escapeCharacters.push_back(propertyValue[i]);
  }
}

const std::wstring& TagWarehouse::determineLineBreak(const std::wstring &propertyValue)
{
  static const std::wstring emptyResult(L"");

  if (propertyValue.compare(TAG_CR_NAME) == 0)
    return TAG_CR;
  else if (propertyValue.compare(TAG_LF_NAME) == 0)
    return TAG_LF;
  else if (propertyValue.compare(TAG_CRLF_NAME) == 0)
    return TAG_CRLF;

  return emptyResult;
}

void TagWarehouse::putTagDescription() 
{
  if (0 != myTagDescription) {
    AddTagDescription(myTagDescription);
    myTagDescription = 0;
  }
}

void TagWarehouse::putHeaderTagPair() 
{
  if (!openTag.empty() && !closeTag.empty()) 
  {
    TagDescription *openTagDescription = GetTagDescription(&openTag);
    TagDescription *closeTagDescription = GetTagDescription(&closeTag);

    if (0 != openTagDescription && 0 != closeTagDescription) 
      AddHeaderTagPair(openTagDescription, closeTagDescription, correctPair, escapeCharacters);

    escapeCharacters.clear();
    openTag.clear();
    closeTag.clear();
  }
}

void TagWarehouse::putBodyTagPair() 
{
  if (!openTag.empty() && !closeTag.empty()) 
  {
    TagDescription *openTagDescription = GetTagDescription(&openTag);
    TagDescription *closeTagDescription = GetTagDescription(&closeTag);

    if (0 != openTagDescription && 0 != closeTagDescription) 
      AddBodyTagPair(openTagDescription, closeTagDescription, correctPair, escapeCharacters);

    escapeCharacters.clear();
    openTag.clear();
    closeTag.clear();
  }
}

void TagWarehouse::handleEndOfSection()
{
  switch (section)
  {
  case tag:
    putTagDescription();
    break;

  case bodyPair:
    putBodyTagPair();
    break;

  case headerPair:
    putHeaderTagPair();
    break;
  }
}