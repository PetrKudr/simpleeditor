#ifndef MessageCollector_h__
#define MessageCollector_h__

#include <list>
#include <string>
#include <cstdarg>


enum MessageType
{
  information,
  error
};

struct Message
{
  MessageType type;

  std::wstring message;

  Message(const MessageType &_messageType, const std::wstring &_message) : type(_messageType), message(_message) {}
};


class MessageCollector
{
private:

  static MessageCollector ourInstance;


public:

  // Profiling
  /*int searchAhoTime;

  int transformAhoTime;

  int prepareAhoSearch;*/

  /*int extendingParsingPositionEndTime;*/


private:

  std::list<Message> myMessages;


public:

  static MessageCollector& GetInstance() 
  {
    return ourInstance;
  }


public:

  void AddMessage(const MessageType &type, const wchar_t *message, ...)
  {
    wchar_t buffer[512];
    va_list args;
    va_start(args, message);
#ifdef UNDER_CE
    vswprintf(buffer, message, args);    
#else
    vswprintf(buffer, 512, message, args);    
#endif
    va_end (args);
    myMessages.push_back(Message(type, buffer));
  }

  bool GetMessage(__out Message &message)
  {
    if (!myMessages.empty())
    {
      std::list<Message>::const_iterator iter = myMessages.begin();
      message.type = iter->type;
      message.message = iter->message;
      myMessages.pop_back();
      return true;
    }
    return false;
  }

  bool GetMessage(__in const MessageType type, __out std::wstring &message)
  {
    std::list<Message>::const_iterator iter = myMessages.begin();
    while (iter != myMessages.end())
    {
      if (iter->type == type)
      {
        message = iter->message;
        myMessages.erase(iter);
        return true;
      }

      ++iter;
    }
    return false;
  }

  void ClearMessages()
  {
    myMessages.clear();
  }


private:

  MessageCollector() {}

};

#endif // MessageCollector_h__