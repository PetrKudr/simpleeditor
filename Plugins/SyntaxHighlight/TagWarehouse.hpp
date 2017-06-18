#ifndef TagWarehouse_h__
#define TagWarehouse_h__

#include <string>
#include <vector>

#include "PropertyFileParser.hpp"
#include "AhoSearch.hpp"
#include "TagDescription.hpp"


class TagWarehouse : public PropertyFileParser 
{
public:

  static const std::wstring TAG_EMBEDDED;

  static const std::wstring TAG_CR_NAME;
  static const std::wstring TAG_CR;

  static const std::wstring TAG_LF_NAME;
  static const std::wstring TAG_LF;

  static const std::wstring TAG_CRLF_NAME;
  static const std::wstring TAG_CRLF;

  static const std::wstring LINE_BREAK_NAME;


private:

  enum Section 
  {
    undefined,
    common,
    tag,
    bodyPair,
    headerPair
  };


private:

  static const std::wstring SECTION_COMMON_NAME;

  static const std::wstring SECTION_TAG_NAME;

  static const std::wstring SECTION_TAG_HEADER_PAIR_NAME;

  static const std::wstring SECTION_TAG_BODY_PAIR_NAME;


private:

  int myCounter;

  std::wstring myName;

  std::vector<TagDescription*> myTags;

  //stdext::hash_map<std::wstring, TagDescription*> myTagsMap;

  AhoSearch myTagsParser;

  // for loading:

  TagDescription *myTagDescription;

  std::wstring openTag, closeTag, myLineBreak;

  bool correctPair;

  std::vector<wchar_t> escapeCharacters;

  Section section;


public:

  static bool IsTagDescriptionEmbedded(const TagDescription &description)
  {
    return 0 == description.sequence->compare(TAG_EMBEDDED);
  }


public:

  TagWarehouse() : myCounter(0), myTagDescription(0), correctPair(false), section(undefined)
  {
    // Here we should add embedded tags
    AddTagDescription(new TagDescription(TAG_EMBEDDED));
  }

  bool Init();

  void SetName(const std::wstring &name);

  const std::wstring& GetName();

  void AddTagDescription(TagDescription *tagDescription);

  void AddHeaderTagPair(TagDescription *openTag, TagDescription *closeTag, bool isCorrectPair, const std::vector<wchar_t> &escapeCharacters) const;

  void AddBodyTagPair(TagDescription *openTag, TagDescription *closeTag, bool isCorrectPair, const std::vector<wchar_t> &escapeCharacters) const;

  TagDescription* GetTagDescription(int index) const;

  TagDescription* GetTagDescription(const std::wstring *name) const;

  TagDescription* GetTagDescription(const wchar_t *name, int length) const;

  const AhoSearch &GetTagsParser() const;

  void Clear();

  ~TagWarehouse();


protected:

  void HandleSection(const std::wstring &name)
  {
    handleEndOfSection();

    if (name.compare(SECTION_COMMON_NAME) == 0)
      section = common;
    else if (name.compare(SECTION_TAG_NAME) == 0)
      section = tag;
    else if (name.compare(SECTION_TAG_HEADER_PAIR_NAME) == 0)
      section = headerPair;
    else if (name.compare(SECTION_TAG_BODY_PAIR_NAME) == 0)
      section = bodyPair;
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

    case tag:
      sectionTag(name, value);
      break;

    case bodyPair:
      sectionTagBodyPair(name, value);
      break;

    case headerPair:
      sectionTagHeaderPair(name, value);
      break;
    }
  }

  void HandleEndOfFile()
  {
    handleEndOfSection();
  }


private:

  void sectionCommon(const std::wstring &propertyName, const std::wstring &propertyValue);

  void sectionTag(const std::wstring &propertyName, const std::wstring &propertyValue);

  void sectionTagBodyPair(const std::wstring &propertyName, const std::wstring &propertyValue);

  void sectionTagHeaderPair(const std::wstring &propertyName, const std::wstring &propertyValue);

  const std::wstring& determineLineBreak(const std::wstring &propertyValue);

  void putTagDescription();

  void putHeaderTagPair();

  void putBodyTagPair();

  void handleEndOfSection();
};

#endif // TagWarehouse_h__
