#include "seditcontrol.h"

#include "AhoSearch.hpp"
#include "TagNode.hpp"
#include "Language.hpp"
#include "DocumentHighlighter.hpp"
#include "TagSection.hpp"


#include <assert.h>
#include <ctime>


TagSection::TagSection(DocumentHighlighter *highlighter, const std::wstring &language, const range_t &position, TagNode *parent) 
{
  initSection(highlighter, LanguagesSource::GetInstance().GetLanguage(language), position, parent);
  appendSection(position);
}

TagSection::TagSection(DocumentHighlighter *highlighter, const Language *language, const range_t &position, TagNode *parent) 
{
  initSection(highlighter, language, position, parent);
  appendSection(position);
}

TagNode* TagSection::GetSectionParent() const
{
  return myParentNode;
}

TagNode* TagSection::GetFirstTag() const
{
  return myFirstTag;
}

position_t TagSection::GetStartPosition() const 
{
  return mySectionRange.start;
}

position_t TagSection::GetEndPosition() const 
{
  return mySectionRange.end;
}

const range_t& TagSection::GetRange() const 
{
  return mySectionRange;
}

const Language* TagSection::GetLanguage() const 
{
  return myLanguage;
}

bool TagSection::UpdateSections(__in const range_t &range, __in bool isDelete, __out TagUpdateResult &updateResult) 
{
  // Method should be called for root section.
  if (0 == myParentNode) 
  {
    if (!HighlighterTools::AreEqual(range.start, range.end))
    {      
      if (!isDelete) 
        return updateSectionOnInsert(range, updateResult);
      else 
        return updateSectionOnDelete(range, updateResult);
    }
  }
  return false;
}

TagSection::~TagSection()
{
  TagNode *tag = myFirstTag, *prev = 0;
  while (tag != 0)
  {
    prev = tag;
    tag = tag->right;
    TagNode::DestroyTag(prev);
  }  
}

void TagSection::initSection(DocumentHighlighter *highlighter, const Language *language, const range_t &range, TagNode *parent) 
{
  myDocumentHighlighter = highlighter;
  myParentNode = parent;
  myLanguage = language;
  mySectionRange = getRangeFromPositions(range.start, range.start);
  myFirstTag = 0;
}

void TagSection::appendSection(const range_t &range) 
{
  if (!HighlighterTools::AreEqual(range.start, mySectionRange.end) || !HighlighterTools::IsBigger(range.end, mySectionRange.end))
    throw std::logic_error("Wrong append range");

  if (HighlighterTools::AreEqual(range.start, range.end))
    return; 

  mySectionRange.end = range.end;

  if (myLanguage->GetTagWarehouse().GetTagsParser().GetNumberOfWords() > 0)
  {
    // Section could be complex, so we need to reparse all after last properly ended tag
    range_t parseRange = range;
    TagNode *tag = myFirstTag;

    if (tag != 0)
    {
      while (tag->right != 0)
        tag = tag->right;

      ValidateTagResult validationResult = validateTag(tag);
      switch (validationResult) 
      {
      case MISSING_HEADER_TAG:
        parseRange.start = tag->getRange().end;
        break;

      case MISSING_BODY_TAG:
        parseRange.start = tag->bodyPair->getRightPosition();
        break;

      case MISSING_BODY_HEADER_TAG:
        parseRange.start = tag->bodyPair->getRange().end;
        break;
      }
    }
    else 
      parseRange.start = mySectionRange.start;

    //clock_t before = clock(), after;

    // Now we can update range according to max word length
    position_t alternativeStart;
    alternativeStart.x = range.start.x > (unsigned int)myLanguage->GetTagWarehouse().GetTagsParser().GetLengthOfTheLongestWord() ? range.start.x - myLanguage->GetTagWarehouse().GetTagsParser().GetLengthOfTheLongestWord() : 0;
    alternativeStart.y = range.start.y;
    parseRange.start = HighlighterTools::Max(parseRange.start, alternativeStart);

    // Now we should update range in order to take into account possible delimiters
    int extraCharactersBeforeBeginning = myDocumentHighlighter->ExtendStartPosition(parseRange, 1);
    int extraCharactersAfterEnd = myDocumentHighlighter->ExtendEndPosition(parseRange, 1);
  
    TextBlock *textBlock = myDocumentHighlighter->GetText(parseRange);
    AhoSearch::AhoMatches ahoMatches;

    //after = clock();
    //MessageCollector::GetInstance().prepareAhoSearch += (after - before);
    //before = after;

    HighlighterTools::GetMatches(
        myLanguage->GetTagWarehouse(),
        textBlock->GetText(),
        textBlock->GetTextLength(),
        extraCharactersBeforeBeginning,
        textBlock->GetTextLength() - extraCharactersAfterEnd,
        ahoMatches
    );

    //after = clock();
    //MessageCollector::GetInstance().searchAhoTime += (after - before);
    //before = after;

    // Now get Matches from AhoMathes

    std::vector<Match> matches;
    position_t matchStartPosition = textBlock->GetRange().start, matchEndPosition;
    const wchar_t *matchInText = textBlock->GetText();
    int prevMatchOffset = 0;

    for (AhoSearch::AhoMatches::const_iterator iAhoMatch = ahoMatches.begin(); iAhoMatch != ahoMatches.end(); ++iAhoMatch)
    {
      matchStartPosition = HighlighterTools::GetPositionFromOffset(matchInText, myDocumentHighlighter->GetLineBreak(), matchStartPosition, iAhoMatch->pos - prevMatchOffset);
      matchInText = &matchInText[iAhoMatch->pos - prevMatchOffset];
      prevMatchOffset = iAhoMatch->pos;
      matchEndPosition = HighlighterTools::GetPositionFromOffset(matchInText, myDocumentHighlighter->GetLineBreak(), matchStartPosition, iAhoMatch->length);
      matches.push_back(Match(matchStartPosition, matchEndPosition, iAhoMatch->pos, iAhoMatch->param));    
    }

    //after = clock();
    //MessageCollector::GetInstance().transformAhoTime += (after - before);

    appendTagTree(matches, textBlock);

    delete textBlock;
  }
}

bool TagSection::updateSectionOnChange(__in const range_t &range, __out TagUpdateResult &updateResult) 
{
  // try to find subsection which contains modification and update it
  TagSection *section = getAppropriateSection(range.start);
  if (0 != section && section->updateSectionOnChange(range, updateResult)) 
    return true;

  // test if changes affect only on current section. (We do not need to go to the parents)
  if (!validateSubSection(range))
    return false;

  // All changes affect on current section, its tags and subsections. Try to update current section  

  TagNode *tag = getAppropriateTag(range.start);  // tag which contains modification

  range_t sectionRange;
  TagNode *previousNode = 0, *nextNode = 0;  // new Section would be between them

  if (0 != tag) 
  {
    // We have to parse string for this tag again (this tag will be removed)
    previousNode = tag->left;
    nextNode = tag->right;

    sectionRange.start = tag->getLeftPosition();
    sectionRange.end = nextNode != 0 ? nextNode->getRange().start : mySectionRange.end;

    removeTag(tag);

    if (tag->isOpen || tag->description->IsComplex())
    {
      // we just remove tag with own subsection(s). Probably we are going to parse section from tag start to section end
      myDocumentHighlighter->CacheText(getRangeFromPositions(sectionRange.start, mySectionRange.end), false);
    }
  } 
  else
  {
    // We have to parse string between tags

    std::pair<TagNode*, TagNode*> outerTags = findOuterTags(range);
    previousNode = outerTags.first;
    nextNode = outerTags.second;

    sectionRange = range;

    range_t outerRange;
    outerRange.start = previousNode != 0 ? previousNode->getMostRightPosition() : mySectionRange.start;
    outerRange.end = nextNode != 0 ? nextNode->getRange().start : mySectionRange.end;

    if (outerRange.start.y == range.start.y)
      sectionRange.start = outerRange.start;
    else
      sectionRange.start.x = 0;

    if (outerRange.end.y == range.end.y)
      sectionRange.end = outerRange.end;
    else
      sectionRange.end.x = myDocumentHighlighter->GetStringLength(sectionRange.end.y);

    // check that next node remains correct ( for example: [... /*] -> [... //*] )
    if (nextNode != 0 && !checkTagSequence(nextNode))
    {
      TagNode *toDelete = nextNode;
      nextNode = nextNode->right;
      sectionRange.end = nextNode != 0 ? nextNode->getRange().start : mySectionRange.end;
      removeTag(toDelete);
    }
  }

  // Now we have nodes which surround changed area
  ApplyAuxiliarySectionResult applyResult;
  applyResult.stopTag = nextNode;

  TagNode *toDelete;
  TagSection *subsection = 0;

  do
  {
    if (0 == subsection)
    {    
      subsection = new TagSection(
          myDocumentHighlighter,
          GetLanguage(),
          sectionRange,
          GetSectionParent()
      );
    } 
    else
      subsection->appendSection(getRangeFromPositions(subsection->GetRange().end, sectionRange.end));

    applyResult = applyAuxiliarySection(previousNode, nextNode, *subsection);

    updateResult.section = this;
    updateResult.range = subsection->GetRange();

    if (!applyResult.success)
    {
      while (nextNode != 0 && nextNode != applyResult.stopTag)
      {
        toDelete = nextNode;
        nextNode= nextNode->right;
        removeTag(toDelete);
      }

      if (applyResult.stopTag != 0) 
      {
        nextNode = applyResult.stopTag->right;
        removeTag(applyResult.stopTag);
        sectionRange.end = nextNode != 0 ? nextNode->getLeftPosition() : mySectionRange.end;
      }
      else
        sectionRange.end = mySectionRange.end;
    }
  } while (!applyResult.success);

  if (subsection != 0) // paranoia
    delete subsection;

  return true;
}

bool TagSection::updateSectionOnInsert(__in const range_t &range, __out TagUpdateResult &updateResult)
{
  updateSectionsPositionsOnChange(myParentNode, range, false);
  myDocumentHighlighter->CacheText(range);
  return updateSectionOnChange(range, updateResult);
}

bool TagSection::updateSectionOnDelete(__in const range_t &range, __out TagUpdateResult &result) 
{
  // remove damaged tags
  range_t affectedRange;
  TagSection *lastAffectedSection = GetAppropriateSection(range);
  std::pair<TagNode*, TagNode*> outerTags = removeAffectedTags(range, lastAffectedSection, affectedRange);
  
  // update positions
  updateSectionsPositionsOnChange(myParentNode, range, true); 
  updatePosition(affectedRange.end, range, true, false);

  // calculate outer range
  range_t outerRange;
  outerRange.start = outerTags.first != 0 ? outerTags.first->getMostRightPosition() : lastAffectedSection->GetRange().start;
  outerRange.end = outerTags.second != 0 ? outerTags.second->getRange().start : lastAffectedSection->GetRange().end;

  if (outerRange.start.y == affectedRange.start.y)
    affectedRange.start = outerRange.start;
  else
    affectedRange.start.x = 0;

  if (outerRange.end.y == affectedRange.end.y)
    affectedRange.end = outerRange.end;
  else
    affectedRange.end.x = myDocumentHighlighter->GetStringLength(affectedRange.end.y);
  
  return updateSectionOnChange(affectedRange, result);
}

// finds appropriate section for the given range and deletes affected tags
std::pair<TagNode*, TagNode*> TagSection::removeAffectedTags(__in const range_t &range, __inout TagSection *lastAffectedSection, __out range_t &affectedRange)
{
  // Now we must delete all tags (and sections) which have not empty intersection with deleted range
  TagNode *sectionTag = lastAffectedSection->GetFirstTag(), *toDelete = 0;
  affectedRange = range;

  // Note that we are deleting only damaged tags (because if delete range contained inside tag body, lastAffectedSection equals to tag body)
  while (0 != sectionTag && HighlighterTools::IsLesser(sectionTag->getMostLeftPosition(), range.end, true)) 
  {
    if (HighlighterTools::IsBigger(sectionTag->getMostRightPosition(), range.start, true)) 
    {
      if (toDelete == 0) 
      {
        // toDelete = 0 => it is the first tag we are deleting
        if (HighlighterTools::IsLesser(sectionTag->getRange().start, affectedRange.start, true))
          affectedRange.start = sectionTag->getRange().start;
      }
      
      toDelete = sectionTag;

      if (HighlighterTools::IsBigger(sectionTag->getMostRightPosition(), affectedRange.end, true))
        affectedRange.end = sectionTag->getMostRightPosition();

      sectionTag = sectionTag->right;

      lastAffectedSection->removeTag(toDelete);
    } 
    else 
      sectionTag = sectionTag->right;
  }

  return lastAffectedSection->findOuterTags(range);
}

// finds outer tags for the range
std::pair<TagNode*, TagNode*> TagSection::findOuterTags(__in const range_t &range)
{
  TagNode *tag, *left = 0, *right = 0;
  tag = GetFirstTag();

  while (0 != tag && HighlighterTools::IsLesser(tag->getMostRightPosition(), range.start, false))
  {
    left = tag;
    tag = tag->right;
  }

  if (left != 0 && !HighlighterTools::IsLesser(left->getMostRightPosition(), range.start, false))
    left = 0;

  while (0 != tag && HighlighterTools::IsLesser(tag->getRange().start, range.end, true))
    tag = tag->right;
  right = tag;

  return std::pair<TagNode*, TagNode*>(left, right);
}

// checks if tag has appropriate surrounding 
bool TagSection::checkTagSequence(const TagNode *tag)
{
  return checkTagSequenceOnDelimiters(tag) && checkTagSequenceOnConflicts(tag);
}

bool TagSection::checkTagSequenceOnDelimiters(const TagNode *tag)
{
  bool retval = true;

  range_t extendedRange = tag->getRange();

  // Now we should update range in order to take into account possible delimiters
  int extraCharactersBeforeBeginning = myDocumentHighlighter->ExtendStartPosition(extendedRange, 1);
  int extraCharactersAfterEnd = myDocumentHighlighter->ExtendEndPosition(extendedRange, 1);

  if (extraCharactersBeforeBeginning + extraCharactersAfterEnd > 0)
  {
    TextBlock *text = myDocumentHighlighter->GetText(extendedRange);
    if (extraCharactersBeforeBeginning > 0)
    {
      if (!tag->description->IsLeftDelimiter(text->GetText()[0])) 
        retval = false;
    }

    if (extraCharactersAfterEnd > 0)
    {
      if (!tag->description->IsRightDelimiter(text->GetText()[text->GetTextLength() - 1]))
        retval = false;
    }
    delete text;
  }

  return retval;
}

bool TagSection::checkTagSequenceOnConflicts(const TagNode *tag)
{
  bool retval = true;

  const TagWarehouse &warehouse = myLanguage->GetTagWarehouse();
  const AhoSearch &parser = warehouse.GetTagsParser();

  range_t extendedRange = extendRange(tag->getRange(), parser.GetLengthOfTheLongestWord(), tag->left, tag->right);
  int tagOffset = tag->getRange().start.x - extendedRange.start.x;

  if (tagOffset > 0)
  {
    TextBlock *text = myDocumentHighlighter->GetText(extendedRange);

    // Now we are need to search tags in the next area:
    // ...>......<tag ...> ...
    //        |_____| - text before tag + tag body
    AhoSearch::AhoMatches matches;
    HighlighterTools::GetMatches(warehouse, text->GetText(), text->GetTextLength(), 0, tagOffset + tag->getRange().end.x - tag->getRange().start.x, matches);

    if (matches.size() > 1)
    {
      AhoSearch::AhoMatches::const_iterator iter = matches.end();
      iter -= 2;
      if (iter->pos + iter->length > tagOffset)
      {
        // Intersection. Now we need to determine if this tag is open or complex
        const TagDescription *tagDescription = myLanguage->GetTagWarehouse().GetTagDescription(iter->param);
        assert(tagDescription);
        retval = !(tagDescription->IsComplex() || tagDescription->CanBeOpen());  
      }
    }

    delete text;
  }

  return retval;
}

// Method ensures that section doesn't contain closing tag for section's parent tag
bool TagSection::validateSubSection(const range_t &range) {
  bool result = true;

  TagNode *parent = GetSectionParent();

  if (0 != parent) 
  {
    // Extend modified range
    const TagWarehouse &tagWarehouse = parent->parent->GetLanguage()->GetTagWarehouse();
    bool isHeaderSection = parent->header == this;

    range_t parseRange = extendRange(range, tagWarehouse.GetTagsParser().GetLengthOfTheLongestWord(), 0, 0);
    int extraCharactersBeforeBeginning = myDocumentHighlighter->ExtendStartPosition(parseRange, 1);
    int extraCharactersAfterEnd = myDocumentHighlighter->ExtendEndPosition(parseRange, 1);

    TextBlock *textBlock = myDocumentHighlighter->GetText(parseRange);


    // Here, probably, must be used function checkTagSequence!

    // Check parent tag's borders on correctness
    if (HighlighterTools::AreEqual(mySectionRange.start, range.start))
    {
      wchar_t character = textBlock->GetText()[mySectionRange.start.x - textBlock->GetRange().start.x];

      if (parent->description->IsComplex())
      {
        if (isHeaderSection && !parent->description->IsRightDelimiter(character))
          result = false;
        else if (!isHeaderSection && !parent->headerPair->description->IsRightDelimiter(character))
          result = false;
      }
      else if (!parent->description->IsRightDelimiter(character))
        result = false;
    }

    if (result)
    {
      if (HighlighterTools::AreEqual(mySectionRange.end, range.end))
      {
        wchar_t character = textBlock->GetText()[mySectionRange.end.x - textBlock->GetRange().start.x - 1];

        // No need to check if this is complex tag or not, because we are going to check "closing tag"

        if (isHeaderSection && !parent->headerPair->description->IsLeftDelimiter(character))
          result = false;
        else if (isHeaderSection && parent->description->GetHeaderPair(*parent->headerPair->description)->IsEscapeCharacter(character))
          result = false;
        else if (!isHeaderSection && !parent->bodyPair->description->IsLeftDelimiter(character))
          result = false;
        else if (!isHeaderSection && parent->description->GetBodyPair(*parent->bodyPair->description)->IsEscapeCharacter(character))
          return false;
      }
    }

    // Check if there is ending tag in the range
    if (result)
    {    
      AhoSearch::AhoMatches matches;

      HighlighterTools::GetMatches(
        tagWarehouse,
        textBlock->GetText(),
        textBlock->GetTextLength(),
        extraCharactersBeforeBeginning,
        textBlock->GetTextLength() - extraCharactersAfterEnd,
        matches
      );

      const TagDescription *description;

      for (AhoSearch::AhoMatches::const_iterator iMatch = matches.begin(); iMatch != matches.end(); ++iMatch) 
      {
        description = tagWarehouse.GetTagDescription(iMatch->param);
        if (!TagWarehouse::IsTagDescriptionEmbedded(*description))
        {
          if (isHeaderSection) 
          {
            if (parent->description->IsHeaderPair(*description))  
            {
              result = false;
              break;
            }
          }
          else
          {
            if (parent->description->IsBodyPair(*description))  
            {
              result = false;
              break;
            }
          }
        }
      }
    }

    delete textBlock;    
  }

  return result;
}

range_t TagSection::extendRange(const range_t &range, unsigned int lengthOfTheLongestWord, const TagNode *left, const TagNode *right)
{
  range_t limiter;

  if (left != 0)
    limiter.start = left->getMostRightPosition();
  else
    limiter.start = mySectionRange.start;

  if (right != 0)
    limiter.end = right->getRange().start;
  else
    limiter.end = mySectionRange.end;

  return myDocumentHighlighter->ExtendRange(range, lengthOfTheLongestWord, &limiter);
}

TagSection::ApplyAuxiliarySectionResult TagSection::applyAuxiliarySection(TagNode *leftNode, TagNode *rightNode, TagSection &auxSection)
{
  ApplyAuxiliarySectionResult result;

  ValidateTagResult validationResult = SUCCESS;

  // we need to test subsection if it ends before our section end
  if (!HighlighterTools::AreEqual(auxSection.GetEndPosition(), mySectionRange.end))
  {
    // Test if we can just insert new tags into current section. (Changes have no affect on current section)
    validationResult = validateSection(auxSection);
  }

  if (SUCCESS == validationResult) 
  {
    prepareAuxiliarySection(auxSection);

    TagNode *tag = auxSection.GetFirstTag();

    if (0 != tag) 
    {
      if (0 != leftNode) 
      {
        leftNode->right = tag;
        tag->left = leftNode;
      } 
      else 
        myFirstTag = tag;

      if (0 != rightNode) 
      {
        while (0 != tag->right) 
          tag = tag->right;

        tag->right = rightNode;
        rightNode->left = tag;
      }
    }

    auxSection.myFirstTag = 0;  // we have moved all tags to the current section

    result.success = true;
    result.stopTag = rightNode;
  }
  else
  {
    result.success = false;

    TagNode *notValidTag = auxSection.GetFirstTag();
    while (notValidTag->right != 0)
      notValidTag = notValidTag->right;

    TagNode *tag = rightNode;
    if (MISSING_HEADER_TAG == validationResult)
    {
      while (tag != 0 && !tag->ContainsHeaderPairForTag(*notValidTag))
        tag = tag->right;
    } 
    else if (MISSING_BODY_TAG == validationResult)
    {
      while (tag != 0 && !tag->ContainsBodyPairForTag(*notValidTag))
        tag = tag->right;
    }
    else // MISSING_BODY_HEADER_TAG
    {
      while (tag != 0 && !tag->ContainsHeaderPairForTag(*notValidTag->bodyPair))
        tag = tag->right;
    }
    
    result.stopTag = tag;
  }

  return result;
}

// Checks if tag properly closed or not
TagSection::ValidateTagResult TagSection::validateTag(const TagNode *tag)
{
  if (0 != tag) 
  {
    if (tag->description->IsComplex() && !tag->description->IsProperHeaderPair(*tag->headerPair->description)) 
    {
      if (HighlighterTools::IsBigger(mySectionRange.end, tag->getMostRightPosition(), true)) 
        return MISSING_HEADER_TAG;
    }

    if (tag->isOpen) 
    {
      if (!tag->description->IsProperBodyPair(*tag->bodyPair->description)) 
      {
        if (HighlighterTools::IsBigger(mySectionRange.end, tag->getMostRightPosition(), true)) 
          return MISSING_BODY_TAG;
      }
      if (tag->bodyPair->description->IsComplex() && !tag->bodyPair->description->IsProperHeaderPair(*tag->bodyPair->headerPair->description)) 
      {
        if (HighlighterTools::IsBigger(mySectionRange.end, tag->getMostRightPosition(), true)) 
          return MISSING_BODY_HEADER_TAG;                    
      }
    }
  }

  return SUCCESS;
}

// Checks if section is properly closed or not
TagSection::ValidateTagResult TagSection::validateSection(TagSection &subsection) 
{
  TagNode *subsectionTag = subsection.GetFirstTag();

  if (0 != subsectionTag) 
  {
    while (0 != subsectionTag->right) 
      subsectionTag = subsectionTag->right;

    return validateTag(subsectionTag);
  }

  return SUCCESS;
}

// function changes parent in all subsection tags
void TagSection::prepareAuxiliarySection(TagSection &subsection)
{
  TagNode *subsectionTag = subsection.GetFirstTag();
  while (subsectionTag != 0)
  {
    subsectionTag->parent = this;
  
    if (subsectionTag->headerPair != 0)
      subsectionTag->headerPair->parent = this;

    if (subsectionTag->bodyPair != 0)
    {
      subsectionTag->bodyPair->parent = this;
      if (subsectionTag->bodyPair->headerPair != 0)
        subsectionTag->bodyPair->headerPair->parent = this;
    }

    subsectionTag = subsectionTag->right;
  }
}

void TagSection::updateSectionsPositionsOnChange(TagNode *parent, const range_t &range, bool isDeleteModification) 
{
  if (HighlighterTools::IsBigger(mySectionRange.end, range.start))
  {
    if (parent != 0) 
    {
      if (parent->header == this)
        mySectionRange.start = parent->range.end;
      else          
        mySectionRange.start = parent->getRightPosition();        
    } 
    else
      updatePosition(mySectionRange.start, range, isDeleteModification, true);

    updatePosition(mySectionRange.end, range, isDeleteModification, false);

    TagNode *tag = GetFirstTag();

    while (0 != tag) 
    {
      updateTagOffsetsOnChange(tag, range, isDeleteModification);
      tag = tag->right;
    }
  }
}

void TagSection::updateTagOffsetsOnChange(TagNode *tag, const range_t &range, bool isDeleteModification) 
{
  if (HighlighterTools::IsBigger(tag->range.start, range.start)) 
  {
    updatePosition(tag->range.start, range, isDeleteModification, false);
    updatePosition(tag->range.end, range, isDeleteModification, false);
  }

  if (tag->description->IsComplex()) 
  {
    updateTagOffsetsOnChange(tag->headerPair, range, isDeleteModification);
    tag->header->updateSectionsPositionsOnChange(tag, range, isDeleteModification);
  }

  if (tag->isOpen) 
  {
    updateTagOffsetsOnChange(tag->bodyPair, range, isDeleteModification);
    tag->body->updateSectionsPositionsOnChange(tag, range, isDeleteModification);
  }
}

void TagSection::updatePosition(position_t &pos, const range_t &modification, bool isDeleteModification, bool strictInequality)
{
  if (HighlighterTools::IsBigger(pos, modification.start, strictInequality)) 
  {
    int diffY, diffX;

    diffY = modification.end.y - modification.start.y;
    diffX = modification.end.x - modification.start.x;  // works for both cases (diffY > 0 and diffY = 0)

    if (!isDeleteModification)
    {
      if (pos.y == modification.start.y)
        pos.x += diffX;
      pos.y += diffY;
    }
    else
    {
      if (pos.y == modification.end.y)
        pos.x -= diffX;
      pos.y -= diffY;
    }
  }
}

TagSection* TagSection::GetAppropriateSection(const range_t &range, TagNode *startTag)
{
  if (isSectionContains(range.start) && isSectionContains(range.end)) 
  {
    TagSection *startSection = getAppropriateSection(range.start, startTag);
    TagSection *endSection = getAppropriateSection(range.end, startTag);

    if (!startSection || !endSection || startSection != endSection) 
      return this;

    return startSection->GetAppropriateSection(range);
  }
  return 0;
}

TagSection* TagSection::getAppropriateSection(const position_t &pos, TagNode *startTag) const
{
  TagNode *tag = (startTag != 0 && startTag->parent == this) ? startTag : GetFirstTag();

  if (tag)
  {
    bool rightDirection = HighlighterTools::IsLesser(tag->getRange().start, pos);

    while (0 != tag) 
    {
      // some exit conditions
      if (rightDirection && HighlighterTools::IsBigger(tag->getRange().start, pos, true))
        return 0;
      else if (!rightDirection && HighlighterTools::IsLesser(tag->getMostRightPosition(), pos, true))
        return 0;


      if (tag->headerPair != 0)
      {
        if (tag->header->isSectionContains(pos))
          return tag->header;

        if (tag->bodyPair != 0)
        {
          if (tag->description->IsProperHeaderPair(*tag->headerPair->description) && tag->body->isSectionContains(pos))
            return tag->body;
        }
      }
      else  if (tag->bodyPair != 0)
      {
        if (tag->body->isSectionContains(pos))
          return tag->body;
      }

      tag = rightDirection ? tag->right : tag->left;
    }
  }

  return 0;
}

TagNode* TagSection::getAppropriateTag(const position_t &pos, TagNode *startTag) const 
{
  TagNode *tag = (startTag != 0 && startTag->parent == this) ? startTag : GetFirstTag();

  if (tag)
  {
    bool rightDirection = HighlighterTools::IsLesser(tag->getRange().start, pos);

    while (0 != tag) 
    {
      // some exit conditions
      if (rightDirection && HighlighterTools::IsBigger(tag->getRange().start, pos, true))
        return 0;
      else if (!rightDirection && HighlighterTools::IsLesser(tag->getMostRightPosition(), pos, true))
        return 0;

      if (HighlighterTools::IsLesser(tag->getRange().start, pos, true) && HighlighterTools::IsBigger(tag->getMostRightPosition(), pos, true)) 
        return tag;

      tag = rightDirection ? tag->right : tag->left;
    }
  }

  return 0;
}

void TagSection::removeTag(TagNode *tag)
{
  if (myFirstTag == tag) 
    myFirstTag = tag->right;
  tag->removeNode();
  TagNode::DestroyTag(tag);
}

bool TagSection::isSectionContains(const position_t &pos) const
{
  return HighlighterTools::RangeContains(mySectionRange, pos);
}

void TagSection::appendTagTree(const std::vector<Match> &matches, const TextBlock *textBlock) 
{
  TagNode *currentTagNode = myFirstTag, *tagNode = 0;
  unsigned int currentMatch = 0;

  if (currentTagNode != 0)
  {
    while (currentTagNode->right != 0)
      currentTagNode = currentTagNode->right;
  }

  ValidateTagResult validationResult;
  while ((validationResult = validateTag(currentTagNode)) != SUCCESS)
  {
    switch (validationResult) 
    {
    case MISSING_HEADER_TAG:
      assert(currentTagNode->headerPair != 0);
      TagNode::DestroyTag(currentTagNode->headerPair);
      currentTagNode->headerPair = 0;
      finishComplexTag(matches, currentMatch, currentTagNode, textBlock);
      break;

    case MISSING_BODY_TAG:
      assert(currentTagNode->bodyPair != 0 && currentTagNode->isOpen);
      TagNode::DestroyTag(currentTagNode->bodyPair);
      currentTagNode->bodyPair = 0;
      finishOpenTag(matches, currentMatch, currentTagNode, textBlock);
      break;

    case MISSING_BODY_HEADER_TAG:
      assert(currentTagNode->bodyPair != 0 && currentTagNode->bodyPair->description->IsComplex());
      TagNode::DestroyTag(currentTagNode->bodyPair->headerPair);
      currentTagNode->bodyPair->headerPair = 0;
      finishComplexTag(matches, currentMatch, currentTagNode->bodyPair, textBlock);
      break;
    }
  }  

  while ((tagNode = createNextTag(matches, currentMatch, currentTagNode, textBlock)) != 0) 
  {
    if (0 != currentTagNode) 
    {
      currentTagNode->right = tagNode;
      tagNode->left = currentTagNode;
    } 
    else 
      myFirstTag = tagNode;
          
    currentTagNode = tagNode;
  }
}

TagNode* TagSection::createNextTag(const std::vector<Match> &matches, unsigned int &currentMatch, TagNode *previous, const TextBlock *textBlock) 
{
  while (currentMatch < matches.size()) 
  {
    const Match &match = matches[currentMatch++];
    const TagDescription *description = myLanguage->GetTagWarehouse().GetTagDescription(match.param);

    // Check if new tag is complex (has header) or is open (has body).
    if (description->CanBeOpen() || description->IsComplex())
    {
      // Check match on intersection with previous tag
      if (!previous || !HighlighterTools::IsBigger(previous->getMostRightPosition(), match.range.start, true))
      {
        // Create and fill the new node
        TagNode *tag = new TagNode(this, description);
        tag->range = match.range;

        if (description->IsComplex())
          finishComplexTag(matches, currentMatch, tag, textBlock);

        tag->isOpen = description->CanBeOpen();  // we suppose that new tag is open

        // Now we should try to find a pair for this tag if it is an open tag
        if (tag->isOpen) 
          finishOpenTag(matches, currentMatch, tag, textBlock);

        return tag;
      }
    }
  }

  return 0;
}

void TagSection::finishOpenTag(const std::vector<Match> &matches, unsigned int &currentMatch, TagNode *tagNode, const TextBlock *textBlock) 
{
  TagNode *closingTagNode = createClosingTag(matches, currentMatch, tagNode, textBlock);

  tagNode->bodyPair = closingTagNode;
  closingTagNode->bodyPair = tagNode;

  if (!tagNode->body)
  { 
    tagNode->body = new TagSection(
        myDocumentHighlighter,
        *tagNode->description->bodyLanguage,
        getRangeFromPositions(tagNode->getRightPosition(), closingTagNode->getRange().start),
        tagNode
    );
  }
  else 
  {
    tagNode->body->appendSection(getRangeFromPositions(tagNode->body->GetRange().end, closingTagNode->getRange().start));
  }
}

TagNode* TagSection::createClosingTag(const std::vector<Match> &matches, unsigned int &currentMatch, TagNode *tagNode, const TextBlock *textBlock) 
{
  // if header finished not in proper way, finish body with the embedded tag
  if (tagNode->headerPair != 0 && !tagNode->description->IsProperHeaderPair(*tagNode->headerPair->description))
  {        
    return new TagNode(
      false, 
      0, 0, 
      0, 0, 
      0, 0, 
      getRangeFromPositions(tagNode->getRightPosition(), tagNode->getRightPosition()), 
      this, 
      myLanguage->GetTagWarehouse().GetTagDescription(&TagWarehouse::TAG_EMBEDDED)
    );
  }
  else 
  {  
    while (currentMatch < matches.size()) 
    {
      const Match &match = matches[currentMatch++];
      const TagDescription *tagDescription = myLanguage->GetTagWarehouse().GetTagDescription(match.param);
      const TagPair *pair = tagNode->description->GetBodyPair(*tagDescription);

      if (pair)
      {
        if (!HighlighterTools::AreIntersect(tagNode->range, match.range, true))
        {
          if (!(match.offset > 0 && pair->IsEscapeCharacter(textBlock->GetText()[match.offset - 1])))
          {          
            // We have found closing tag
            TagNode *closingTagNode = new TagNode(false, 0, 0, 0, 0, 0, 0, match.range, this, tagDescription);

            if (closingTagNode->description->IsComplex()) 
              finishComplexTag(matches, currentMatch, closingTagNode, textBlock);

            return closingTagNode;
          }
        }
      }
    }

    return new TagNode(false, 0, 0, 0, 0, 0, tagNode, getRangeFromPositions(mySectionRange.end, mySectionRange.end), this, myLanguage->GetTagWarehouse().GetTagDescription(&TagWarehouse::TAG_EMBEDDED));
  }
}

void TagSection::finishComplexTag(const std::vector<Match> &matches, unsigned int &currentMatch, TagNode *tagNode, const TextBlock *textBlock) 
{
  // We have to find body suffix
  TagNode *suffixTagNode = createSuffixTag(matches, currentMatch, tagNode, textBlock);

  tagNode->headerPair = suffixTagNode;
  suffixTagNode->headerPair = tagNode;

  if (!tagNode->header)
  {
    range_t range = getRangeFromPositions(tagNode->getRange().end, suffixTagNode->getRange().start);
    tagNode->header = new TagSection(
            myDocumentHighlighter,
            *tagNode->description->headerLanguage,
            range,
            tagNode
    );
  }
  else
  {
    tagNode->header->appendSection(getRangeFromPositions(tagNode->header->GetRange().end, suffixTagNode->getRange().start));
  }
}

TagNode* TagSection::createSuffixTag(const std::vector<Match> &matches, unsigned int &currentMatch, TagNode *tagNode, const TextBlock *textBlock) 
{
  while (currentMatch < matches.size()) 
  {
    const Match &match = matches[currentMatch++];
    const TagDescription *tagDescription = myLanguage->GetTagWarehouse().GetTagDescription(match.param);
    const TagPair *pair = tagNode->description->GetHeaderPair(*tagDescription);

    if (pair) 
    {
      if (!HighlighterTools::AreIntersect(tagNode->range, match.range, true))
      {      
        if (!(match.offset > 0 && pair->IsEscapeCharacter(textBlock->GetText()[match.offset - 1])))
        {
          // We have found suffix tag
          return new TagNode(false, 0, 0, 0, 0, 0, 0, match.range, this, tagDescription);
        }
      }
    }
  }

  return new TagNode(false, 0, 0, 0, 0, 0, 0, getRangeFromPositions(mySectionRange.end, mySectionRange.end), this, myLanguage->GetTagWarehouse().GetTagDescription(&TagWarehouse::TAG_EMBEDDED));
}

range_t TagSection::getRangeFromCoordinates(int x1, int y1, int x2, int y2) const
{
  range_t range;
  range.start.x = x1;
  range.start.y = y1;
  range.end.x = x2;
  range.end.y = y2;
  return range;
}

range_t TagSection::getRangeFromPositions(const position_t &pos1, const position_t &pos2) const
{
  range_t range;
  range.start = pos1;
  range.end = pos2;
  return range;
}