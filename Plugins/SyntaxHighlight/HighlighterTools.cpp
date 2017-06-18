#include "HighlighterTools.hpp"

#include <assert.h>

#include <ctime>
#include "MessageCollector.hpp"


bool HighlighterTools::AreIntersect(const range_t &range1, const range_t &range2, bool strictly)
{
  return RangeContains(range1, range2.start, strictly) || RangeContains(range1, range2.end, strictly) || 
         RangeContains(range2, range1.start, strictly) || RangeContains(range2, range1.end, strictly);
}

range_t HighlighterTools::Intersection(const range_t &range1, const range_t &range2)
{
  range_t result;
  result.start = Max(range1.start, range2.start);
  result.end = Min(range1.end, range2.end);
  return result;
}

bool HighlighterTools::IsEmpty(const range_t &range)
{
  return range.end.y == range.start.y && range.end.x == range.start.x;
}

bool HighlighterTools::IsEnclosed(const range_t &outside, const range_t &inside, bool strictly) 
{
  return IsLesser(outside.start, inside.start, strictly) && IsBigger(outside.end, inside.end, strictly);
}

bool HighlighterTools::RangeContains(const range_t &range, const position_t &pos, bool strictly) 
{
  return IsLesser(range.start, pos, strictly) && IsBigger(range.end, pos, strictly);
}

bool HighlighterTools::IsOnRightSide(const range_t &right, const range_t &left, bool strictly)
{
  return IsLesser(left.end, right.start, strictly);
}

bool HighlighterTools::IsOnLeftSide(const range_t &left, const range_t &right, bool strictly)
{
  return IsOnRightSide(right, left, strictly);
}

bool HighlighterTools::AreEqual(const range_t &first, const range_t &second)
{
  return AreEqual(first.start, second.start) && AreEqual(first.end, second.end);
}

bool HighlighterTools::AreEqual(const position_t &first, const position_t &second)
{
  return first.y == second.y && first.x == second.x;
}

bool HighlighterTools::IsLesser(const position_t &lesser, const position_t &bigger, bool strictly) 
{
  return lesser.y < bigger.y || (lesser.y == bigger.y && (strictly ? lesser.x < bigger.x : lesser.x <= bigger.x));
}

bool HighlighterTools::IsBigger(const position_t &bigger, const position_t &lesser, bool strictly) 
{
  return IsLesser(lesser, bigger, strictly);
}

const position_t& HighlighterTools::Max(const position_t &pos1, const position_t &pos2)
{
  if (IsBigger(pos1, pos2))
    return pos1;
  return pos2;
}

const position_t& HighlighterTools::Min(const position_t &pos1, const position_t &pos2)
{
  if (IsLesser(pos1, pos2))
    return pos1;
  return pos2;
}

position_t HighlighterTools::GetPositionFromOffset(const wchar_t *text, const std::wstring &lineBreak, const position_t& textPosition, int offset)
{
  int prevOffset = 0, curOffset = 0;
  int lines = 0;

  const wchar_t *br = text;

  while ((br = wcsstr(br, lineBreak.c_str())) != 0 && (curOffset = br - text) < offset)
  {
    ++lines;
    br += lineBreak.length();
    prevOffset = curOffset + lineBreak.length();
  } 

  position_t position;
  if (lines > 0) 
  {
    position.x = offset - prevOffset;
    position.y = textPosition.y + lines; 
  }
  else
  {
    position.x = textPosition.x + offset;
    position.y = textPosition.y;
  }
  return position;
}

int HighlighterTools::GetOffsetFromPosition(const wchar_t *text, const std::wstring &lineBreak, const position_t& startPos, const position_t &pos)
{
  if (startPos.y != pos.y) 
  {  
    const wchar_t *br = text;
    for (unsigned int i = startPos.y; i < pos.y; ++i)
    {
      br = wcsstr(br, lineBreak.c_str());
      br += lineBreak.length();
    }
    return br - text + pos.x;
  }
  return pos.x - startPos.x;
}


struct IMatchChecker
{
  virtual bool IsLeftDelimiter(const wchar_t *str, const AhoSearch::AhoMatchItem &match, wchar_t character) = 0;

  virtual bool IsRightDelimiter(const wchar_t *str, const AhoSearch::AhoMatchItem &match, wchar_t character) = 0;
};

class KeywordMatchChecker : public IMatchChecker
{
private:

  const HighlightParser &parser;


public:

  KeywordMatchChecker(const HighlightParser &_parser) : parser(_parser) {}

  bool IsLeftDelimiter(const wchar_t *str, const AhoSearch::AhoMatchItem &match, wchar_t character)
  {
    return parser.GetWordGroups()[match.param]->IsLeftDelimiter(character);
  }

  bool IsRightDelimiter(const wchar_t *str, const AhoSearch::AhoMatchItem &match, wchar_t character)
  {
    return parser.GetWordGroups()[match.param]->IsRightDelimiter(character);
  }
};

class TagMatchChecker : public IMatchChecker
{
private:

  const TagWarehouse &warehouse;


public:

  TagMatchChecker(const TagWarehouse &_warehouse) : warehouse(_warehouse) {}

  bool IsLeftDelimiter(const wchar_t *str, const AhoSearch::AhoMatchItem &match, wchar_t character)
  {
    //const TagDescription *description = warehouse.GetTagDescription(&str[match.pos], match.length);
    const TagDescription *description = warehouse.GetTagDescription(match.param);
    assert(description != 0);
    return description->IsLeftDelimiter(character);
  }

  bool IsRightDelimiter(const wchar_t *str, const AhoSearch::AhoMatchItem &match, wchar_t character)
  {
    //const TagDescription *description = warehouse.GetTagDescription(&str[match.pos], match.length);
    const TagDescription *description = warehouse.GetTagDescription(match.param);
    assert(description != 0);
    return description->IsRightDelimiter(character);
  }
};

static void getMatches(__in const AhoSearch &parser,
                       __in const wchar_t *str, 
                       __in int length,
                       __in int searchBegin,
                       __in int searchEnd, 
                       __in IMatchChecker &delimitersChecker,
                       __out AhoSearch::AhoMatches &matches)
{
  matches.clear();

  parser.Search(&str[searchBegin], searchEnd - searchBegin, matches, true);

  // Update match positions (they should be from the start of the line) and test them with delimiters
  if (matches.size() > 0) 
  {
    wchar_t predecessor, subsequent;
    bool checkMatchResult;

    AhoSearch::AhoMatches::iterator i = matches.begin();
    while (i != matches.end()) 
    {
      AhoSearch::AhoMatchItem &match = *i;

      match.pos += searchBegin;

      checkMatchResult = true;

      if (checkMatchResult && match.pos > 0) 
      {
        predecessor = str[match.pos - 1];
        checkMatchResult &= delimitersChecker.IsLeftDelimiter(str, match, predecessor);
      }

      if (checkMatchResult && match.pos + match.length < length) 
      {
        subsequent = str[match.pos + match.length];
        checkMatchResult &= delimitersChecker.IsRightDelimiter(str, match, subsequent);
      }

      if (!checkMatchResult)
        i = matches.erase(i);
      else 
        i++;
    }
  }

  // Intersections are not allowed, so remove them
  //AhoSearch::RemoveIntersections(matches);
}

void HighlighterTools::GetMatches(__in const HighlightParser &parser,
                                  __in const wchar_t *str, 
                                  __in int length,
                                  __in int searchBegin,
                                  __in int searchEnd, 
                                  __out AhoSearch::AhoMatches &matches)
{
  getMatches(parser.GetWordsParser(), str, length, searchBegin, searchEnd, KeywordMatchChecker(parser), matches);
}

void HighlighterTools::GetMatches(__in const TagWarehouse &warehouse,
                                  __in const wchar_t *str, 
                                  __in int length,
                                  __in int searchBegin,
                                  __in int searchEnd, 
                                  __out AhoSearch::AhoMatches &matches)
{
  getMatches(warehouse.GetTagsParser(), str, length, searchBegin, searchEnd, TagMatchChecker(warehouse), matches);
}

range_t HighlighterTools::ConstructRange(const position_t &start, const position_t &end)
{
  range_t result;
  result.start = start;
  result.end = end;
  return result;
}

range_t HighlighterTools::ConstructRange(unsigned int startX, unsigned int startY, unsigned int endX, unsigned int endY)
{
  range_t result;
  result.start.x = startX;
  result.start.y = startY;
  result.end.x = endX;
  result.end.y = endY;
  return result;
}