#ifndef __DIALOGS_
#define __DIALOGS_

#include "options.hpp"
#include "getfile.hpp"
#include "ibase.hpp"

#pragma warning(disable:4996)


class CDialogs
{
  wchar_t filter[FILTER_SIZE];
  wchar_t dialogPath[MAX_PATH];
  int iOpenFilter, iSaveFilter;


private:

  enum ButtonPosition {
    LEFT,
    CENTER,
    RIGHT
  };


private:

  static int moveButton(HWND hwnd, RECT *parentRect, ButtonPosition position);

  static UINT_PTR CALLBACK MsgOKDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);
  static UINT_PTR CALLBACK MsgYesNoDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);
  static UINT_PTR CALLBACK MsgYesNoCancelDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);
  static UINT_PTR CALLBACK AboutDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);
  static UINT_PTR CALLBACK GetCodingDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);
  static UINT_PTR CALLBACK GotoLineDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);
  static UINT_PTR CALLBACK FileInfoDialogProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);


private:

  void addExtention(std::wstring &path, int iFilter);


public:

  static int ShowError(HWND hwnd, const std::wstring &errmsg);
  static int MsgBox(HWND hwnd, const wchar_t *message, const wchar_t *caption, int type);  


public:

  CDialogs()
  {
    memcpy(filter, L"All\0*.*\0Text\0*.txt\0", 20 * sizeof(wchar_t));
    iOpenFilter = iSaveFilter = 1;
    dialogPath[0] = L'\0';
  }

  void SetFileFilters(const wchar_t *filter, size_t len, int OpenFilter, int SaveFilter) 
  {
    memcpy(this->filter, filter, len * sizeof(wchar_t));
    this->iOpenFilter = OpenFilter;
    this->iSaveFilter = SaveFilter;
  }

  void SetDialogsPath(std::wstring &path)
  {
    if (path.size() < MAX_PATH)
      wcscpy(dialogPath, path.c_str());
  }

  const wchar_t *GetDialogsPath()
  {
    return dialogPath;
  }

  bool OpenFileDialog(HWND hwnd, std::wstring &path, LPCWSTR &cp, bool owndDialog);
  bool SaveFileDialog(HWND hwnd, const std::wstring &name, std::wstring &path, LPCWSTR &cp, bool &BOM, bool ownDialog);
  void AboutDialog(HWND hwnd);
  void FileInfoDialog(HWND hwnd, const std::wstring &path, int size);

  bool StdOpenFileDialog(HWND hwnd, std::wstring &path, LPCWSTR &cp);
  bool StdSaveFileDialog(HWND hwnd, const std::wstring &name, std::wstring &path, LPCWSTR &cp, bool &BOM);
  bool OwnOpenFileDialog(HWND hwnd, std::wstring &path, LPCWSTR &cp);
  bool OwnSaveFileDialog(HWND hwnd, const std::wstring &name, std::wstring &path, LPCWSTR &cp, bool &BOM);

  void GotoLineDialog(HWND hwnd, CDocuments &Documents, bool realLines);
  void OptionsDialog(HWND hwnd, COptions &Options);
};


#endif /* __OPTIONS_ */