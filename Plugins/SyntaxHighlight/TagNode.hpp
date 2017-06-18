#ifndef TagNode_h__
#define TagNode_h__

#include <string>
#include "HighlighterTools.hpp"
#include "TagDescription.hpp"


class TagSection;


struct TagNode 
{
public:

  TagSection *parent;

  TagSection *body;
  TagNode *bodyPair;

  TagSection *header;
  TagNode *headerPair;

  TagNode *left;
  TagNode *right;

  range_t range;
  const TagDescription *description;

  bool isOpen;


public:

  TagNode(TagSection *parent, const TagDescription *description) 
  {
    this->parent = parent;
    this->description = description;
    
    this->isOpen = false;
    this->header = 0;
    this->body = 0;
    this->bodyPair = 0;
    this->headerPair = 0;
    this->left = 0;
    this->right = 0;
    memset(&range, 0, sizeof(range_t));
  }

  TagNode(bool isOpen,
          TagSection *body,
          TagSection *header,
          TagNode *left,
          TagNode *right,
          TagNode *headerPair,
          TagNode *bodyPair,
          const range_t &range,
          TagSection *parent,
          const TagDescription *description)
  {
    this->isOpen = isOpen;
    this->header = header;
    this->body = body;
    this->bodyPair = bodyPair;
    this->headerPair = headerPair;
    this->parent = parent;
    this->description = description;
    this->range = range;
    this->left = left;
    this->right = right;
  }

  const position_t& getLeftPosition() const
  {
    if (headerPair) 
      return HighlighterTools::Min(range.start, headerPair->range.start); 
    return range.start;
  }

  const position_t& getRightPosition() const
  {
    if (headerPair)
      return HighlighterTools::Max(range.end, headerPair->range.end);
    return range.end;
  }

  const position_t& getMostRightPosition() const 
  {
    const position_t &rightPosition = getRightPosition();

    if (bodyPair)
      return HighlighterTools::Max(rightPosition, bodyPair->getRightPosition());
    return rightPosition;
  }

  const position_t& getMostLeftPosition() const 
  {
    const position_t &leftPosition = getLeftPosition();

    if (bodyPair)
      return HighlighterTools::Min(leftPosition, bodyPair->getLeftPosition());
    return leftPosition;
  }

  const range_t& getRange() const
  {
    return range;
  }

  void insertNodeAfter(TagNode *node)
  {
    if (right) 
    {
      right->left = node;
      node->right = right;
    }

    node->left = this;
    right = node;
  }

  void removeNode() 
  {
    if (left)
      left->right = right;
    if (right)
      right->left = left;
  }

  bool ContainsHeaderPairForTag(const TagNode &tag);

  bool ContainsBodyPairForTag(const TagNode &tag);


public:

  static void DestroyTag(TagNode *tag);


private:

  static void destroyBodyPairTag(TagNode *tag);

  static void destroyHeaderPairTag(TagNode *tag);

};

#undef TagSection


#endif // TagNode_h__
