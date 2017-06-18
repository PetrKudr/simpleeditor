#ifndef HighlightParser_h__
#define HighlightParser_h__

#include <windows.h>

#include <hash_set>
#include <algorithm>
#include <string>
#include <vector>

#include "AhoSearch.hpp"
#include "PropertyFileParser.hpp"


class HighlightParser : public PropertyFileParser 
{
public:

  struct SWordItem
  {
    COLORREF textColor;
    COLORREF bgColor;
    bool leftDelimitersMode;
    bool rightDelimitersMode;
    stdext::hash_set<wchar_t> delimiters;
    stdext::hash_set<wchar_t> leftDelimiters;
    stdext::hash_set<wchar_t> rightDelimiters;


    SWordItem() : textColor(RGB(0, 0, 0)), bgColor(RGB(255, 255, 255)), leftDelimitersMode(true), rightDelimitersMode(true)
    {
      delimiters.insert(L'\r');
      delimiters.insert(L'\n');
    }

    SWordItem(COLORREF _textColor, COLORREF _bgColor) : textColor(_textColor), bgColor(_bgColor), leftDelimitersMode(true), rightDelimitersMode(true)
    {
      delimiters.insert(L'\r');
      delimiters.insert(L'\n');
    }

    bool IsLeftDelimiter(wchar_t chr) const 
    {
      bool isCharacterLeftDelimiter = delimiters.find(chr) != delimiters.end() || leftDelimiters.find(chr) != leftDelimiters.end();    
      return !(leftDelimitersMode ^ isCharacterLeftDelimiter); // leftDelimitersMode ? isCharacterLeftDelimiter : !isCharacterLeftDelimiter;
    }

    bool IsRightDelimiter(wchar_t chr) const 
    {
      bool isCharacterRightDelimiter = delimiters.find(chr) != delimiters.end() || rightDelimiters.find(chr) != rightDelimiters.end();    
      return !(rightDelimitersMode ^ isCharacterRightDelimiter); // rightDelimitersMode ? isCharacterRightDelimiter : !isCharacterRightDelimiter
    }
  };


public:

  static const int DEFAULT_WORD_GROUP = 0;


private:

  enum Section 
  {
    undefined,
    default_wordgroup,
    wordgroup
  };


private:

  static const std::wstring SECTION_DEFAULT_WORDGROUP_NAME;

  static const std::wstring SECTION_WORDGROUP_NAME;


private:

  std::wstring myName;

  AhoSearch myAhoWords;

  std::vector<SWordItem*> myWordGroups;

  Section myCurrentSection;


public:

  HighlightParser() : myCurrentSection(undefined) {
    myWordGroups.push_back(new SWordItem());
  }

  HighlightParser(COLORREF defTextColor, COLORREF defBgColor) : myCurrentSection(undefined) {
    myWordGroups.push_back(new SWordItem(defTextColor, defBgColor));
  }

  const std::vector<SWordItem*>& GetWordGroups() const 
  {
    return myWordGroups;
  }

  const std::wstring& GetName() const
  {
    return myName;
  }

  const AhoSearch& GetWordsParser() const
  {
    return myAhoWords; 
  }

  int GetLengthOfTheLongestWord() const
  {
    return myAhoWords.GetLengthOfTheLongestWord();
  }

  int GetNumberOfWords() const
  {
    return myAhoWords.GetNumberOfWords();
  }

  void Clear()
  {
    for (std::vector<SWordItem*>::const_iterator iter = myWordGroups.begin(); iter != myWordGroups.end(); ++iter)
      delete (*iter);
    myWordGroups.clear();
  }

  ~HighlightParser()
  {
    Clear();
  }


protected:

  void HandleSection(const std::wstring &section) 
  {
    if (section.compare(SECTION_DEFAULT_WORDGROUP_NAME) == 0)
      myCurrentSection = default_wordgroup;
    else if (section.compare(SECTION_WORDGROUP_NAME) == 0)
    {
      myWordGroups.push_back(new SWordItem());
      myCurrentSection = wordgroup;
    }
    else 
      myCurrentSection = undefined;
  }

  void HandleSectionProperty(const std::wstring &name, const std::wstring &value)
  {
    switch (myCurrentSection) 
    {
    case default_wordgroup: 
      sectionDefaultWordGroup(name, value);
      break;

    case wordgroup:
      sectionWordgroup(name, value);
      break;
    }
  }

  void HandleEndOfFile()
  {
    myAhoWords.Prepare();
  }


private:

  void sectionWordgroup(const std::wstring &name, const std::wstring &value);

  void sectionDefaultWordGroup(const std::wstring &name, const std::wstring &value);
};

#endif // HighlightParser_h__
