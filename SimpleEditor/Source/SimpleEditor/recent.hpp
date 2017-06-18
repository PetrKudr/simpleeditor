#ifndef __RECENT_
#define __RECENT_

#include "Encoder.hpp"

#include <string>
#include <vector>

#define MAX_RECENT 50
#define DEFAULT_RECENT 10
#define MIN_RECENT 1

#define RECENT_VERSION 106

struct SSession
{
  wchar_t *path;
  unsigned int windowX, windowY;
  unsigned int caretRealX, caretRealY;
  wchar_t codePage[ENCODER_MAX_CODING_ID_LENGTH];

  bool isWordWrap;
  bool isSmartInput;
  bool isDragMode;
  bool isScrollByLine;
};

class CRecent
{
  wchar_t* recent[MAX_RECENT];
  size_t lengths[MAX_RECENT];
  int numOfRecentDocs;
  int current;
  int maxRecent;
  bool isRecentChanged;

  int findPath(const wchar_t *path);
  void floatRecent(int index);

public:

  void SetMaxRecent(int value)   { maxRecent = value ? value : 10; }
  bool ReadRecent();
  bool ReadRecent(std::vector<SSession> &session, int &active);
  bool SaveRecent();
  bool SaveRecent(std::vector<SSession> &session, int active);
  bool AddRecent(const std::wstring &path);
  const wchar_t* GetRecent(int index);
  void RemoveRecent(const std::wstring &path);
  void ClearRecent();

  bool GetSession(std::vector<SSession> &session, int &active);
  bool LoadSession(const std::vector<SSession> &session, int active);
  void ClearSession(std::vector<SSession> &session);

  bool IsRecentExists(const std::wstring &path)  { return findPath(path.c_str()) >= 0 ? true : false; }
  int NumberOfRecent()                           { return numOfRecentDocs;                            }
  
  CRecent()
  {
    current = numOfRecentDocs = 0;
    isRecentChanged = false;
  }
};


#endif /* __RECENT_ */