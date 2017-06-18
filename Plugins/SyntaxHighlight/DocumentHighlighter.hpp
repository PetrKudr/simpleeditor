#ifndef DocumentHighlighter_h__
#define DocumentHighlighter_h__

#include <vector>
#include <list>

#include "seditcontrol.h"
#include "MessageCollector.hpp"
#include "AhoSearch.hpp"
#include "TagNode.hpp"
#include "TagDescription.hpp"
#include "TagWarehouse.hpp"
#include "TagSection.hpp"
#include "HighlightParser.hpp"
#include "LanguagesSource.hpp"
#include "TextBlock.hpp"


#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif


class DocumentHighlighter 
{
private:

  // Window of the edit control
  HWND myEditorWnd;

  // language for the root section
  const Language *myDefaultLanguage;

  // Color of text in caret line
  const COLORREF *myCaretLineTextColor;
  
  // Color of background in caret line
  const COLORREF *myCaretLineBackgroundColor;

  // Last position of caret
  position_t myLastCaretPosition;

  // Line break which is used in editor (now always CRLF)
  std::wstring lineBreak;

  // Root section
  TagSection *myRootSection;

  // buffer for text color
  std::vector<COLORREF> myTextColorBuffer;

  // buffer for background color
  std::vector<COLORREF> myBgColorBuffer;

  // last colored range (buffers contain information for it)
  range_t myLastColoredRange;

  // Cached text
  mutable TextBlock *myCachedText;

  // Last requested position from cached text
  mutable range_t myLastRequestedPosition;

  // Start offset in cached text of myLastRequestedPosition.start
  mutable int myLastRequestedPositionStartOffset;

  // End offset in cached text of myLastRequestedPosition.start
  mutable int myLastRequestedPositionEndOffset;

  // Top level tag near last colored section (for search optimization)
  TagNode *myCachedTopLevelTag;


public:

  DocumentHighlighter(HWND _heditorWnd, const std::wstring &defaultLanguage, const COLORREF *caretLineTextColor, const COLORREF *caretLineBackgroundColor);

  HWND GetEditorWindow() const;

  void HandleTextModification(const range_t &modifiedRange, bool isDeleteAction, bool fullRefresh = false);

  void HandleCaretModification(unsigned int x, unsigned int y);

  void ColorRange(const range_t &range);

  const std::vector<COLORREF>& GetTextColorBuffer() const;

  const std::vector<COLORREF>& GetBgColorBuffer() const;

  const range_t& GetLastColoredRange() const;

  ~DocumentHighlighter();

  /************************************************************************/
  /*                     Auxiliary functions                              */
  /************************************************************************/

  int GetStringLength(int number) const;

  range_t GetTextRange() const;

  TextBlock* GetText() const;

  TextBlock* GetText(const range_t &position) const;

  void CacheText();

  void CacheText(const range_t &position, bool clear = true);

  void CacheText(const TextBlock &textBlock, bool clear = true);

  void ClearCachedText();

  const std::wstring& GetLineBreak() const;

  range_t ExtendRange(const range_t &range, unsigned int lengthOfTheLongestWord, const range_t *limiter) const;

  int ExtendStartPosition(range_t &range, int length) const;

  int ExtendEndPosition(range_t &range, int length) const;

  const Language* GetDefaultLanguage() const;


private:

  void redrawRangeInEditor(const range_t &range);

  void highlightSection(const TagSection &tagSection, const range_t &colorRange, COLORREF *textColorBuffer, COLORREF *bgColorBuffer) const;

  void highlightTag(const TagSection &section, const TagNode &tag, const range_t &colorRange, COLORREF *textColorBuffer, COLORREF *bgColorBuffer) const;

  void highlightTagHeader(const TagSection &section, const TagNode &tag, const range_t &colorRange, COLORREF *textColorBuffer, COLORREF *bgColorBuffer) const;

  void highlightTagBody(const TagSection &section, const TagNode &tag, const range_t &colorRange, COLORREF *textColorBuffer, COLORREF *bgColorBuffer) const;

  void highlightTagName(const TagSection &section, const TagNode &tag, const range_t &colorRange, COLORREF *textColorBuffer, COLORREF *bgColorBuffer) const;

  void highlightText(const HighlightParser &highlightParser, const range_t &elementRange,  const range_t &colorRange, COLORREF *textColorBuffer, COLORREF *bgColorBuffer) const;

};


#endif // DocumentHighlighter_h__