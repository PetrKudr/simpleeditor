#ifndef TextBlock_h__
#define TextBlock_h__


// representation of text block
class TextBlock
{
public:

  virtual const wchar_t* GetText() const = 0;

  virtual size_t GetTextLength() const = 0;

  virtual const range_t& GetRange() const = 0;

  virtual ~TextBlock() {};
};

// text block which just stores text
class SimpleTextBlock : public TextBlock
{
protected:

  const wchar_t *text;

  size_t length;

  range_t range;


public:

  SimpleTextBlock(const wchar_t *_text, size_t _length, const range_t &_range) : text(_text), length(_length), range(_range) {}

  SimpleTextBlock(const TextBlock &textBlock)
  {
    text = textBlock.GetText();
    length = textBlock.GetTextLength();
    range = textBlock.GetRange();
  }

  const wchar_t* GetText() const
  {
    return text;
  }

  size_t GetTextLength() const
  {
    return length;
  }

  const range_t& GetRange() const
  {
    return range;
  }
};


// text block which deletes text in destructor
class AutoClearingTextBlock : public SimpleTextBlock
{
public:

  AutoClearingTextBlock(const wchar_t *_text, size_t _length, const range_t &_range) : SimpleTextBlock(_text, _length, _range) {}

  AutoClearingTextBlock(const TextBlock &_textBlock) : SimpleTextBlock(_textBlock) {}

  ~AutoClearingTextBlock() 
  {
    if (text != 0)
      delete[] text;
  }
};


#endif // TextBlock_h__