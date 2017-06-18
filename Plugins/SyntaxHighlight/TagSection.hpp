#ifndef TagSection_h__
#define TagSection_h__

//#ifdef _DEBUG
//#include "vld.h"
//#endif

#include "Language.hpp"
#include "TextBlock.hpp"

#include <vector>
#include <list>

class DocumentHighlighter;


class TagSection 
{
public:

  struct TagUpdateResult 
  {
    range_t range;
    TagSection *section;

    TagUpdateResult() : section(0) {};
    
    TagUpdateResult(TagSection *_section, const range_t &_range) : section(_section), range(_range) {}
  };


private:

  struct Match {
    range_t range;
    int offset;
    int param;

    Match(const position_t &_start, const position_t &_end, int _offset, int _param) : offset(_offset), param(_param) 
    {
      range.start = _start;
      range.end = _end;
    }
  };

  enum ValidateTagResult {
    SUCCESS, 
    MISSING_HEADER_TAG,      // missing header pair.
    MISSING_BODY_TAG,        // missing body pair
    MISSING_BODY_HEADER_TAG  // missing header pair for body pair
  };

  struct ApplyAuxiliarySectionResult {
    bool success;
    TagNode *stopTag;
  };


private:

  // DocumentHighlighter which contains this section
  DocumentHighlighter *myDocumentHighlighter;

  // Language which is used to parse string with this class
  const Language *myLanguage;

  // Node which contains this section
  TagNode *myParentNode;

  // Head of the list of tags
  TagNode *myFirstTag;

  // Section position
  range_t mySectionRange;


public:

  TagSection(DocumentHighlighter *highlighter, const std::wstring &language, const range_t &position, TagNode *parent);

  TagSection(DocumentHighlighter *highlighter, const Language *language, const range_t &position, TagNode *parent);

  TagNode* GetSectionParent() const;

  TagNode* GetFirstTag() const;

  const range_t& GetRange() const;

  position_t GetStartPosition() const;

  position_t GetEndPosition() const;

  const Language* GetLanguage() const;

  TagSection* GetAppropriateSection(const range_t &modification, TagNode *startTag = 0);

  bool UpdateSections(const range_t &range, bool isDelete, TagUpdateResult &updateResult);

  ~TagSection();


private:

  void initSection(DocumentHighlighter *highlighter, const Language *language, const range_t &position, TagNode *parent);

  void appendSection(const range_t &range);

  bool updateSectionOnChange(__in const range_t &range, __out TagUpdateResult &updateResult);

  bool updateSectionOnInsert(__in const range_t &range, __out TagUpdateResult &updateResult);

  bool updateSectionOnDelete(__in const range_t &range, __out TagUpdateResult &result);

  std::pair<TagNode*, TagNode*> removeAffectedTags(__in const range_t &range, __inout TagSection *lastAffectedSection, __out range_t &affectedRange);

  std::pair<TagNode*, TagNode*> findOuterTags(__in const range_t &range);

  bool checkTagSequence(const TagNode *tag);

  bool checkTagSequenceOnDelimiters(const TagNode *tag);

  bool checkTagSequenceOnConflicts(const TagNode *tag);

  bool validateSubSection(const range_t &range);

  range_t extendRange(const range_t &range, unsigned int lengthOfTheLongestWord, const TagNode *left, const TagNode *right);

  ApplyAuxiliarySectionResult applyAuxiliarySection(TagNode *leftNode, TagNode *rightNode, TagSection &auxSection);

  ValidateTagResult validateSection(TagSection &subsection);

  ValidateTagResult validateTag(const TagNode *tag);

  void prepareAuxiliarySection(TagSection &subsection);

  void updateSectionsPositionsOnChange(TagNode *parent, const range_t &range, bool isDeleteModification);

  void updateTagOffsetsOnChange(TagNode *tag, const range_t &range, bool isDeleteModification);

  void updatePosition(position_t &pos, const range_t &modification, bool isDeleteModification, bool strictInequality);

  TagSection* getAppropriateSection(const position_t &pos, TagNode *startTag = 0) const;

  TagNode* getAppropriateTag(const position_t &pos, TagNode *startTag = 0) const;

  void removeTag(TagNode *tag);

  bool isSectionContains(const position_t &pos) const;

  void appendTagTree(const std::vector<Match> &matches, const TextBlock *textBlock);

  TagNode* createNextTag(const std::vector<Match> &matches, unsigned int &currentMatch, TagNode *previous, const TextBlock *textBlock);

  void finishOpenTag(const std::vector<Match> &matches, unsigned int &currentMatch, TagNode *tagNode, const TextBlock *textBlock);

  TagNode *createClosingTag(const std::vector<Match> &matches, unsigned int &currentMatch, TagNode *tagNode, const TextBlock *textBlock);

  void finishComplexTag(const std::vector<Match> &matches, unsigned int &currentMatch, TagNode *tagNode, const TextBlock *textBlock);

  TagNode* createSuffixTag(const std::vector<Match> &matches, unsigned int &currentMatch, TagNode *tagNode, const TextBlock *textBlock);

  range_t getRangeFromCoordinates(int x1, int y1, int x2, int y2) const;

  range_t getRangeFromPositions(const position_t &pos1, const position_t &pos2) const;
};

#endif // TagSection_h__
