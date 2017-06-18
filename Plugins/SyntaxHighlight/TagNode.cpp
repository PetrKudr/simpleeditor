#include "TagNode.hpp"
#include "TagSection.hpp"


bool TagNode::ContainsHeaderPairForTag(const TagNode &tag)
{
  if (tag.description->IsHeaderPair(*description))
    return true;

  if (headerPair != 0 && tag.description->IsHeaderPair(*headerPair->description))
    return true;

  if (bodyPair != 0 && tag.description->IsHeaderPair(*bodyPair->description))
    return true;

  if (bodyPair != 0 && bodyPair->headerPair != 0 && tag.description->IsHeaderPair(*bodyPair->headerPair->description))
    return true;

  return false;
}

bool TagNode::ContainsBodyPairForTag(const TagNode &tag)
{
  if (tag.description->IsBodyPair(*description))
    return true;

  if (headerPair != 0 && tag.description->IsBodyPair(*headerPair->description))
    return true;

  if (bodyPair != 0 && tag.description->IsBodyPair(*bodyPair->description))
    return true;

  if (bodyPair != 0 && bodyPair->headerPair != 0 && tag.description->IsBodyPair(*bodyPair->headerPair->description))
    return true;

  return false;
}


void TagNode::DestroyTag(TagNode *tag)
{
  if (tag->isOpen)
  {
    if (tag->bodyPair != 0)
      DestroyTag(tag->bodyPair);
    if (tag->body != 0)
      delete tag->body;
  }
  if (tag->description->IsComplex())
  {
    if (tag->headerPair != 0)
      DestroyTag(tag->headerPair);
    if (tag->header != 0)
      delete tag->header;
  }
  delete tag;
}
