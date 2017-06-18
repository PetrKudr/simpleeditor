#include "notepad.hpp"
#include <cstdio>

#ifndef _WIN32_WCE
#include "base\codecvt.hpp"
#endif


// function tries to find path in the array of recent
int CRecent::findPath(const wchar_t *path)
{
  int cur = current;

  do
  {
    cur = (cur + maxRecent - 1) % maxRecent;
    if (recent[cur] && wcscmp(recent[cur], path) == 0) 
      return cur;
  } while (cur != current && 0 != recent[cur]);

  return -1;
}

// function makes recent with the specified index last
void CRecent::floatRecent(int index)
{
  int first = (current + maxRecent - 1) % maxRecent;
  wchar_t *path;
  size_t len;

  if (index != first)
    isRecentChanged = true;

  while (index != first)
  {
    path = recent[index];
    len = lengths[index];

    recent[index] = recent[(index + 1) % maxRecent];
    lengths[index] = lengths[(index + 1) % maxRecent];

    index = (index + 1) % maxRecent;
    recent[index] = path;
    lengths[index] = len;
  }   
}


// Function reads recent list, but ignores session
bool CRecent::ReadRecent()
{
  std::wstring path = g_execPath;
  path += L"\\recent.rct";
  std::wifstream fIn(path.c_str(), std::ios_base::binary);
  if (!fIn.is_open())
    return false;

#ifndef _WIN32_WCE
  fIn.imbue(std::locale(fIn.getloc(), new ucs2_conversion));
#endif

  std::wstring str;
  wchar_t *end;

  // get recent version, bumber of recent docs, docs in a session and index of the last active
  std::getline(fIn, str);
  if (wcstol(str.c_str(), &end, 10) != RECENT_VERSION)
  {
    fIn.close();
    isRecentChanged = true;
    return false;
  }
  if (!(*end++))
  {
    // file is corrupted
    fIn.close();
    isRecentChanged = 1;
    CError::SetError(NOTEPAD_CORRUPTED_RECENT_FILE);
    return false;
  }

  int numOfRecent = wcstol(end, &end, 10);
  if (!(*end++))
  {
    // file is corrupted
    fIn.close();
    isRecentChanged = 1;
    CError::SetError(NOTEPAD_CORRUPTED_RECENT_FILE);
    return false;
  }

  if (numOfRecent > MAX_RECENT)  
  {
    // file is corrupted
    fIn.close();
    isRecentChanged = 1;
    CError::SetError(NOTEPAD_CORRUPTED_RECENT_FILE);
    return false;
  }
  if (numOfRecent > maxRecent)
    numOfRecent = maxRecent;
  this->numOfRecentDocs = numOfRecent;

  int current = 0;
  size_t i;

  while (numOfRecent-- && !fIn.eof())
  {
    std::getline(fIn, str);

    // remove eol
    i = str.find_first_of(L"\r\n", 0, 2);
    if (str.npos != i)
      str[i] = L'\0';

    lengths[current] = wcslen(str.c_str()) + 1;
    if ((recent[current] = new wchar_t[lengths[current]]) == 0)
    {
      fIn.close();
      CError::SetError(NOTEPAD_MEMORY_ERROR);
      return false;
    }
    wcscpy(recent[current], str.c_str());
    ++current;
  }
  this->current = current % maxRecent;

  fIn.close();
  return true;
}

// Function reads recent list and session
bool CRecent::ReadRecent(std::vector<SSession> &session, int &active)
{
  std::wstring path = g_execPath;
  path += L"\\recent.rct";
  std::wifstream fIn(path.c_str(), std::ios_base::binary);
  if (!fIn.is_open())
    return false;

#ifndef _WIN32_WCE
  fIn.imbue(std::locale(fIn.getloc(), new ucs2_conversion));
#endif

  std::wstring str;
  wchar_t *end;

  // get recent version, bumber of recent docs, docs in a session and index of the last active
  std::getline(fIn, str);
  if (wcstol(str.c_str(), &end, 10) != RECENT_VERSION)
  {
    fIn.close();
    isRecentChanged = true;
    return false;
  }

  int numOfRecent, numOfActive;

  if (!(*end++))
  {
    // file is corrupted
    fIn.close();
    isRecentChanged = 1;
    CError::SetError(NOTEPAD_CORRUPTED_RECENT_FILE);
    return false;
  }

  numOfRecent = wcstol(end, &end, 10);
  if (!(*end++))
  {
    // file is corrupted
    fIn.close();
    isRecentChanged = 1;
    CError::SetError(NOTEPAD_CORRUPTED_RECENT_FILE);
    return false;
  }

  numOfActive = wcstol(end, &end, 10);
  if (!(*end++))
  {
    // file is corrupted
    fIn.close();
    isRecentChanged = 1;
    CError::SetError(NOTEPAD_CORRUPTED_RECENT_FILE);
    return false;
  }

  active = wcstol(end, &end, 10);

  if (numOfRecent > MAX_RECENT)  
  {
    // file is corrupted
    fIn.close();
    isRecentChanged = 1;
    CError::SetError(NOTEPAD_CORRUPTED_RECENT_FILE);
    return false;
  }
  if (numOfRecent > maxRecent)
    numOfRecent = maxRecent;
  this->numOfRecentDocs = numOfRecent;

  int current = 0;
  size_t i;

  while (numOfRecent-- && !fIn.eof())
  {
    std::getline(fIn, str);

    // remove eol
    i = str.find_first_of(L"\r\n", 0, 2);
    if (str.npos != i)
      str[i] = L'\0';

    lengths[current] = wcslen(str.c_str()) + 1;
    if ((recent[current] = new wchar_t[lengths[current]]) == 0)
    {
      fIn.close();
      CError::SetError(NOTEPAD_MEMORY_ERROR);
      return false;
    }
    wcscpy(recent[current], str.c_str());
    ++current;
  }
  this->current = current % maxRecent;

  if (numOfActive)
  {
    wchar_t *start, *end;

    session.resize(numOfActive);
    current = 0;
    while (current < numOfActive && !fIn.eof())
    {
      std::getline(fIn, str);

      i = str.find_first_of(';', 0);
      if (i == str.npos)
        break;

      if ((session[current].path = new wchar_t[i + 1]) == 0)
      {
        fIn.close();
        session.resize(current);
        CError::SetError(NOTEPAD_MEMORY_ERROR);
        return false;
      }
      wcscpy(session[current].path, str.substr(0, i).c_str());
      end = const_cast<wchar_t*>(&str.c_str()[i + 1]);

      session[current].windowX = wcstoul(end, &end, 10);
      if (!(*end++))
        break;

      session[current].windowY = wcstoul(end, &end, 10);
      if (!(*end++))
        break;

      session[current].caretRealX = wcstoul(end, &end, 10);
      if (!(*end++))
        break;

      session[current].caretRealY = wcstoul(end, &end, 10);
      if (!(*end++))
        break;
      
      start = end;
      end = wcschr(start, L';');
      wcsncpy(session[current].codePage, start, end - start);
      session[current].codePage[end - start] = L'\0';
      if (!(*end++))
        break;

      session[current].isWordWrap = *end == L'0' ? false : true;
      if (!(*++end))
        break;

      session[current].isSmartInput = *end == L'0' ? false : true;
      if (!(*++end))
        break;

      session[current].isDragMode = *end == L'0' ? false : true;
      if (!(*++end))
        break;

      session[current].isScrollByLine = *end == L'0' ? false : true;

      ++current;
    }
    if (current != numOfActive)
    {
      if (session[current].path)
        delete[] session[current].path;
      session.resize(current);
    }
  }

  fIn.close();
  return true;
}

bool CRecent::SaveRecent()
{
  if (!isRecentChanged)
    return true; // all right, we do not need to save recent or session

  std::wstring path = g_execPath;
  path += L"\\recent.rct";
  FILE *fOut = _wfopen(path.c_str(), L"wb");
  if (!fOut)
  {
    CError::SetError(NOTEPAD_CANNOT_SAVE_RECENT);
    return false;
  }

  fwprintf(fOut, L"%i;%i;%i;%i\n", RECENT_VERSION, numOfRecentDocs, 0, 0);

  for (int i = numOfRecentDocs - 1; i >= 0; i--)
    fwprintf(fOut, L"%s\n", GetRecent(i));

  fclose(fOut);

  return true;
}

bool CRecent::SaveRecent(std::vector<SSession> &session, int active)
{
  std::wstring path = g_execPath;
  path += L"\\recent.rct";
  FILE *fOut = _wfopen(path.c_str(), L"wb");
  if (!fOut)
  {
    CError::SetError(NOTEPAD_CANNOT_SAVE_RECENT);
    return false;
  }

  fwprintf(fOut, L"%i;%i;%i;%i\n", RECENT_VERSION, numOfRecentDocs, session.size(), active);

  for (int i = numOfRecentDocs - 1; i >= 0; i--)
    fwprintf(fOut, L"%s\n", GetRecent(i));

  for (size_t i = 0; i < session.size(); i++)
  {
    fwprintf(fOut, L"%s;%i;%i;%i;%i;%s;%c%c%c%c\n", session[i].path, 
      session[i].windowX, 
      session[i].windowY, 
      session[i].caretRealX, 
      session[i].caretRealY,
      session[i].codePage,
      session[i].isWordWrap ? L'1' : L'0',
      session[i].isSmartInput ? L'1' : L'0',
      session[i].isDragMode ? L'1' : L'0',
      session[i].isScrollByLine ? L'1' : L'0');
  }

  fclose(fOut);

  return true;
}

// Function adds new recent into recent list
bool CRecent::AddRecent(const std::wstring &path)
{
  _ASSERTE(!path.empty());

  int index = findPath(path.c_str());
  if (index > -1)  // such path already exists
  { 
    floatRecent(index);
    return true; 
  }


  size_t len = wcslen(path.c_str());
  if (!recent[current] || lengths[current] < len + 1)
  {
    bool newRecent = recent[current] ? false : true;
    if (recent[current])
      free(recent[current]);
    if ((recent[current] = new wchar_t[len + 1]) == 0)
    {
      if (!newRecent)
        --numOfRecentDocs;
      CError::SetError(NOTEPAD_MEMORY_ERROR);
      return false;
    }
    lengths[current] = len + 1;
    if (newRecent)
      ++numOfRecentDocs;
  }
  wcscpy(recent[current], path.c_str());
  current = (current + 1) % maxRecent;
  isRecentChanged = true;
  return true;
}

const wchar_t* CRecent::GetRecent(int index)
{
  if (index < 0 || index >= maxRecent)
    return 0;

  return recent[(current + maxRecent - 1 - index) % maxRecent];
}

void CRecent::RemoveRecent(const std::wstring &path)
{
  int index = findPath(path.c_str());

  if (index >= 0)
  {
    floatRecent(index);

    int first = (current + maxRecent - 1) % maxRecent;
    if (recent[first])
    {
      delete recent[first];
      recent[first] = 0;
      lengths[first] = 0;
    }
    current = first;

    numOfRecentDocs--;
    isRecentChanged = true;
  }
}

void CRecent::ClearRecent()
{
  for (int i = 0; i < MAX_RECENT; i++)
  {
    if (recent[i])
    {
      delete recent[i];
      recent[i] = 0;
      lengths[i] = 0;
    }
  }
  current = 0;
  numOfRecentDocs = 0;
  isRecentChanged = true;
}

void CRecent::ClearSession(std::vector<SSession> &session)
{
  for (size_t i = 0; i < session.size(); i++)
    delete[] session[i].path;
}

// Function fills array of documents (session)
bool CRecent::GetSession(std::vector<SSession> &session, int &active)
{
  CNotepad &Notepad = CNotepad::Instance();
  size_t len, i = 0;

  session.resize(Notepad.Documents.GetDocsNumber());
  for (size_t count = 0; count < session.size(); count++)
  {
    if ((len = wcslen(Notepad.Documents.GetPath(i).c_str())) == 0)
      continue;

    if ((session[i].path = new wchar_t[len + 1]) == 0)
    {
      CError::SetError(NOTEPAD_MEMORY_ERROR);
      session.resize(i);
      return false;
    }
    wcscpy(session[i].path, Notepad.Documents.GetPath(i).c_str());

    wcscpy(session[i].codePage, Notepad.Documents.GetCoding(i));

    session[i].isWordWrap = Notepad.Documents.IsWordWrap(i);
    session[i].isSmartInput = Notepad.Documents.IsSmartInput(i);
    session[i].isDragMode = Notepad.Documents.IsDragMode(i);
    session[i].isScrollByLine = Notepad.Documents.IsScrollByLine(i);

    Notepad.Documents.GetCaretPos(i, session[i].caretRealX, session[i].caretRealY, true);
    Notepad.Documents.GetWindowPos(i, session[i].windowX, session[i].windowY);
    ++i;
  }
  session.resize(i);

  active = 0;
  if (session.size())
    active = Notepad.Documents.Current();

  return true;
}

bool CRecent::LoadSession(const std::vector<SSession> &session, int active)
{
  CNotepad &Notepad = CNotepad::Instance();
  std::wstring path;

  for (size_t i = 0; i < session.size(); i++)
  {
    path = session[i].path;

    Encoder *encoder = Notepad.Documents.CodingManager.GetEncoderById(session[i].codePage);
    if (!Notepad.Documents.Open(path, encoder != 0 ? encoder->GetCodingId() : CCodingManager::CODING_UNKNOWN_ID, OPEN, false))
    {
      // standart error (maybe such file doesn't exist)
      Notepad.Interface.Dialogs.ShowError(Notepad.Interface.GetHWND(), CError::GetError().message);
      CError::SetError(NOTEPAD_NO_ERROR);

      if (Notepad.Documents.GetDocsNumber() > 0)
        Notepad.Documents.Close(Notepad.Documents.GetDocsNumber() - 1);
    }
    else // document is opening
    {
      Notepad.Documents.SetWordWrap(i, session[i].isWordWrap);
      Notepad.Documents.SetSmartInput(i, session[i].isSmartInput);
      Notepad.Documents.SetDragMode(i, session[i].isDragMode);
      Notepad.Documents.SetScrollByLine(i, session[i].isScrollByLine);
      Notepad.Documents.SetCaretPos(i, session[i].caretRealX, session[i].caretRealY, true, false);
      Notepad.Documents.SetScreenPos(i, session[i].windowX, session[i].windowY);
    }
  }

  if (active >= 0 && active < Notepad.Documents.GetDocsNumber())
    Notepad.Documents.SetActive(active);
  else if (Notepad.Documents.GetDocsNumber())
    Notepad.Documents.SetActive(0);

  return true;
}