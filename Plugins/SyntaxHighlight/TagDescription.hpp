#ifndef TagDescription_h__
#define TagDescription_h__

#include <list>
#include <string>
#include <hash_set>


struct TagDescription;


struct TagPair
{
  TagDescription *pair;
  bool correct;
  stdext::hash_set<wchar_t> escapeCharacters;

  TagPair() {}

  TagPair(TagDescription *_pair, bool _correct, const std::vector<wchar_t> &_escapeCharacters) : pair(_pair), correct(_correct)
  {
    for (std::vector<wchar_t>::const_iterator iter = _escapeCharacters.begin(); iter != _escapeCharacters.end(); iter++)
      escapeCharacters.insert(*iter);
  }

  bool IsEscapeCharacter(wchar_t chr) const 
  {
    return escapeCharacters.find(chr) != escapeCharacters.end();
  }
};


struct TagDescription 
{
public:

  typedef std::list<TagPair> TagPairs;


public:

  TagPairs bodyPairs;
  TagPairs headerPairs;

  const std::wstring *bodyLanguage;    // language which is used for parsing tag body. Tag body will be independent section
  const std::wstring *headerLanguage;  // language which is used for parsing tag header. Tag header will be independent section
  const std::wstring *sequence;        // main part of the tag (for example: "<td")

  // Delimiters
  bool isLeftDelimitersMode;
  bool isRightDelimitersMode;
  stdext::hash_set<wchar_t> delimiters;         // common
  stdext::hash_set<wchar_t> leftDelimiters;     // left
  stdext::hash_set<wchar_t> rightDelimiters;    // right


public:

  TagDescription(const std::wstring &sequence) 
  {
    setParameters(0, 0, &sequence);
  }

  const TagPair* GetBodyPair(const TagDescription &description) const
  {
    for (TagDescription::TagPairs::const_iterator iter = bodyPairs.begin(); iter != bodyPairs.end(); ++iter) 
    {
      if (0 == (*iter).pair->sequence->compare(*description.sequence)) 
        return &(*iter);
    }
    return 0;
  }

  bool IsBodyPair(const TagDescription &description) const
  {
    return GetBodyPair(description) != 0;
  }

  const TagPair* GetHeaderPair(const TagDescription &description) const
  {
    for (TagDescription::TagPairs::const_iterator iter = headerPairs.begin(); iter != headerPairs.end(); ++iter) 
    {
      if (0 == (*iter).pair->sequence->compare(*description.sequence)) 
        return &(*iter);
    }
    return 0;
  }

  bool IsHeaderPair(const TagDescription &description) const
  {
    return GetHeaderPair(description) != 0;
  }

  bool IsProperBodyPair(const TagDescription &description) const
  {
    const TagPair *pair = GetBodyPair(description);
    return pair != 0 && pair->correct;
  }

  bool IsProperHeaderPair(const TagDescription &description) const
  {
    const TagPair *pair = GetHeaderPair(description);
    return pair != 0 && pair->correct;
  }

  bool IsLeftDelimiter(wchar_t chr) const 
  {
    bool isCharacterLeftDelimiter = delimiters.find(chr) != delimiters.end() || leftDelimiters.find(chr) != leftDelimiters.end();    
    return !(isLeftDelimitersMode ^ isCharacterLeftDelimiter); // isLeftDelimitersMode ? isCharacterLeftDelimiter : !isCharacterLeftDelimiter;
  }

  bool IsRightDelimiter(wchar_t chr) const 
  {
    bool isCharacterRightDelimiter = delimiters.find(chr) != delimiters.end() || rightDelimiters.find(chr) != rightDelimiters.end();    
    return !(isRightDelimitersMode ^ isCharacterRightDelimiter); // isRightDelimitersMode ? isCharacterRightDelimiter : !isCharacterRightDelimiter
  }

  bool IsComplex() const
  {
    return headerLanguage != 0;
  }

  bool CanBeOpen() const
  {
    return bodyLanguage != 0;
  }

  ~TagDescription()
  {
    if (bodyLanguage != 0)
      delete bodyLanguage;
    if (headerLanguage != 0)
      delete headerLanguage;
    if (sequence != 0)
      delete sequence;
  }


private:

  void setParameters(const std::wstring *bodyLanguage, const std::wstring *headerLanguage, const std::wstring *sequence)
  {
    this->isLeftDelimitersMode = false;
    this->isRightDelimitersMode = false;
    this->bodyLanguage = bodyLanguage != 0 ? new std::wstring(*bodyLanguage) : 0;
    this->headerLanguage = headerLanguage != 0 ? new std::wstring(*headerLanguage) : 0;
    this->sequence = sequence != 0 ? new std::wstring(*sequence) : 0;
  }
};

#endif // TagDescription_h__
