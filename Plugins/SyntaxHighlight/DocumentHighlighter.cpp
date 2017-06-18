#include <cstring>
#include <cassert>
#include <cstdlib>
#include <climits>
#include <ctime>

#include "DocumentHighlighter.hpp"
#include "HighlighterTools.hpp"
#include "seditcontrol.h"



DocumentHighlighter::DocumentHighlighter(HWND _heditorWnd, const std::wstring &defaultLanguage, const COLORREF *caretLineTextColor, const COLORREF *caretLineBackgroundColor) 
  : myEditorWnd(_heditorWnd), myCaretLineTextColor(caretLineTextColor), myCaretLineBackgroundColor(caretLineBackgroundColor), myRootSection(0), 
    myCachedText(0), myLastRequestedPositionStartOffset(0), myLastRequestedPositionEndOffset(0), myCachedTopLevelTag(0)
{
  memset(&myLastRequestedPosition, 0, sizeof(range_t));

  options_t options;

  SendMessage(myEditorWnd, SE_GETOPTIONS, (WPARAM)&options, 0);

  switch (options.lineBreakType) 
  {
  case SEDIT_LINEBREAK_CRLF:
    lineBreak.assign(L"\r\n");
    break;

  case SEDIT_LINEBREAK_CR:
    lineBreak.assign(L"\r");
    break;

  case SEDIT_LINEBREAK_LF:
    lineBreak.assign(L"\n");
    break;
  }


  doc_info_t di;
  di.flags = SEDIT_GETCARETREALPOS;

  SendMessage(myEditorWnd, SE_GETDOCINFO, (WPARAM)&di, 0);

  myLastCaretPosition = di.caretPos;


  myDefaultLanguage = LanguagesSource::GetInstance().GetLanguage(defaultLanguage);

  memset(&myLastColoredRange, 0, sizeof(range_t));
}

DocumentHighlighter::~DocumentHighlighter()
{
  if (myCachedText)
    delete myCachedText;
  if (myRootSection)
    delete myRootSection;
}

int DocumentHighlighter::GetStringLength(int number) const
{
  int length = SendMessage(myEditorWnd, SE_GETLINELENGTH, (WPARAM)number, SEDIT_REALLINE);
  return length >= 0 ? length : 0;
}

range_t DocumentHighlighter::GetTextRange() const 
{
  doc_info_t docInfo = {0};
  SendMessage(myEditorWnd, SE_GETDOCINFO, (WPARAM)&docInfo, 0);

  range_t range = {0};
  range.end.y = docInfo.nRealLines - 1;
  range.end.x = GetStringLength(range.end.y);
  return range;
}

TextBlock* DocumentHighlighter::GetText() const 
{
  //MessageCollector::GetInstance().AddMessage(information, L"Loading whole text\n");

  text_block_t tb = {0};
  tb.position = GetTextRange();

  tb.size = SendMessage(myEditorWnd, SE_GETTEXT, (WPARAM)&tb, SEDIT_REALLINE);
  tb.text = new wchar_t[tb.size];
  SendMessage(myEditorWnd, SE_GETTEXT, (WPARAM)&tb, SEDIT_REALLINE);
  
  // Note that tb.size = length of the text block + 1 (for null symbol)
  return new AutoClearingTextBlock(tb.text, tb.size - 1, tb.position);
}

TextBlock* DocumentHighlighter::GetText(const range_t &position) const
{
  if (myCachedText && HighlighterTools::IsEnclosed(myCachedText->GetRange(), position)) 
  {
    const wchar_t* cachedText = myCachedText->GetText();
    const range_t& cachedPosition = myCachedText->GetRange();

    const position_t *pfromPosition = &cachedPosition.start;
    int fromOffset = 0;

    // try to use cached results from last requested range
    if (!HighlighterTools::IsEmpty(myLastRequestedPosition))
    {
      if (HighlighterTools::IsBigger(position.start, myLastRequestedPosition.end))
      {
        pfromPosition = &myLastRequestedPosition.end;
        fromOffset = myLastRequestedPositionEndOffset;
      }
      else if (HighlighterTools::IsBigger(position.start, myLastRequestedPosition.start))
      {
        pfromPosition = &myLastRequestedPosition.start;
        fromOffset = myLastRequestedPositionStartOffset;
      }
    }

    int startOffset = HighlighterTools::GetOffsetFromPosition(&cachedText[fromOffset], lineBreak, *pfromPosition, position.start) + fromOffset;
    int length = HighlighterTools::GetOffsetFromPosition(&cachedText[startOffset], lineBreak, position.start, position.end);

    myLastRequestedPosition = position;
    myLastRequestedPositionStartOffset = startOffset;
    myLastRequestedPositionEndOffset = startOffset + length;

    return new SimpleTextBlock(&cachedText[startOffset], length, position);
  }
  else 
  {
    text_block_t tb;
    tb.size = 0;
    tb.position = position;

    tb.size = SendMessage(myEditorWnd, SE_GETTEXT, (WPARAM)&tb, SEDIT_REALLINE);
    tb.text = new wchar_t[tb.size];  
    SendMessage(myEditorWnd, SE_GETTEXT, (WPARAM)&tb, SEDIT_REALLINE);

    // tb.size = length of the text block + 1 (for null symbol)
    return new AutoClearingTextBlock(tb.text, tb.size - 1, position);
  }
}

void DocumentHighlighter::CacheText()
{
  ClearCachedText();
  myCachedText = GetText();
}

void DocumentHighlighter::CacheText(const range_t &position, bool clear)
{
  bool needCache = clear || (myCachedText != 0 && !HighlighterTools::IsEnclosed(myCachedText->GetRange(), position));
  if (needCache) 
  {  
    ClearCachedText();  
    myCachedText = GetText(position);
  }
}

void DocumentHighlighter::CacheText(const TextBlock &textBlock, bool clear)
{
  bool needCache = clear || (myCachedText != 0 && !HighlighterTools::IsEnclosed(myCachedText->GetRange(), textBlock.GetRange()));
  if (needCache)
  {
    ClearCachedText();
    myCachedText = new SimpleTextBlock(textBlock);
  }
}

void DocumentHighlighter::ClearCachedText()
{
  if (myCachedText)
  {
    delete myCachedText;
    myCachedText = 0;
    memset(&myLastRequestedPosition, 0, sizeof(range_t));
    myLastRequestedPositionStartOffset = myLastRequestedPositionEndOffset = 0;
  }
}

const std::wstring& DocumentHighlighter::GetLineBreak() const
{
  return lineBreak;
}

range_t DocumentHighlighter::ExtendRange(const range_t &range, unsigned int size, const range_t *limiter) const
{
  range_t extendedRange = range;
  ExtendStartPosition(extendedRange, size);
  ExtendEndPosition(extendedRange, size);
  return limiter != 0 ? HighlighterTools::Intersection(extendedRange, *limiter) : extendedRange;
}

int DocumentHighlighter::ExtendStartPosition(range_t &range, int length) const
{
  if (range.start.x > length)
  {
    range.start.x -= length;
    return length;
  } 
  else 
  {
    int retval = range.start.x;
    range.start.x = 0;
    return retval;
  }  
}

int DocumentHighlighter::ExtendEndPosition(range_t &range, int length) const
{
  unsigned int lineLength = GetStringLength(range.end.y);

  if (range.end.x + length < lineLength)
  {
    range.end.x = range.end.x + length;
    return length;
  } 
  else 
  {
    int retval = lineLength - range.end.x;
    range.end.x = lineLength;
    return retval;
  }
}

const Language* DocumentHighlighter::GetDefaultLanguage() const
{
  return myDefaultLanguage;
}

HWND DocumentHighlighter::GetEditorWindow() const
{
  return myEditorWnd;
}

void DocumentHighlighter::HandleTextModification(const range_t &modifiedRange, bool isDeleteAction, bool fullRefresh) 
{
  // Here we must analyze modification and recolor text, which could be modified

  if (fullRefresh || memcmp(&modifiedRange.start, &modifiedRange.end, sizeof(position_t)) != 0)
  {
    TagSection::TagUpdateResult updateResult;

    //MessageCollector &collector = MessageCollector::GetInstance();
    //clock_t before = clock();
    //collector.searchAhoTime = collector.transformAhoTime = collector.prepareAhoSearch = 0;

    if (fullRefresh || !myRootSection || !myRootSection->UpdateSections(modifiedRange, isDeleteAction, updateResult)) 
    { 
      if (myRootSection != 0)
        delete myRootSection;
      CacheText();
      myRootSection = new TagSection(this, myDefaultLanguage, myCachedText->GetRange(), 0);
      updateResult.section = myRootSection;
      updateResult.range = myRootSection->GetRange();
    }

    //clock_t after = clock();

    /*
    MessageCollector::GetInstance().AddMessage(
      information, 
      L"Whole operation takes %d ms in total where parsing part is %d (%.1f%%)", 
      (after - before), 
      collector.searchAhoTime + collector.transformAhoTime + collector.prepareAhoSearch, 
      (float)(collector.searchAhoTime + collector.transformAhoTime + collector.prepareAhoSearch) / (after - before) * 100
    );

    MessageCollector::GetInstance().AddMessage(
      information, 
      L"Parsing takes %d ms - %d ms for preparing, %d ms for searching and %d ms for transforming", 
      collector.prepareAhoSearch + collector.searchAhoTime + collector.transformAhoTime, 
      collector.prepareAhoSearch,
      collector.searchAhoTime,
      collector.transformAhoTime
    );
    */

    // text was modified, so last colored range can be invalid
    memset(&myLastColoredRange, 0, sizeof(range_t));

    // last top level tag can be invalid too
    myCachedTopLevelTag = 0;

    redrawRangeInEditor(updateResult.range);
  }
}

void DocumentHighlighter::HandleCaretModification(unsigned int x, unsigned int y)
{
  if (myCaretLineTextColor || myCaretLineBackgroundColor)
  {
    if (myLastCaretPosition.y != y)
    {
      range_t oldRange = {0};
      range_t newRange = {0};

      if (myLastCaretPosition.y != y)
      {
        oldRange.start.y = oldRange.end.y = myLastCaretPosition.y;
        oldRange.end.x = GetStringLength(oldRange.end.y);

        newRange.start.y = newRange.end.y = y;
        newRange.end.x = GetStringLength(newRange.end.y);
      }

      myLastCaretPosition.y = y;
      myLastCaretPosition.x = x;

      // last colored range can be invalid, if it corresponds to the caret line
      memset(&myLastColoredRange, 0, sizeof(range_t));

      redrawRangeInEditor(oldRange);
      redrawRangeInEditor(newRange);
    }
  }
}

void DocumentHighlighter::ColorRange(const range_t &range)
{
  if (!HighlighterTools::AreEqual(range, myLastColoredRange)) 
  {
    if (range.start.y != range.end.y)
      throw new std::runtime_error("Such case is not implemented, range must represent only one line.");

    TagSection *rangeSection = myRootSection->GetAppropriateSection(range, myCachedTopLevelTag);

    assert(rangeSection != 0);

    myTextColorBuffer.resize(range.end.x - range.start.x + 1);
    myBgColorBuffer.resize(range.end.x - range.start.x + 1);

    // cache top-level tag
    if (rangeSection == myRootSection)
    {
      TagNode *tagNode = (myCachedTopLevelTag != 0) ? myCachedTopLevelTag : rangeSection->GetFirstTag();

      if (tagNode)
      {
        if (HighlighterTools::IsLesser(tagNode->getRange().start, range.start))
        {
          while (tagNode && HighlighterTools::IsLesser(tagNode->getRange().start, range.start, true))
          {
            myCachedTopLevelTag = tagNode;
            tagNode = tagNode->right;
          }
        }
        else 
        {
          while (tagNode && !HighlighterTools::IsLesser(tagNode->getRange().start, range.start, true))
            tagNode = tagNode->left;
          myCachedTopLevelTag = (tagNode != 0) ? tagNode : rangeSection->GetFirstTag();
        }
      }
    }

    highlightSection(*rangeSection, range, &myTextColorBuffer[0], &myBgColorBuffer[0]);

    myLastColoredRange = range;
  }
}

const std::vector<COLORREF>& DocumentHighlighter::GetTextColorBuffer() const
{
  return myTextColorBuffer;
}

const std::vector<COLORREF>& DocumentHighlighter::GetBgColorBuffer() const
{
  return myBgColorBuffer;
}

const range_t& DocumentHighlighter::GetLastColoredRange() const
{
  return myLastColoredRange;
}


void DocumentHighlighter::redrawRangeInEditor(const range_t &range)
{
  SendMessage(myEditorWnd, SE_UPDATERANGE, (WPARAM)SEDIT_REALLINE, (LPARAM)&range);
}

void DocumentHighlighter::highlightSection(const TagSection &tagSection, const range_t &colorRange, COLORREF *textColorBuffer, COLORREF *bgColorBuffer) const 
{
  TagNode *tagNode = (myCachedTopLevelTag != 0 && myCachedTopLevelTag->parent == &tagSection) ? myCachedTopLevelTag : tagSection.GetFirstTag();
  TagNode *previous = 0;

  while (0 != tagNode && HighlighterTools::IsLesser(tagNode->getRange().start, colorRange.end, true)) 
  {
    if (HighlighterTools::IsBigger(tagNode->getMostRightPosition(), colorRange.start, true))
      highlightTag(tagSection, *tagNode, colorRange, textColorBuffer, bgColorBuffer);
    previous = tagNode;
    tagNode = tagNode->right;
  }

  if (0 != tagNode) 
  {
    range_t range;

    if (0 != previous)
    {
      range.start = previous->getMostRightPosition();
      range.end = tagNode->getMostLeftPosition();
    }
    else 
    {
      range.start = tagSection.GetStartPosition();
      range.end = tagNode->getMostLeftPosition();
    }

    highlightText(tagSection.GetLanguage()->GetHighlightParser(), range, colorRange, textColorBuffer, bgColorBuffer);
  } 
  else 
  {
    if (0 != previous) 
    {
      range_t range;
      range.start = previous->getMostRightPosition();
      range.end = tagSection.GetEndPosition();
      highlightText(tagSection.GetLanguage()->GetHighlightParser(), range, colorRange, textColorBuffer, bgColorBuffer);
    } 
    else 
      highlightText(tagSection.GetLanguage()->GetHighlightParser(), tagSection.GetRange(), colorRange, textColorBuffer, bgColorBuffer);
  }
}

void DocumentHighlighter::highlightTag(const TagSection &section, const TagNode &tag, const range_t &colorRange, COLORREF *textColorBuffer, COLORREF *bgColorBuffer) const
{
  // highlight text between tag and previous tag (we must do it only once)
  if (tag.isOpen || 0 == tag.bodyPair) 
  {
    range_t range;

    if (0 != tag.left) 
    {
      range.start = tag.left->getMostRightPosition();
      range.end = tag.getLeftPosition();
      highlightText(section.GetLanguage()->GetHighlightParser(), range, colorRange, textColorBuffer, bgColorBuffer);
    } 
    else 
    {
      range.start = section.GetStartPosition();
      range.end = tag.getMostLeftPosition();
      highlightText(section.GetLanguage()->GetHighlightParser(), range, colorRange, textColorBuffer, bgColorBuffer);
    }
  }

  // highlight tag name
  highlightTagName(section, tag, colorRange, textColorBuffer, bgColorBuffer);

  if (tag.description->IsComplex()) 
  {
    // highlight tag header
    highlightTagHeader(section, tag, colorRange, textColorBuffer, bgColorBuffer);

    // highlight suffix name
    highlightTagName(section, *tag.headerPair, colorRange, textColorBuffer, bgColorBuffer);
  }

  if (tag.isOpen) 
  {
    // highlight tag body
    highlightTagBody(section, tag, colorRange, textColorBuffer, bgColorBuffer);

    // highlight closing tag
    highlightTag(section, *tag.bodyPair, colorRange, textColorBuffer, bgColorBuffer);
  }
}

void DocumentHighlighter::highlightTagHeader(const TagSection &section, const TagNode &tag, const range_t &colorRange, COLORREF *textColorBuffer, COLORREF *bgColorBuffer) const 
{
  if (0 != tag.header) 
    highlightSection(*tag.header, colorRange, textColorBuffer, bgColorBuffer);
}

void DocumentHighlighter::highlightTagBody(const TagSection &section, const TagNode &tag, const range_t &colorRange, COLORREF *textColorBuffer, COLORREF *bgColorBuffer) const
{
  if (0 != tag.body) {  // <=> tagNode.isOpen
    highlightSection(*tag.body, colorRange, textColorBuffer, bgColorBuffer);
  }
}

void DocumentHighlighter::highlightTagName(const TagSection &section, const TagNode &tag, const range_t &colorRange, COLORREF *textColorBuffer, COLORREF *bgColorBuffer) const 
{
  highlightText(section.GetLanguage()->GetHighlightParser(), tag.getRange(), colorRange, textColorBuffer, bgColorBuffer);        
}

void DocumentHighlighter::highlightText(const HighlightParser &highlightParser, const range_t &limitRange, const range_t &colorRange, COLORREF *textColorBuffer, COLORREF *bgColorBuffer) const
{
  // Test if we need to color something
  if (!HighlighterTools::AreIntersect(limitRange, colorRange))
    return;

  range_t intersection = HighlighterTools::Intersection(limitRange, colorRange);

  if (HighlighterTools::IsBigger(intersection.start, intersection.end)) 
    return;  // intersection is empty

  // Range in which we should search keywords
  range_t searchRange = ExtendRange(intersection, highlightParser.GetLengthOfTheLongestWord() + 1, &limitRange);

  // Get text for search and color ranges
  TextBlock *searchBlock;
  TextBlock *blockForColoring;

  // This is optimization - when one range is inside another, we should first retrieve text for the bigger range (because of caching).
  if (HighlighterTools::IsEnclosed(searchRange, colorRange))
  {
    searchBlock = GetText(searchRange);
    blockForColoring = GetText(colorRange);
  }
  else
  {
    blockForColoring = GetText(colorRange);
    searchBlock = GetText(searchRange);
  }

  // offsets of blockForColoring in search range
  int blockForColoringStartOffset, blockForColoringEndOffset;
  if (HighlighterTools::IsLesser(searchRange.start, colorRange.start)) {
    blockForColoringStartOffset = HighlighterTools::GetOffsetFromPosition(searchBlock->GetText(), lineBreak, searchRange.start, colorRange.start);
    blockForColoringEndOffset = blockForColoringStartOffset + blockForColoring->GetTextLength();
  }
  else
  {
    blockForColoringStartOffset = -HighlighterTools::GetOffsetFromPosition(blockForColoring->GetText(), lineBreak, colorRange.start, searchRange.start);
    blockForColoringEndOffset = blockForColoringStartOffset + blockForColoring->GetTextLength();
  }

  // offsets of intersection in blockForColoring
  int intersectionStartOffset = HighlighterTools::GetOffsetFromPosition(blockForColoring->GetText(), lineBreak, colorRange.start, intersection.start);
  int intersectionEndOffset = intersectionStartOffset + HighlighterTools::GetOffsetFromPosition(&blockForColoring->GetText()[intersectionStartOffset], lineBreak, intersection.start, intersection.end);
  int length = intersectionEndOffset - intersectionStartOffset;


  const HighlightParser::SWordItem &wordGroup = *highlightParser.GetWordGroups()[HighlightParser::DEFAULT_WORD_GROUP];
  COLORREF textColor = wordGroup.textColor;
  COLORREF bgColor = wordGroup.bgColor;

  // in fact colorRange always represents only one line, so second check is useless
  if (myLastCaretPosition.y == colorRange.start.y && myLastCaretPosition.y == colorRange.end.y)
  {
    if (myCaretLineTextColor)
      textColor = *myCaretLineTextColor;
    if (myCaretLineBackgroundColor)
      bgColor = *myCaretLineBackgroundColor;
  }

  for (int i = intersectionStartOffset; i < intersectionEndOffset; ++i) 
  {
    textColorBuffer[i] = textColor;
    bgColorBuffer[i] = bgColor;
  }


  AhoSearch::AhoMatches matches;
  AhoSearch::AhoMatches::const_iterator iMatch;

  HighlighterTools::GetMatches(
      highlightParser, 
      searchBlock->GetText(), 
      searchBlock->GetTextLength(),
      0,
      searchBlock->GetTextLength(),
      matches
  );
  AhoSearch::RemoveIntersections(matches);

  for (iMatch = matches.begin(); iMatch != matches.end(); ++iMatch)
  {
    if (blockForColoringStartOffset + intersectionStartOffset < iMatch->pos + iMatch->length && blockForColoringStartOffset + intersectionEndOffset > iMatch->pos)
    {
      int startOffsetInMatch = max(0, blockForColoringStartOffset + intersectionStartOffset - iMatch->pos);
      int endOffsetInMatch = min(iMatch->length, blockForColoringStartOffset + intersectionEndOffset - iMatch->pos);

      textColor = highlightParser.GetWordGroups()[iMatch->param]->textColor;
      bgColor = highlightParser.GetWordGroups()[iMatch->param]->bgColor;

        // in fact colorRange always represents only one line, so second check is useless
      if (myLastCaretPosition.y == colorRange.start.y && myLastCaretPosition.y == colorRange.end.y)
      {
        if (myCaretLineTextColor)
          textColor = *myCaretLineTextColor;
        if (myCaretLineBackgroundColor)
          bgColor = *myCaretLineBackgroundColor;
      }

      for (int i = startOffsetInMatch; i < endOffsetInMatch; ++i) 
      {
        textColorBuffer[iMatch->pos - blockForColoringStartOffset + i] = textColor;
        bgColorBuffer[iMatch->pos - blockForColoringStartOffset + i] = bgColor;
      }
    }
  }

  delete searchBlock;
  delete blockForColoring;
}
