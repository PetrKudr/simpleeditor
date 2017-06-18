#ifndef __DOCUMENTS_
#define __DOCUMENTS_

#include <windows.h>
#include <string>
#include <vector>

#include "codingmanager.hpp"
#include "seditcontrol.h"

#ifdef _WIN32_WCE
#include "ibase.hpp"
#endif


const int MAX_DOCUMENTS = 20; // maximal number of documents

const wchar_t DOC_MODIFIED_STR[] = L" *";
const int DMS_LEN = sizeof(DOC_MODIFIED_STR) / sizeof(wchar_t) - 1;        // length of the DOC_MODIFIED_STR


// Style of a new document
#define DOCUMENT_WINDOW_STYLE (WS_VSCROLL | WS_HSCROLL | WS_CHILD)

// Values for Open Document function
enum OpenType
{
  OPEN,
  REOPEN,
  RECENT
};

struct SOpenEndData
{
  int docId;
  int error;
  OpenType openType;
};


// Values for the flag field
enum DocumentFlag
{
  DOC_RESIZE    = 1,    // document will be resized when it becomes active or enabled(if it is disabled now)
  DOC_CLOSE     = 2,    // document will be closed when open/save operation is over
  DOC_SETREGEXP = 4,    // regexp option was changed while document was busy
  DOC_SETSCROLLBARS = 8,
  DOC_SETHIDESPACES = 16,
  DOC_SETWRAPTYPE = 32,

  DOC_GOTOCARET = 64,     // make caret visible
  DOC_SETCARETPOS  = 128,  // set caret position
  DOC_SETSCREENPOS = 256, // set screen position
  DOC_SETWORDWRAP  = 512,
  DOC_SETDRAGMODE = 1024,
  DOC_SETSMARTINPUT = 2048,
  DOC_SETSCROLLBYLINE = 4096,
  DOC_SETFONT = 8192,
  DOC_SETTABSIZE = 16384,
  DOC_SETLEFTINDENT = 32768
};

struct SDocActions
{
  position_t caretPos;  // for DOC_SETCARETPOS
  bool realCaretPos;

  position_t screenPos; // for DOC_SETSCREENPOS

  bool isWordWrap;
  bool isSmartInput;
  bool isDragMode;
  bool isScrollByLine;
};

struct document_t
{
  HWND hwnd;          // editor window
  std::wstring name;  // document name
  std::wstring path;  // file path
  LPCWSTR cp;         // id of code page
  int prev;           // previous document (id)
  bool modified;      // modification flag
  bool BOM;           // write byte order mark

  SDocActions actions; // actions/individual proporties of a document

  int maxProgress;    // for progress bar
  volatile HANDLE hthread;     // for open/save procedures
  unsigned int flag;  // flags
};


class CDocuments
{
private:
  document_t docs[MAX_DOCUMENTS];
  int indexArray[MAX_DOCUMENTS]; // this array allows to get document index from its id
  int idArray[MAX_DOCUMENTS];     // this array allows to get document id from its index
  int current;   // index of the current document;
  int number;    // number of documents;

  /*
   * Id of the document is its index in docs array. (And id of the editor)
   * Index of the document is its position in imaginary list of documents (it is equal to list of documents in gui)
   */

public:

  CCodingManager CodingManager; 


private:

  int findPlace();
  void processFlag(int id);
  void clearDoc(document_t &doc);
  void getFileName(const std::wstring &path, std::wstring &name);
  void setDocOptions(const document_t &doc);
  int findDocumentByPath(const std::wstring &path);

  static unsigned int _stdcall openThreadProc(void *params);
  static unsigned int _stdcall saveThreadProc(void *params);
  static unsigned int _stdcall waitThreadProc(void *params);

  static HMENU SEAPIENTRY createPopupMenuFunc(HWND hwndFrom, unsigned int idFrom);
  static void SEAPIENTRY destroyPopupMenuFunc(HWND hwndFrom, unsigned int idFrom, HMENU hmenu);

  static char SEAPIENTRY receiveMessageFunc(HWND hwndFrom, unsigned int idFrom, unsigned int message, WPARAM wParam, LPARAM lParam, LRESULT *presult);

public:
  CDocuments();
  ~CDocuments();

  // Actions for a document
  bool Create(const std::wstring &caption);
  bool Close(int index);
  bool Open(const std::wstring &path, LPCWSTR cp, OpenType openType, bool activate = true, bool openInCurrent = false, bool checkExisting = true);
  void OpenEnd(SOpenEndData *popenEndData);
  bool Save(int index, const std::wstring &path, LPCWSTR cp, bool BOM);
  void SaveEnd(int id, bool isError);
  bool SetModified(int index, bool isModified);
  bool SetBOM(int index, bool needBOM);
  bool SetActive(int index);
  bool Move(int index, int x, int y, int clientX, int clientY, bool repaint = true, bool validate = false);
  bool SetRegexp(int index);
  bool SetWrapType(int index);
  bool SetHideSpaces(int index);
  bool SetPermanentScrollBars(int index);
  bool MoveCaret(int index, int count, bool left);
  bool SetCaretPos(int index, unsigned int x, unsigned int y, bool real, bool moveScreenToCaret);
  bool SetScreenPos(int index, unsigned int x, unsigned int y);
  bool SetWordWrap(int index, bool turnON);
  bool SetSmartInput(int index, bool turnON);
  bool SetDragMode(int index, bool turnON);
  bool SetScrollByLine(int index, bool turnON);
  bool SetFont(int index, HFONT hfont);
  bool SetTabSize(int index, int tabSize);
  bool SetLeftIndent(int index, int leftIndent);

  // Information about document
  bool DocumentWithIdExist(int id);
  bool DocumentWithIndexExist(int index);
  HWND GetHWND(int index);
  const std::wstring& GetName(int index);
  const std::wstring& GetPath(int index);
  LPCWSTR GetCoding(int index);
  void GetCaretPos(int index, unsigned int &x, unsigned int &y, bool real);
  void GetWindowPos(int index, unsigned int &x, unsigned int &y);
  bool IsModified(int index);
  bool IsBusy(int index);
  bool NeedBOM(int index);
  bool IsWordWrap(int index);
  bool IsSmartInput(int index);
  bool IsDragMode(int index);
  bool IsScrollByLine(int index);
  unsigned int GetDocLinesNumber(int index, bool real);
  unsigned int GetDocLineLength(int index, unsigned int number, bool real);


  // Actions for the current document
  void SetFocus();
  void Undo();
  void Redo();
  void Copy();
  void Cut();
  void Delete();
  void Paste();
  void Paste(const wchar_t *buff);
  void SelectAll();
  void GotoBegin();
  void GotoEnd();
  void StartUndoGroup();
  void EndUndoGroup();
  wchar_t* GetSelection();
  bool FindReplace(FINDREPLACE *pfr, bool fromEdge);
  bool ReplaceAll(FINDREPLACE *pfr);

  // Information about current document
  bool IsSelection();
  bool CanUndo();
  bool CanRedo();
  bool CanPaste();
  int Current();

  // Actions applied for all documents
  bool SaveAllDocuments(bool warn);
  void CloseAllDocuments();
  void SetFlags(unsigned int flag, bool includeCurrent);
  void SetDocumentsFont(HFONT hfont);
  void SetDocumentsTabSize(int tabSize);
  void SetDocumentsLeftIndent(int leftIndent);

  // other
  bool WaitAll(unsigned int backMsg);
  void SetProgressFunctions();
  const wchar_t* AutoName();
  int GetIndexFromID(int id);
  int GetIdFromIndex(int index);
  int GetDocsNumber();
  void GetDocIds(__out std::vector<int> &ids);
};


#endif /* __DOCUMENTS_ */