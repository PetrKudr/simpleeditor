#include "base\files.h"
#include "resources\res.hpp"
#include "PluginTypes.hpp"
#include "notepad.hpp"


#ifndef MAPVK_VK_TO_CHAR
  #define MAPVK_VK_TO_CHAR (2)
#endif



// structures for threads
struct SOpenThreadData
{
  HWND hwndOwner;
  document_t *pdoc;
  OpenType openType;
  bool useFastReadMode;
  int id;
};

struct SSaveThreadData
{
  HWND hwndOwner;
  document_t *pdoc;
  int id;
  std::wstring path;
  LPCWSTR cp;
};

struct SWaitThreadData
{
  HWND hwndOwner;
  document_t *pdocs;
  unsigned backMsg;
};



static const std::wstring s_emptyString = L"";


CDocuments::CDocuments()
{
  for (int i = 0; i < MAX_DOCUMENTS; i++)
  {
    clearDoc(docs[i]);
    idArray[i] = -1;
    indexArray[i] = -1;
  }
  number = 0;
  current = 0;
}

CDocuments::~CDocuments()
{
}


// Function returns first empty cell in docs massive (id)
int CDocuments::findPlace()
{
  _ASSERTE(number < MAX_DOCUMENTS);
  for (int i = 0; i < MAX_DOCUMENTS; i++)
    if (0 == docs[i].hwnd)
      return i;
  return 0;
}

// Function performs the specified actions (in flag field), except DOC_CLOSE
void CDocuments::processFlag(int id)
{
  CNotepad &Notepad = CNotepad::Instance();

  if (docs[id].flag & DOC_RESIZE)
  {
    SWindowPos coords;

    Notepad.Interface.CalcDocCoords(coords);
    Move(indexArray[id], coords.x, coords.y, coords.width, coords.height, false, false);
    docs[id].flag ^= DOC_RESIZE;
  }

  if (docs[id].flag & DOC_GOTOCARET)
  {
    SendMessage(docs[id].hwnd, SE_GOTOCARET, 0, 0);
    docs[id].flag ^= DOC_GOTOCARET;
  }

  if (docs[id].flag & DOC_SETREGEXP)
    SetRegexp(indexArray[id]);
  if (docs[id].flag & DOC_SETHIDESPACES)
    SetHideSpaces(indexArray[id]);
  if (docs[id].flag & DOC_SETSCROLLBARS)
    SetPermanentScrollBars(indexArray[id]);
  if (docs[id].flag & DOC_SETWRAPTYPE)
    SetWrapType(indexArray[id]);
  if (docs[id].flag & DOC_SETWORDWRAP)
    SetWordWrap(indexArray[id], docs[id].actions.isWordWrap);
  if (docs[id].flag & DOC_SETSMARTINPUT)
    SetSmartInput(indexArray[id], docs[id].actions.isSmartInput);
  if (docs[id].flag & DOC_SETDRAGMODE)
    SetDragMode(indexArray[id], docs[id].actions.isDragMode);
  if (docs[id].flag & DOC_SETSCROLLBYLINE)
    SetScrollByLine(indexArray[id], docs[id].actions.isScrollByLine);

  if (docs[id].flag & DOC_SETFONT)
    SetFont(indexArray[id], Notepad.Options.GetProfile().heditorFont);
  if (docs[id].flag & DOC_SETTABSIZE)
    SetTabSize(indexArray[id], Notepad.Options.GetProfile().tabSize);
  if (docs[id].flag & DOC_SETLEFTINDENT)
    SetLeftIndent(indexArray[id], Notepad.Options.LeftIndent());

  if (docs[id].flag & DOC_SETCARETPOS)
    SetCaretPos(indexArray[id], docs[id].actions.caretPos.x, docs[id].actions.caretPos.y,
      docs[id].actions.realCaretPos, false);
  if (docs[id].flag & DOC_SETSCREENPOS)
    SetScreenPos(indexArray[id], docs[id].actions.screenPos.x, docs[id].actions.screenPos.y);  
}


// Function sets all values in doc structure by default
void CDocuments::clearDoc(document_t &doc)
{
  CNotepad &Notepad = CNotepad::Instance();
  Encoder *defaultEncoder = CodingManager.GetEncoderById(Notepad.Options.DefaultCP().c_str());

  doc.cp = (defaultEncoder != 0) ? defaultEncoder->GetCodingId() : CCodingManager::CODING_DEFAULT_ID;
  doc.flag = 0;
  doc.hthread = 0;
  doc.hwnd = 0;
  doc.prev = 0;
  doc.modified = false;
  doc.path.clear();
  doc.name.clear();
  doc.BOM = false;
}


// Function fills the name string
void CDocuments::getFileName(const std::wstring &path, std::wstring &name)
{
  //std::wstring::const_iterator i = path.end();
  int i = path.rfind(L'\\', path.length());

  _ASSERTE(path.length() > 0);

  name = L"Impossible name";
  if (i >= 0)
    name.assign(path, i + 1, path.length() - i);
}

// Function sets all options
void CDocuments::setDocOptions(const document_t &doc)
{
  _ASSERTE(doc.hwnd);

  CNotepad &Notepad = CNotepad::Instance();
  options_t options;

  options.caretWidth = Notepad.Options.CaretWidth();
  options.wrapType = Notepad.Options.WrapType();
  options.useRegexp = Notepad.Options.UseRegexp();
  options.hideSpaceSyms = Notepad.Options.HideSpaceSyms();
  options.hfont = Notepad.Options.GetProfile().heditorFont;
  options.tabSize = Notepad.Options.GetProfile().tabSize;
  options.bgColor = Notepad.Options.GetProfile().EditorColor;
  options.textColor = Notepad.Options.GetProfile().EditorTextColor;
  options.caretTextColor = Notepad.Options.GetProfile().EditorCaretTextColor;
  options.caretBgColor = Notepad.Options.GetProfile().EditorCaretBgColor;
  options.selectionColor = Notepad.Options.GetProfile().EditorSelColor;
  options.selectedTextColor = Notepad.Options.GetProfile().EditorSelTextColor;
  options.selectionTransparency = (Notepad.Options.GetProfile().selectionTextTransparent ? SEDIT_SELTEXTTRANSPARENT : 0) | 
                                  (Notepad.Options.GetProfile().selectionBackgroundTransparent ? SEDIT_SELBGTRANSPARENT : 0);
  options.caretWidth = Notepad.Options.CaretWidth();
  options.leftIndent = Notepad.Options.LeftIndent();
  options.permanentScrollBars = Notepad.Options.PermanentScrollBars();
  options.kineticScroll = Notepad.Options.KineticScroll();

  options.type = SEDIT_OPTIONSALL;

  SendMessage(doc.hwnd, SE_SETOPTIONS, (WPARAM)&options, 0);
  SendMessage(doc.hwnd, SE_SETWORDWRAP, Notepad.Options.DefaultWordWrap() ? 1 : 0, 0);
  SendMessage(doc.hwnd, SE_SETSMARTINPUT, Notepad.Options.DefaultSmartInput() ? 1 : 0, 0);
  SendMessage(doc.hwnd, SE_SETDRAGMODE, Notepad.Options.DefaultDragMode() ? 1 : 0, 0);
  SendMessage(doc.hwnd, SE_SETSCROLLBYLINE, Notepad.Options.DefaultScrollByLine() ? 1 : 0, 0);
  SendMessage(doc.hwnd, SE_SETUNDOSIZE, 0, Notepad.Options.UndoHistorySize());
}

int CDocuments::findDocumentByPath(const std::wstring &path)
{
  for (int i = 0; i < number; i++)
  {
    int id = idArray[i];
    if (_wcsnicmp(docs[id].path.c_str(), path.c_str(), MAX_PATH) == 0) // case insensitive comparison
      return id;
  }
  return -1;
}


// Function returns index of a document
int CDocuments::GetIndexFromID(int id)
{
  if (id < 0 || id >= MAX_DOCUMENTS)
    return -1;
  return indexArray[id];
}

// Function returns id of a document
int CDocuments::GetIdFromIndex(int index)
{
  if (index < 0 || index >= MAX_DOCUMENTS)
    return -1;
  return idArray[index];
}

// Function returns current document
int CDocuments::Current()
{
  return current;
}


// Function returns number of documents
int CDocuments::GetDocsNumber()
{
  return number;
}

// Function fills ids with ids of the documents
void CDocuments::GetDocIds(__out std::vector<int> &ids)
{
  for (int i = 0; i < MAX_DOCUMENTS; i++)
  {
    if (0 != docs[i].hwnd)
      ids.push_back(idArray[i]);
  }
}

// Function tests if document with given id exist
bool CDocuments::DocumentWithIdExist(int id)
{
  return id >= 0 && id < MAX_DOCUMENTS && docs[id].hwnd != 0;
}

// Function tests if document with given index
bool CDocuments::DocumentWithIndexExist(int index)
{
  return index >= 0 && index < number && DocumentWithIdExist(idArray[index]);
}

// Function returns editor's window
HWND CDocuments::GetHWND(int index)
{
  if (DocumentWithIndexExist(index))
    return docs[idArray[index]].hwnd;
  CError::SetError(NOTEPAD_DOCUMENT_NOT_EXIST);
  return 0;
}

// Function returns name of a document
const std::wstring& CDocuments::GetName(int index)
{
  if (DocumentWithIndexExist(index))
    return docs[idArray[index]].name;
  CError::SetError(NOTEPAD_DOCUMENT_NOT_EXIST);
  return s_emptyString;
}

// Function returns path
const std::wstring& CDocuments::GetPath(int index)
{
  if (DocumentWithIndexExist(index))
    return docs[idArray[index]].path;
  CError::SetError(NOTEPAD_DOCUMENT_NOT_EXIST);
  return s_emptyString;
}

// Function return code page of the document
LPCWSTR CDocuments::GetCoding(int index)
{
  if (DocumentWithIndexExist(index))
    return docs[idArray[index]].cp;
  CError::SetError(NOTEPAD_DOCUMENT_NOT_EXIST);
  return s_emptyString.c_str();
}

// Function returns coordinates of the caret
void CDocuments::GetCaretPos(int index, unsigned int &x, unsigned int &y, bool real)
{
  if (DocumentWithIndexExist(index))
  {
    doc_info_t di;
    di.flags = real ? SEDIT_GETCARETREALPOS : SEDIT_GETCARETPOS;
    SendMessage(docs[idArray[index]].hwnd, SE_GETDOCINFO, (WPARAM)&di, 0);

    x = di.caretPos.x;
    y = di.caretPos.y;
  }
  else
    CError::SetError(NOTEPAD_DOCUMENT_NOT_EXIST);
}

// Function return position of the window
void CDocuments::GetWindowPos(int index, unsigned int &x, unsigned int &y)
{
  if (DocumentWithIndexExist(index))
  {
    doc_info_t di;
    di.flags = SEDIT_GETWINDOWPOS;
    SendMessage(docs[idArray[index]].hwnd, SE_GETDOCINFO, (WPARAM)&di, 0);

    x = di.windowPos.x;
    y = di.windowPos.y;
  }
  else
    CError::SetError(NOTEPAD_DOCUMENT_NOT_EXIST);
}

// Function returns true if the document is modified
bool CDocuments::IsModified(int index)
{
  if (DocumentWithIndexExist(index))
  {

  }
  return docs[idArray[index]].modified;
}

// Function returns true if the document is busy(hthread != 0)
bool CDocuments::IsBusy(int index)
{
  if (DocumentWithIndexExist(index))
    return docs[idArray[index]].hthread != 0;
  CError::SetError(NOTEPAD_DOCUMENT_NOT_EXIST);
  return true;
}

// Function returns true if needed to write BOM
bool CDocuments::NeedBOM(int index)
{
  if (DocumentWithIndexExist(index))
    return docs[idArray[index]].BOM;
  CError::SetError(NOTEPAD_DOCUMENT_NOT_EXIST);
  return true;
}

bool CDocuments::IsWordWrap(int index)
{
  if (DocumentWithIndexExist(index))
    return SendMessage(docs[idArray[index]].hwnd, SE_ISWORDWRAP, 0, 0) ? true : false;
  CError::SetError(NOTEPAD_DOCUMENT_NOT_EXIST);
  return true;
}

bool CDocuments::IsSmartInput(int index)
{
  if (DocumentWithIndexExist(index))
    return SendMessage(docs[idArray[index]].hwnd, SE_ISSMARTINPUT, 0, 0) ? true : false;
  CError::SetError(NOTEPAD_DOCUMENT_NOT_EXIST);
  return true;
}

bool CDocuments::IsDragMode(int index)
{
  if (DocumentWithIndexExist(index))
    return SendMessage(docs[idArray[index]].hwnd, SE_ISDRAGMODE, 0, 0) ? true : false;
  CError::SetError(NOTEPAD_DOCUMENT_NOT_EXIST);
  return true;
}

bool CDocuments::IsScrollByLine(int index)
{
  if (DocumentWithIndexExist(index))
    return SendMessage(docs[idArray[index]].hwnd, SE_ISSCROLLBYLINE, 0, 0) ? true : false;
  CError::SetError(NOTEPAD_DOCUMENT_NOT_EXIST);
  return true;
}

unsigned int CDocuments::GetDocLinesNumber(int index, bool real)
{
  _ASSERTE(index >= 0 && index < number);

  if (IsBusy(index))
    return 0;

  doc_info_t di = {0};
  CNotepad &Notepad = CNotepad::Instance();

  SendMessage(docs[idArray[index]].hwnd, SE_GETDOCINFO, (WPARAM)&di, 0);
  return real ? di.nRealLines : di.nLines;
}

unsigned int CDocuments::GetDocLineLength(int index, unsigned int number, bool real)
{
  _ASSERTE(index >= 0 && index < this->number);
  if (IsBusy(index))
    return 0;
  return SendMessage(docs[idArray[index]].hwnd, SE_GETLINELENGTH, number, real ? SEDIT_REALLINE : SEDIT_VISUAL_LINE);
}

bool CDocuments::IsSelection()
{
  if (number == 0 || IsBusy(current))
    return false;
  return SendMessage(docs[idArray[current]].hwnd, SE_ISSELECTION, 0, 0) > 0 ? true : false;
}

bool CDocuments::CanUndo()
{
  if (number == 0 || IsBusy(current))
    return false;
  return SendMessage(docs[idArray[current]].hwnd, SE_CANUNDO, 0, 0) > 0 ? true : false;
}

bool CDocuments::CanRedo()
{
  if (number == 0 || IsBusy(current))
    return false;
  return SendMessage(docs[idArray[current]].hwnd, SE_CANREDO, 0, 0) > 0 ? true : false;
}

bool CDocuments::CanPaste()
{
  if (number == 0 || IsBusy(current))
    return false;
  return IsClipboardFormatAvailable(CF_UNICODETEXT) != 0 ? true : false;
}


const wchar_t* CDocuments::AutoName()
{
  static wchar_t newName[32] = L"Untitled 0";
  static int num = 0;

  int k = num, i = 1;

  while (k /= 10)
    ++i;

#ifndef _WIN32_WCE
  swprintf(&newName[wcslen(newName) - i], 16, L"%i", num);
#else
  swprintf(&newName[wcslen(newName) - i], L"%i", num);
#endif

  ++num;

  return newName;
}


// Function creates new document
bool CDocuments::Create(const std::wstring &caption)
{
  CNotepad &Notepad = CNotepad::Instance();
  
  // we already have maximal number of documents
  if (number == MAX_DOCUMENTS)
  {
    CError::SetError(NOTEPAD_DOCUMENTS_LIMIT_ERROR);
    return false;
  }


  // Create new document
  int id = findPlace();
  
  docs[id].hwnd = CreateWindow(SEDIT_CLASS, caption.c_str(), DOCUMENT_WINDOW_STYLE, 0, 0, 0, 0,
                               Notepad.Interface.GetHWND(), 0, g_hAppInstance, 0);
  if (!docs[id].hwnd)
  {
    CError::SetError(NOTEPAD_CREATE_DOCUMENT_ERROR);
    return false;
  }
  docs[id].cp = Notepad.Options.DefaultCP().c_str();
  docs[id].BOM = Notepad.Options.NeedBOM();
  docs[id].flag |= DOC_RESIZE;
  docs[id].name = caption;
  SendMessage(docs[id].hwnd, SE_SETID, (WPARAM)id, 0);
  
  void* funcs[2] = {createPopupMenuFunc, destroyPopupMenuFunc};
  SendMessage(docs[id].hwnd, SE_SETPOPUPMENUFUNCS, 0, (LPARAM)funcs);

  SendMessage(docs[id].hwnd, SE_SETRECEIVEMSGFUNC, 0, (LPARAM)receiveMessageFunc);

  // Set options
  setDocOptions(docs[id]);

  // update arrays
  indexArray[id] = number;
  idArray[number] = id;
  ++number;

  // add a new tab if they exist
  if (Notepad.Interface.Tabs.Available())
    Notepad.Interface.Tabs.Add(caption);

  // notify plugins
  PluginNotification pn;
  pn.notification = PN_EDITOR_CREATED;
  pn.data = (void*)&id;
  Notepad.PluginManager.NotifyPlugins(pn);
  
  _ASSERTE(number > 0);

  return true;
}


// Function closes the document
bool CDocuments::Close(int index)
{
  if (!DocumentWithIndexExist(index))
  {
    CError::SetError(NOTEPAD_DOCUMENT_NOT_EXIST);
    return false;
  }

  CNotepad &Notepad = CNotepad::Instance();
  int id = idArray[index];

  if (docs[id].hthread)
  {
    //CError::SetError(NOTEPAD_DOCUMENT_IS_BUSY);
    docs[id].flag |= DOC_CLOSE;
    return false;
  }

  // warn user if document is modified and save it if necessary
  if (docs[id].modified)
  {
    std::wstring warning = DOC_FILE_NOT_SAVED;
    warning += docs[id].name;
    warning += L"?";
    int answer = Notepad.Interface.Dialogs.MsgBox(Notepad.Interface.GetHWND(), 
                                                  warning.c_str(), 
                                                  WARNING_CAPTION, MB_YESNOCANCEL);
    if (answer == IDYES)
    {
      std::wstring path = docs[id].path;
      LPCWSTR cp = docs[id].cp;

      if (docs[id].path.empty())
        return false;

      if (!Notepad.Interface.Dialogs.SaveFileDialog(Notepad.Interface.GetHWND(), docs[id].name, path, cp, docs[id].BOM, Notepad.Options.OwnDialogs()))
        return false;

      docs[id].flag |= DOC_CLOSE;  // document must be closed after saving
      if (!Save(index, path, cp, docs[id].BOM))
        docs[id].flag ^= DOC_CLOSE; // save failed, we don't need to close document

      return false;
    }
    else if (answer == IDCANCEL)
      return false;
  }

  _ASSERTE(docs[id].prev >= 0 && docs[id].prev < MAX_DOCUMENTS);
  int prevID = docs[id].prev;

  // We certainly can close the document
  // it's better to save handle and destroy window after clearing document structure,
  // because DestroyWindow can set focus to the parent window, but parent window does not know
  // that document was deleted
  HWND hwndDestroy = docs[id].hwnd;  
  clearDoc(docs[id]);
  for (int i = index; i < number - 1; i++)
  {
    idArray[i] = idArray[i + 1];
    indexArray[idArray[i]] = i;
  }
  --number;
  idArray[number] = -1;
  indexArray[id] = -1;
  DestroyWindow(hwndDestroy);

  // delete coordinates in the status bar if our document is current
  if (index == current && Notepad.Interface.StatusBar.IsStatusBar())
    Notepad.Interface.StatusBar.SetPosition(-1, -1);

  // we must set new document as active (if index == current)
  bool needSetNewActive = false;
  int prevIndice = 0;
  if (index == current && number > 0)
  {
    prevIndice = indexArray[prevID] >= 0 ? indexArray[prevID] : 0;

    _ASSERTE(prevIndice >= 0 && prevIndice < number);

    current = prevIndice;    // otherwise SetActive function will work with the closed document
    needSetNewActive = true;
  }
  else if (index < current)
    --current;  // update index of the current document

  // we should delete one tab if they exist
  if (Notepad.Interface.Tabs.Available())
    Notepad.Interface.Tabs.Delete(index);

  // set new active document
  if (needSetNewActive)
    SetActive(prevIndice);
  else if (Notepad.Interface.Controlbar.IsToolbar())
    Notepad.Interface.Controlbar.UpdateDocDependButtons();

  // notify plugins
  PluginNotification pn;
  pn.notification = PN_EDITOR_DESTROYED;
  pn.data = (void*)&id;
  Notepad.PluginManager.NotifyPlugins(pn);

  // if there are no documents and CreateEmptyIfNoDocuments == true, we must create an empty document
  if (Notepad.Options.CreateEmptyIfNoDocuments() && number == 0)
  {
    std::wstring name = AutoName();
    Create(name);
    SetActive(0);
  }

  return true;
}


// Function opens the new document
bool CDocuments::Open(const std::wstring &path, LPCWSTR cp, OpenType openType, bool activate, bool openInCurrent, bool checkExisting)
{
  CNotepad &Notepad = CNotepad::Instance();

  int id = idArray[current];

  int alreadyExistingId = findDocumentByPath(path);

  if (checkExisting && Notepad.Options.SwitchToExistingOnOpen() && alreadyExistingId >= 0)
  {
    if (alreadyExistingId != id)
      SetActive(indexArray[alreadyExistingId]);
  }
  else
  { 
    // if we haven't got documents or current is modified, or path is not empty, we should create new one
    // (if it's not restricted by openInCurrent flag)
    if (number == 0 || docs[id].hthread != 0 || (!openInCurrent && (docs[id].modified || !docs[id].path.empty())))
    {
      std::wstring name = AutoName();
      if (!Create(name))
        return false;
      id = idArray[number - 1];

      if (activate)
        SetActive(number - 1);
    }

    docs[id].cp = cp;
    docs[id].path = path;
    getFileName(path, docs[id].name);

    if (Notepad.Interface.Tabs.Available())
    {
      Notepad.Interface.Tabs.DocNameChanged(indexArray[id]);
      Notepad.Interface.Tabs.SetBusy(indexArray[id], true);
    }

    DWORD threadID;
    SOpenThreadData *pdata = new SOpenThreadData;
    if (!pdata)
    {
      CError::SetError(NOTEPAD_MEMORY_ERROR);
      return false;
    }

    pdata->hwndOwner = Notepad.Interface.GetHWND();
    pdata->pdoc = &docs[id];
    pdata->id = id;
    pdata->openType = openType;
    pdata->useFastReadMode = Notepad.Options.UseFastReadMode();

    EnableWindow(docs[id].hwnd, 0);
    docs[id].hthread = CreateThread(0, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(openThreadProc), 
                                    reinterpret_cast<void*>(pdata), 0, &threadID);
    if (!docs[id].hthread)
    {
      delete pdata;
      CError::SetError(NOTEPAD_CREATE_THREAD_ERROR);
      return false;
    }
  }

  return true;
}

// Function is called when Open thread terminates
void CDocuments::OpenEnd(SOpenEndData *popenEndData)
{
  int id = popenEndData->docId;

  _ASSERTE(docs[id].hthread != 0);

  //WaitForSingleObject(docs[id].hthread, 1000); // wait 1 second
  CloseHandle(docs[id].hthread);
  docs[id].hthread = 0;
  EnableWindow(docs[id].hwnd, 1);

  // close document if flag DOC_CLOSE is set
  if (!(popenEndData->error == NOTEPAD_NO_ERROR) && (docs[id].flag & DOC_CLOSE)) 
  {
    delete popenEndData;
    Close(indexArray[id]);
    return;
  }

  CNotepad &Notepad = CNotepad::Instance();
  if (popenEndData->error != NOTEPAD_NO_ERROR)
  {
    if (popenEndData->openType == RECENT && popenEndData->error == NOTEPAD_OPEN_FILE_ERROR)
    {
      // remove file from recent
      Notepad.Recent.RemoveRecent(docs[id].path);
    }

    docs[id].cp = Notepad.Options.DefaultCP().c_str();
    docs[id].path = L"";
  }
  else
  {
    if (!Notepad.Recent.AddRecent(docs[id].path))
    {
      Notepad.Interface.Dialogs.ShowError(Notepad.Interface.GetHWND(), CError::GetError().message);
      CError::SetError(NOTEPAD_NO_ERROR);
    } 
    else if (Notepad.Options.SaveRecentImmediately())
      Notepad.SaveRecentAndSession();
  }

  SetModified(indexArray[id], false);

  if (Notepad.Interface.Tabs.Available())
    Notepad.Interface.Tabs.SetBusy(indexArray[id], false);

  if (current == indexArray[id])
    ::SetFocus(docs[id].hwnd);  // restore focus

  processFlag(id);

  delete popenEndData;

  PluginNotification pn;
  pn.notification = PN_EDITOR_FILE_OPENED;
  pn.data = (void*)&id;
  Notepad.PluginManager.NotifyPlugins(pn);
}

// Function saves the document
bool CDocuments::Save(int index, const std::wstring &path, LPCWSTR cp, bool BOM)
{
  if (!DocumentWithIndexExist(index))
  {
    CError::SetError(NOTEPAD_DOCUMENT_NOT_EXIST);
    return false;
  }

  CNotepad &Notepad = CNotepad::Instance();
  int id = idArray[index];

  if (docs[id].hthread != 0)
  {
    CError::SetError(NOTEPAD_DOCUMENT_IS_BUSY);
    return false;
  }
  SetBOM(index, BOM);

  DWORD threadID;
  SSaveThreadData *pdata = new SSaveThreadData;
  if (!pdata)
  {
    CError::SetError(NOTEPAD_MEMORY_ERROR);
    return false;
  }

  pdata->id = id;
  pdata->path = path;
  pdata->cp = cp;
  pdata->pdoc = &docs[id];
  pdata->hwndOwner = Notepad.Interface.GetHWND();

  if (Notepad.Interface.Tabs.Available())
    Notepad.Interface.Tabs.SetBusy(index, true);

  EnableWindow(docs[id].hwnd, 0);
  docs[id].hthread = CreateThread(0, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(saveThreadProc), 
                                  reinterpret_cast<void*>(pdata), 0, &threadID);

  if (!docs[id].hthread)
  {
    delete pdata;
    CError::SetError(NOTEPAD_CREATE_THREAD_ERROR);
    return false;
  }

  return true;
}

// Function is called when save thread terminates
void CDocuments::SaveEnd(int id, bool isError)
{
  _ASSERTE(docs[id].hthread != 0);

  CloseHandle(docs[id].hthread);
  docs[id].hthread = 0;
  EnableWindow(docs[id].hwnd, 1);

  if (!isError)
    SendMessage(docs[id].hwnd, SE_SETMODIFY, 0, 0);


  CNotepad &Notepad = CNotepad::Instance();

  if (Notepad.Interface.Tabs.Available())
    Notepad.Interface.Tabs.SetBusy(indexArray[id], false);

  // add document to recent if it is a new one
  if (!isError && !Notepad.Recent.IsRecentExists(docs[id].path))
  {
    if (!Notepad.Recent.AddRecent(docs[id].path))
    {
      Notepad.Interface.Dialogs.ShowError(Notepad.Interface.GetHWND(), CError::GetError().message);
      CError::SetError(NOTEPAD_NO_ERROR);
    }
    else if (Notepad.Options.SaveRecentImmediately())
      Notepad.SaveRecentAndSession();
  }

  // close document if flag DOC_CLOSE is set
  if (!isError && (docs[id].flag & DOC_CLOSE)) 
  {
    Close(indexArray[id]);
    return;
  }

  if (!isError)
  {
    getFileName(docs[id].path, docs[id].name);
    Notepad.Interface.Tabs.DocNameChanged(indexArray[id]);
    SetModified(indexArray[id], false);
  }

  if (current == indexArray[id])
    ::SetFocus(docs[id].hwnd);  // restore focus

  processFlag(id);

  PluginNotification pn;
  pn.notification = PN_EDITOR_FILE_SAVED;
  pn.data = (void*)&id;
  Notepad.PluginManager.NotifyPlugins(pn);
}

void CDocuments::SetFocus()
{
  if (number > 0 && !IsBusy(current))
    ::SetFocus(docs[idArray[current]].hwnd);
}

void CDocuments::Undo()
{
  if (number > 0 && !IsBusy(current))
    SendMessage(docs[idArray[current]].hwnd, SE_UNDO, 0, 0);
}

void CDocuments::Redo()
{
  if (number > 0 && !IsBusy(current))
    SendMessage(docs[idArray[current]].hwnd, SE_REDO, 0, 0);
}

void CDocuments::Copy()
{
  if (number > 0 && !IsBusy(current))
    SendMessage(docs[idArray[current]].hwnd, SE_COPY, 0, 0);
}

void CDocuments::Cut()
{
  if (number > 0 && !IsBusy(current))
    SendMessage(docs[idArray[current]].hwnd, SE_CUT, 0, 0);
}

void CDocuments::Delete()
{
  if (number > 0 && !IsBusy(current))
    SendMessage(docs[idArray[current]].hwnd, SE_DELETE, 0, 0);
}

void CDocuments::Paste()
{
  if (number > 0 && !IsBusy(current))
    SendMessage(docs[idArray[current]].hwnd, SE_PASTE, 0, 0);
}

void CDocuments::Paste(const wchar_t *buff)
{
  if (number > 0 && !IsBusy(current))
    SendMessage(docs[idArray[current]].hwnd, SE_PASTEFROMBUFFER, (WPARAM)buff, 1);
}

void CDocuments::SelectAll()
{
  if (number > 0 && !IsBusy(current))
    SendMessage(docs[idArray[current]].hwnd, SE_SELECTALL, 0, 0);
}

void CDocuments::GotoBegin()
{
  if (number > 0 && !IsBusy(current))
    SetCaretPos(current, 0, 0, true, true);
}

void CDocuments::GotoEnd()
{
  if (number > 0 && !IsBusy(current))
  {
    unsigned int maxLines = GetDocLinesNumber(current, true);
    unsigned int x = GetDocLineLength(current, maxLines - 1, true);
    SetCaretPos(current, x, maxLines - 1, true, true);
  }
}

void CDocuments::StartUndoGroup()
{
  if (number > 0 && !IsBusy(current))
    SendMessage(docs[idArray[current]].hwnd, SE_STARTUNDOGROUP, 0, 0);
}

void CDocuments::EndUndoGroup()
{
  if (number > 0 && !IsBusy(current))
    SendMessage(docs[idArray[current]].hwnd, SE_ENDUNDOGROUP, 0, 0);
}

wchar_t* CDocuments::GetSelection()
{
  wchar_t *buff = 0;
  if (number > 0 && !IsBusy(current))
  {
    if (SendMessage(docs[idArray[current]].hwnd, SE_ISSELECTION, 0, 0))
    {
      int size = SendMessage(docs[idArray[current]].hwnd, SE_GETSELECTEDTEXT, 0, 0);
      if ((buff = new wchar_t [size]) != 0)
        SendMessage(docs[idArray[current]].hwnd, SE_GETSELECTEDTEXT, (WPARAM)buff, size);
    }
  }
  return buff;
}

bool CDocuments::FindReplace(FINDREPLACE *pfr, bool fromEdge)
{
  if (number > 0)
  {
    if (fromEdge)
    { 
      position_t position = {0};

      if (!(pfr->Flags & FR_DOWN))
      {
        doc_info_t di = {0};
        SendMessage(docs[idArray[current]].hwnd, SE_GETDOCINFO, (WPARAM)&di, 0);

        position.y = di.nRealLines - 1;
        position.x = SendMessage(docs[idArray[current]].hwnd, SE_GETLINELENGTH, (WPARAM)di.nRealLines - 1, SEDIT_REALLINE);
      }

      SendMessage(docs[idArray[current]].hwnd, SE_SETSEARCHPOS, SEDIT_REALLINE, (LPARAM)&position);
    }

    int retval = SendMessage(docs[idArray[current]].hwnd, SE_FINDREPLACE, (WPARAM)pfr, 0);

    if (retval == 0) // text not found
    {
      //CNotepad &Notepad = CNotepad::Instance();
      //Notepad.Interface.Dialogs.MsgBox(docs[idArray[current]].hwnd, DOC_TEXT_NOT_FOUND, INFO_CAPTION, MB_OK);
      return false;
    }
    else if (retval < 0) // an error occured
    {
      CError::SetError(CError::TranslateEditor(retval));
      return false;
    }
  }

  return true;
}

bool CDocuments::ReplaceAll(FINDREPLACE *pfr)
{
  _ASSERTE(number > 0);
 
  int replaced = 0;
  int retval = SendMessage(docs[idArray[current]].hwnd, SE_FINDREPLACE, (WPARAM)pfr, (LPARAM)&replaced);

  CNotepad &Notepad = CNotepad::Instance();
  if (retval < 0)
  {
    CError::SetError(CError::TranslateEditor(retval));
    return false;
  }

  wchar_t msg[32];
  swprintf(msg, DOC_REPLACED, replaced);

  Notepad.Interface.Dialogs.MsgBox(docs[idArray[current]].hwnd, msg, INFO_CAPTION, MB_OK);

  return true;
}


// Function sets a flag for each document, except, perhaps, the current
void CDocuments::SetFlags(unsigned int flag, bool includeCurrent)
{
  for (int i = 0; i < number; i++)
  {
    if (i == current)
      continue;
    docs[idArray[i]].flag |= flag;
  }
  if (includeCurrent && current != number)
    docs[idArray[current]].flag |= flag;
}

// Function sets document as modified
bool CDocuments::SetModified(int index, bool isModified)
{
  if (!DocumentWithIndexExist(index))
  {
    CError::SetError(NOTEPAD_DOCUMENT_NOT_EXIST);
    return false;
  }

  int id = idArray[index];

  if (docs[id].modified && isModified)
    return true;
  if (!docs[id].modified && !isModified)
    return true;

  docs[id].modified = isModified;
  if (isModified && (docs[id].name.length() < DMS_LEN || 
      docs[id].name.compare(docs[id].name.length() - DMS_LEN, DMS_LEN, DOC_MODIFIED_STR) != 0))
    docs[id].name += DOC_MODIFIED_STR;
  else if (!isModified && docs[id].name.length() >= DMS_LEN &&
           docs[id].name.compare(docs[id].name.length() - DMS_LEN, DMS_LEN, DOC_MODIFIED_STR) == 0)
    docs[id].name.erase(docs[id].name.length() - DMS_LEN, DMS_LEN);

  CNotepad &Notepad = CNotepad::Instance();
  if (Notepad.Interface.Tabs.Available())
    Notepad.Interface.Tabs.SetModified(index, isModified);

  return true;
}

// Function sets BOM flag
bool CDocuments::SetBOM(int index, bool needBOM)
{
  if (!DocumentWithIndexExist(index))
  {
    CError::SetError(NOTEPAD_DOCUMENT_NOT_EXIST);
    return false;
  }

  docs[idArray[index]].BOM = needBOM;
  return true;
}

bool CDocuments::SetWordWrap(int index, bool turnON)
{
  if (!DocumentWithIndexExist(index))
  {
    CError::SetError(NOTEPAD_DOCUMENT_NOT_EXIST);
    return false;
  }

  if (IsBusy(index))
  {
    docs[idArray[index]].flag |= DOC_SETWORDWRAP;
    docs[idArray[index]].actions.isWordWrap = turnON;
    return true;
  }

  SendMessage(docs[idArray[index]].hwnd, SE_SETWORDWRAP, turnON ? 1 : 0, 0);
  docs[idArray[index]].flag &= (~DOC_SETWORDWRAP);
  return true;
}

bool CDocuments::SetSmartInput(int index, bool turnON)
{
  if (!DocumentWithIndexExist(index))
  {
    CError::SetError(NOTEPAD_DOCUMENT_NOT_EXIST);
    return false;
  }

  if (IsBusy(index))
  {
    docs[idArray[index]].flag |= DOC_SETSMARTINPUT;
    docs[idArray[index]].actions.isSmartInput = turnON;
    return true;
  }

  SendMessage(docs[idArray[index]].hwnd, SE_SETSMARTINPUT, turnON ? 1 : 0, 0);
  docs[idArray[index]].flag &= (~DOC_SETSMARTINPUT);
  return true;
}

bool CDocuments::SetDragMode(int index, bool turnON)
{
  if (!DocumentWithIndexExist(index))
  {
    CError::SetError(NOTEPAD_DOCUMENT_NOT_EXIST);
    return false;
  }

  if (IsBusy(index))
  {
    docs[idArray[index]].flag |= DOC_SETDRAGMODE;
    docs[idArray[index]].actions.isDragMode = turnON;
    return true;
  }

  SendMessage(docs[idArray[index]].hwnd, SE_SETDRAGMODE, turnON ? 1 : 0, 0);
  docs[idArray[index]].flag &= (~DOC_SETDRAGMODE);

  if (index == current)
  {
    CNotepad &Notepad = CNotepad::Instance();
    if (Notepad.Interface.Controlbar.IsToolbar() && 
        Notepad.Interface.Controlbar.IsTBButtonChecked(CMD_DRAGMODE) != turnON)
      Notepad.Interface.Controlbar.SetTBButtonState(CMD_DRAGMODE, turnON, true);
  }

  return true;
}

bool CDocuments::SetScrollByLine(int index, bool turnON)
{
  if (!DocumentWithIndexExist(index))
  {
    CError::SetError(NOTEPAD_DOCUMENT_NOT_EXIST);
    return false;
  }

  if (IsBusy(index))
  {
    docs[idArray[index]].flag |= DOC_SETSCROLLBYLINE;
    docs[idArray[index]].actions.isScrollByLine = turnON;
    return true;
  }

  SendMessage(docs[idArray[index]].hwnd, SE_SETSCROLLBYLINE, turnON ? 1 : 0, 0);
  docs[idArray[index]].flag &= (~DOC_SETSCROLLBYLINE);
  return true;
}

bool CDocuments::SetFont(int index, HFONT hfont)
{
  if (!DocumentWithIndexExist(index))
  {
    CError::SetError(NOTEPAD_DOCUMENT_NOT_EXIST);
    return false;
  }

  if (IsBusy(index))
  {
    docs[idArray[index]].flag |= DOC_SETFONT;
    return true;
  }

  options_t options;
  options.type = SEDIT_SETFONT;
  options.hfont = hfont;
  SendMessage(docs[idArray[index]].hwnd, SE_SETOPTIONS, (WPARAM)&options, 0);
  
  docs[idArray[index]].flag &= (~DOC_SETFONT);
  return true;
}

bool CDocuments::SetLeftIndent(int index, int leftIndent)
{
  if (!DocumentWithIndexExist(index))
  {
    CError::SetError(NOTEPAD_DOCUMENT_NOT_EXIST);
    return false;
  }

  if (IsBusy(index))
  {
    docs[idArray[index]].flag |= DOC_SETLEFTINDENT;
    return true;
  }

  options_t options;
  options.type = SEDIT_SETLEFTINDENT;
  options.leftIndent = leftIndent;
  SendMessage(docs[idArray[index]].hwnd, SE_SETOPTIONS, (WPARAM)&options, 0);
  
  docs[idArray[index]].flag &= (~DOC_SETLEFTINDENT);
  return true;
}

bool CDocuments::SetTabSize(int index, int tabSize)
{
  if (!DocumentWithIndexExist(index))
  {
    CError::SetError(NOTEPAD_DOCUMENT_NOT_EXIST);
    return false;
  }

  if (IsBusy(index))
  {
    docs[idArray[index]].flag |= DOC_SETTABSIZE;
    return true;
  }

  options_t options;
  options.type = SEDIT_SETTAB;
  options.tabSize = tabSize;
  SendMessage(docs[idArray[index]].hwnd, SE_SETOPTIONS, (WPARAM)&options, 0);
  
  docs[idArray[index]].flag &= (~DOC_SETTABSIZE);
  return true;
}

// Function activates document with the given index
bool CDocuments::SetActive(int index)
{
  if (!DocumentWithIndexExist(index))
  {
    CError::SetError(NOTEPAD_DOCUMENT_NOT_EXIST);
    return false;
  }

  int id = idArray[index];

  processFlag(id);

  if (idArray[current] >= 0 && index != current) // current window exists (it may be closed recently)
    ShowWindow(docs[idArray[current]].hwnd, SW_HIDE);
  ShowWindow(docs[id].hwnd, SW_SHOW);

  if (current != index)
  {
    docs[id].prev = idArray[current] >= 0 ? idArray[current] : docs[id].prev;
    current = index;
  }
  SetFocus();
  

  CNotepad &Notepad = CNotepad::Instance();

  // set appropriate tab as active
  if (Notepad.Interface.Tabs.Available())
    Notepad.Interface.Tabs.SetActive(current);

  // set appropriate coordiantes in the status bar
  if (Notepad.Interface.StatusBar.IsStatusBar())
  {
    unsigned int x, y;
    GetCaretPos(current, x, y, Notepad.Options.CaretRealPos());
    Notepad.Interface.StatusBar.SetPosition(x, y);
  }

  // update buttons on a toolbar
  if (Notepad.Interface.Controlbar.IsToolbar())
    Notepad.Interface.Controlbar.UpdateDocDependButtons();

  return true;
}

// Function moves window with the given index
// * Parameter validate allows Simple Edit Control to validate unaltered area
bool CDocuments::Move(int index, int x, int y, int clientX, int clientY, bool repaint, bool validate)
{
  if (!DocumentWithIndexExist(index))
  {
    CError::SetError(NOTEPAD_DOCUMENT_NOT_EXIST);
    return false;
  }

  CNotepad &Notepad = CNotepad::Instance();

  char isCaretVisible = 0;
  if (!IsBusy(index))
  {
    isCaretVisible = (char)SendMessage(docs[idArray[index]].hwnd, SE_ISCARETVISIBLE, 0, 0);
    /*if (isCaretVisible < 0)
    {
      CError::SetError(CError::TranslateEditor(isCaretVisible));
      return false;
    }*/
  }

#ifndef _WIN32_WCE
  validate = false; // on PC we should never use validate
#endif
  
  // validate is used for pocket pc when the keyboard goes up/down
  if (validate)
  {
    SendMessage(docs[idArray[index]].hwnd, SE_VALIDATE, 1, 0);
    UpdateWindow(docs[idArray[index]].hwnd); // to clear autocomplete bar
  }
  MoveWindow(docs[idArray[index]].hwnd, x, y, clientX, clientY, repaint ? TRUE : FALSE);
  if (validate)
    SendMessage(docs[idArray[index]].hwnd, SE_VALIDATE, 0, 0);

  if (isCaretVisible)
    SendMessage(docs[idArray[index]].hwnd, SE_GOTOCARET, 0, 0);

  return true;
}

bool CDocuments::SetRegexp(int index)
{
  if (!DocumentWithIndexExist(index))
  {
    CError::SetError(NOTEPAD_DOCUMENT_NOT_EXIST);
    return false;
  }

  if (docs[idArray[index]].hthread)
  {
    docs[idArray[index]].flag |= DOC_SETREGEXP;
    return true;
  }
  else
  {
    CNotepad &Notepad = CNotepad::Instance();
    options_t options;

    options.useRegexp = Notepad.Options.UseRegexp() ? 1 : 0;
    options.type = SEDIT_SETUSEREGEXP;
    SendMessage(docs[idArray[index]].hwnd, SE_SETOPTIONS, (WPARAM)&options, 0);

    docs[idArray[index]].flag &= (~DOC_SETREGEXP);
    return true;
  }
}

bool CDocuments::SetWrapType(int index)
{
  if (!DocumentWithIndexExist(index))
  {
    CError::SetError(NOTEPAD_DOCUMENT_NOT_EXIST);
    return false;
  }

  if (docs[idArray[index]].hthread)
  {
    docs[idArray[index]].flag |= DOC_SETWRAPTYPE;
    return true;
  }
  else
  {
    CNotepad &Notepad = CNotepad::Instance();
    options_t options;

    options.wrapType = Notepad.Options.WrapType();
    options.type = SEDIT_SETWRAPTYPE;
    SendMessage(docs[idArray[index]].hwnd, SE_SETOPTIONS, (WPARAM)&options, 0);

    docs[idArray[index]].flag &= (~DOC_SETWRAPTYPE);
    return true;
  }
}

bool CDocuments::SetHideSpaces(int index)
{
  if (!DocumentWithIndexExist(index))
  {
    CError::SetError(NOTEPAD_DOCUMENT_NOT_EXIST);
    return false;
  }

  if (docs[idArray[index]].hthread)
  {
    docs[idArray[index]].flag |= DOC_SETHIDESPACES;
    return true;
  }
  else
  {
    CNotepad &Notepad = CNotepad::Instance();
    options_t options;

    options.hideSpaceSyms = Notepad.Options.HideSpaceSyms() ? 1 : 0;
    options.type = SEDIT_SETHIDESPACES;
    SendMessage(docs[idArray[index]].hwnd, SE_SETOPTIONS, (WPARAM)&options, 0);

    docs[idArray[index]].flag &= (~DOC_SETHIDESPACES);
    return true;
  }
}

bool CDocuments::SetPermanentScrollBars(int index)
{
  if (!DocumentWithIndexExist(index))
  {
    CError::SetError(NOTEPAD_DOCUMENT_NOT_EXIST);
    return false;
  }

  if (docs[idArray[index]].hthread)
  {
    docs[idArray[index]].flag |= DOC_SETSCROLLBARS;
    return true;
  }
  else
  {
    CNotepad &Notepad = CNotepad::Instance();
    options_t options;

    options.permanentScrollBars = Notepad.Options.PermanentScrollBars() ? 1 : 0;
    options.type = SEDIT_SETSCROLLBARS;
    SendMessage(docs[idArray[index]].hwnd, SE_SETOPTIONS, (WPARAM)&options, 0);

    docs[idArray[index]].flag &= (~DOC_SETSCROLLBARS);
    return true;
  }
}

bool CDocuments::MoveCaret(int index, int count, bool left)
{
  if (!DocumentWithIndexExist(index))
  {
    CError::SetError(NOTEPAD_DOCUMENT_NOT_EXIST);
    return false;
  }

  if (!IsBusy(index))
  {
    SendMessage(docs[idArray[index]].hwnd, SE_MOVECARET, count, left ? 1 : 0);
    return true;
  }
  else 
  {
    CError::SetError(NOTEPAD_DOCUMENT_IS_BUSY);
    return false;
  }
}

bool CDocuments::SetCaretPos(int index, unsigned int x, unsigned int y, bool real, bool moveScreenToCaret)
{
  if (!DocumentWithIndexExist(index))
  {
    CError::SetError(NOTEPAD_DOCUMENT_NOT_EXIST);
    return false;
  }

  if (IsBusy(index))
  {
    docs[idArray[index]].flag |= DOC_SETCARETPOS;
    docs[idArray[index]].actions.realCaretPos = real;
    docs[idArray[index]].actions.caretPos.x = x;
    docs[idArray[index]].actions.caretPos.y = y;
    return true;
  }

  doc_info_t di;
  di.flags = real ? SEDIT_SETCARETREALPOS : SEDIT_SETCARETPOS;
  di.caretPos.x = x;
  di.caretPos.y = y;
  SendMessage(docs[idArray[index]].hwnd, SE_SETPOSITIONS, (WPARAM)&di, 0);

  docs[idArray[index]].flag &= (~DOC_SETCARETPOS);

  if (moveScreenToCaret)
    SendMessage(docs[idArray[index]].hwnd, SE_GOTOCARET, 0, 0);

  return true;
}

bool CDocuments::SetScreenPos(int index, unsigned int x, unsigned int y)
{
  if (!DocumentWithIndexExist(index))
  {
    CError::SetError(NOTEPAD_DOCUMENT_NOT_EXIST);
    return false;
  }

  if (IsBusy(index))
  {
    docs[idArray[index]].flag |= DOC_SETSCREENPOS;
    docs[idArray[index]].actions.screenPos.x = x;
    docs[idArray[index]].actions.screenPos.y = y;
    return true;
  }

  doc_info_t di;
  di.flags = SEDIT_SETWINDOWPOS;
  di.windowPos.x = x;
  di.windowPos.y = y;
  SendMessage(docs[idArray[index]].hwnd, SE_SETPOSITIONS, (WPARAM)&di, 0);

  docs[idArray[index]].flag &= (~DOC_SETSCREENPOS);
  return true;
}

// Function saves all documents
bool CDocuments::SaveAllDocuments(bool warn)
{
  CNotepad &Notepad = CNotepad::Instance();
  std::wstring path;
  int id;
  LPCWSTR codingId;
  bool retval = true;

  for (int i = 0; i < number; i++)
  {
    id = idArray[i];
    _ASSERTE(id >= 0);

    if (docs[id].modified && docs[id].hthread == 0)
    {
      // warn user if file is modified
      int answer = IDYES;
      if (warn)
      {
        std::wstring warning = DOC_FILE_NOT_SAVED;
        warning += docs[id].name;
        warning += L"?";
        answer = Notepad.Interface.Dialogs.MsgBox(Notepad.Interface.GetHWND(), 
                                                  warning.c_str(), 
                                                  WARNING_CAPTION, MB_YESNOCANCEL);
      }

      if (answer == IDYES)
      {
        path = docs[id].path;
        codingId = docs[id].cp;
        if (path.empty() && !Notepad.Interface.Dialogs.SaveFileDialog(Notepad.Interface.GetHWND(), docs[id].name, path, codingId, docs[id].BOM, Notepad.Options.OwnDialogs()))
          continue;
        if (!Save(i, path, codingId, docs[id].BOM))
        {
          Notepad.Interface.Dialogs.ShowError(Notepad.Interface.GetHWND(), 
                                              CError::GetError().message);
          CError::SetError(NOTEPAD_NO_ERROR);
          retval = false;
        }
      }
      else if (answer == IDCANCEL)
        return false; // at least one doc is not saved
    }
  }

  return retval;
}

// Function closes all documents
void CDocuments::CloseAllDocuments()
{
  CNotepad &Notepad = CNotepad::Instance();

  for (int i = number - 1; i >= 0; i--)
  {
    if (!Close(i) && CError::GetError().code != NOTEPAD_NO_ERROR)
    {
        Notepad.Interface.Dialogs.ShowError(Notepad.Interface.GetHWND(), 
                                            CError::GetError().message);
        CError::SetError(NOTEPAD_NO_ERROR);
    }
  }
}

// Function changes font in all documents
void CDocuments::SetDocumentsFont(HFONT hfont)
{
  options_t options;
  options.type = SEDIT_SETFONT;
  options.hfont = hfont;

  for (int i = 0; i < number; i++)
  {
    if (IsBusy(i))
    {
      docs[idArray[i]].flag |= DOC_SETFONT;
      continue;
    }
    SendMessage(docs[idArray[i]].hwnd, SE_SETOPTIONS, (WPARAM)&options, 0);
    docs[idArray[i]].flag &= (~DOC_SETFONT);
  }
}

// Function changes tab size in all documents
void CDocuments::SetDocumentsTabSize(int tabSize)
{
  options_t options;
  options.type = SEDIT_SETTAB;
  options.tabSize = tabSize;

  for (int i = 0; i < number; i++)
  {
    if (IsBusy(i))
    {
      docs[idArray[i]].flag |= DOC_SETTABSIZE;
      continue;
    }
    SendMessage(docs[idArray[i]].hwnd, SE_SETOPTIONS, (WPARAM)&options, 0);
    docs[idArray[i]].flag &= (~DOC_SETTABSIZE);
  }
}

// Function changes left indent in all documents
void CDocuments::SetDocumentsLeftIndent(int leftIndent)
{
  options_t options;
  options.type = SEDIT_SETLEFTINDENT;
  options.leftIndent = leftIndent;

  for (int i = 0; i < number; i++)
  {
    if (IsBusy(i))
    {
      docs[idArray[i]].flag |= DOC_SETLEFTINDENT;
      continue;
    }
    SendMessage(docs[idArray[i]].hwnd, SE_SETOPTIONS, (WPARAM)&options, 0);
    docs[idArray[i]].flag &= (~DOC_SETLEFTINDENT);
  }
}


// Function sets hook functions for open and save operations
void CDocuments::SetProgressFunctions()
{
  ::SetFileCallbacks(
      CInterface::SetMaxProgress,
      CInterface::SetProgress, 
      CInterface::ProgressEnd, 
      CCodingManager::DecodeFunction, 
      CCodingManager::EncodeFunction,
      CCodingManager::BomFunction, 
      CCodingManager::DeterminCodingFunction, 
      CCodingManager::InitializeCodingFunction,
      CCodingManager::FinalizeCodingFunction
  );
}

// Function waits for all threads will be over
bool CDocuments::WaitAll(unsigned int backMsg)
{
  CNotepad &Notepad = CNotepad::Instance();
  SWaitThreadData *pdata = new SWaitThreadData;

  pdata->hwndOwner = Notepad.Interface.GetHWND();
  pdata->pdocs = docs;
  pdata->backMsg = backMsg;

  DWORD threadID;
  HANDLE hthread = CreateThread(0, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(waitThreadProc), 
                                reinterpret_cast<void*>(pdata), 0, &threadID);
  if (!hthread)
  {
    delete pdata;
    CError::SetError(NOTEPAD_CREATE_THREAD_ERROR);
    return false;
  }
  CloseHandle(hthread);

  return true;
}

// open file thread procedure
unsigned int _stdcall CDocuments::openThreadProc(void *params)
{
  SOpenThreadData *pdata = reinterpret_cast<SOpenThreadData*>(params);
  int translatedError = NOTEPAD_NO_ERROR;
  char fileError;
  char isBOM;

  Encoder *encoder = CNotepad::Instance().Documents.CodingManager.GetEncoderById(pdata->pdoc->cp);
  encoder = (Encoder*)LoadFile(pdata->pdoc->hwnd, pdata->pdoc->path.c_str(), encoder, pdata->useFastReadMode ? 1 : 0, &isBOM, &fileError);

  if (fileError != FILES_NO_ERROR)
    translatedError = CError::TranslateFile(fileError);
  else
  {
    pdata->pdoc->cp = encoder->GetCodingId();
    pdata->pdoc->BOM = isBOM ? true : false;
  }

  SOpenEndData *preturnData = new SOpenEndData;
  preturnData->docId = pdata->id;
  preturnData->error = translatedError;
  preturnData->openType = pdata->openType;

  PostMessage(pdata->hwndOwner, NPD_OPENTHREADEND, 0, (LPARAM)preturnData);

  delete pdata;
  
  return 0;
}


// thread procedure for saving document
unsigned int _stdcall CDocuments::saveThreadProc(void *params)
{
  SSaveThreadData *pdata = reinterpret_cast<SSaveThreadData*>(params);

  Encoder *encoder = CNotepad::Instance().Documents.CodingManager.GetEncoderById(pdata->cp);
  int error = SaveFile(pdata->pdoc->hwnd, pdata->path.c_str(), encoder, CCodingManager::LINE_BREAK, pdata->pdoc->BOM ? 1 : 0);

  if (error != FILES_NO_ERROR)
    error = CError::TranslateFile(error);
  else
  {
    pdata->pdoc->cp = pdata->cp;
    pdata->pdoc->path = pdata->path;
  }
  PostMessage(pdata->hwndOwner, NPD_SAVETHREADEND, pdata->id, error);
  delete pdata;
  
  return 0;
}

// thread procedure for waiting documents
unsigned int _stdcall CDocuments::waitThreadProc(void *params)
{
  SWaitThreadData *pdata = reinterpret_cast<SWaitThreadData*>(params);
  //HANDLE hthread;

  for (int i = 0; i < MAX_DOCUMENTS; i++)
  {
    //hthread = pdata->pdocs[i].hthread;
    //if (hthread)
      //WaitForSingleObject(hthread, INFINITE);
    while (pdata->pdocs[i].hthread)
      Sleep(16);
  }
  PostMessage(pdata->hwndOwner, pdata->backMsg, 0, 0);
  delete pdata;

  return 0;
}


HMENU SEAPIENTRY CDocuments::createPopupMenuFunc(HWND hwndFrom, unsigned int idFrom)
{
  CNotepad &Notepad = CNotepad::Instance();

  HMENU hmenu = LoadMenu(g_hResInstance, MAKEINTRESOURCE(IDM_EDITORPOPUP_MENU));
  if (!hmenu)
  {
    std::wstring message;
    CError::GetErrorMessage(message, NOTEPAD_LOAD_RESOURCE_ERROR);
    Notepad.Interface.Dialogs.ShowError(hwndFrom, message);
    return 0;
  }

  // Check enabled/disabled points
  Notepad.ModifyEditorPopupMenu(hmenu);

  return hmenu;
}

void SEAPIENTRY CDocuments::destroyPopupMenuFunc(HWND hwndFrom, unsigned int idFrom, HMENU hmenu)
{
  if (hmenu != 0)
    DestroyMenu(hmenu);
}

char SEAPIENTRY CDocuments::receiveMessageFunc(HWND hwndFrom, unsigned int idFrom, unsigned int message, WPARAM wParam, LPARAM lParam, LRESULT *presult)
{
  if (WM_COMMAND == message)
  {
    CNotepad &Notepad = CNotepad::Instance();
    SendMessage(Notepad.Interface.GetHWND(), message, wParam, lParam);
  }
  else if (WM_KEYDOWN == message)
  {
    CNotepad &Notepad = CNotepad::Instance();

    const std::list<InternalPlugin*> plugins = Notepad.PluginManager.ListPlugins();

    if (plugins.size() > 0)
    {
      bool isShiftPressed = GetKeyState(VK_SHIFT) < 0;
      bool isCtrlPressed = GetKeyState(VK_CONTROL) < 0;
      wchar_t keyPressed = wParam;

      for (std::list<InternalPlugin*>::const_iterator iter = plugins.begin(); iter != plugins.end(); iter++)
      {
        InternalPlugin *internalPlugin = *iter;
  
        int commandsNumber = internalPlugin->GetPlugin()->GetSupportedCommandsNumber();
        const PluginCommand *commands = internalPlugin->GetPlugin()->GetSupportedCommands();

        for (int i = 0; i < commandsNumber; i++)
        {
          PluginHotKey *hotKey = commands[i].hotKey;
          if (hotKey && hotKey->key == keyPressed && hotKey->ctrl == isCtrlPressed && hotKey->shift == isShiftPressed)
          {
            internalPlugin->GetPlugin()->ExecuteCommand(commands[i].name);
            *presult = 0;
            return 1;
          }
        }
      }
    }
  }
  return 0;
}