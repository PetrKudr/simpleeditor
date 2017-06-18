#ifndef HighlighterTools_h__
#define HighlighterTools_h__

#include <string>

#include "seditcontrol.h"
#include "HighlightParser.hpp"
#include "TagWarehouse.hpp"

#include <hash_set>


namespace HighlighterTools {

  struct eqstr
  {
    bool operator()(const std::wstring &s1, const std::wstring &s2) const
    {
      return s1.compare(s2) == 0;
    }
  };


  bool AreIntersect(const range_t &range1, const range_t &range2, bool strictly = false);

  range_t Intersection(const range_t &range1, const range_t &range2);

  bool IsEmpty(const range_t &range);
  
  bool IsEnclosed(const range_t &outside, const range_t &inside, bool strictly = false); 

  bool RangeContains(const range_t &range, const position_t &pos, bool strictly = false);

  bool IsOnRightSide(const range_t &right, const range_t &left, bool strictly = false);

  bool IsOnLeftSide(const range_t &left, const range_t &right, bool strictly = false);

  bool AreEqual(const range_t &first, const range_t &second);

  bool AreEqual(const position_t &first, const position_t &second);

  bool IsLesser(const position_t &lesser, const position_t &bigger, bool strictly = false);

  bool IsBigger(const position_t &bigger, const position_t &lesser, bool strictly = false);

  const position_t& Max(const position_t &pos1, const position_t &pos2);

  const position_t& Min(const position_t &pos1, const position_t &pos2);

  position_t GetPositionFromOffset(const wchar_t *text, const std::wstring &lineBreak, const position_t& startPos, int offset);

  int GetOffsetFromPosition(const wchar_t *text, const std::wstring &lineBreak, const position_t& startPos, const position_t &pos);

  void GetMatches(__in const HighlightParser &parser,
                  __in const wchar_t *str, 
                  __in int length, 
                  __in int searchBegin,
                  __in int searchEnd, 
                  __out AhoSearch::AhoMatches &matches);

  void GetMatches(__in const TagWarehouse &warehouse,
                  __in const wchar_t *str, 
                  __in int length, 
                  __in int searchBegin,
                  __in int searchEnd, 
                  __out AhoSearch::AhoMatches &matches);

  range_t ConstructRange(const position_t &start, const position_t &end);

  range_t ConstructRange(unsigned int startX, unsigned int startY, unsigned int endX, unsigned int endY);

};

#endif // HighlighterTools_h__