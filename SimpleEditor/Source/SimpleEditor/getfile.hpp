#ifndef __GETFILE_
#define __GETFILE_

#include "ibase.hpp"

const int GETFILE_CANCEL            = 0;
const int GETFILE_SUCCESS           = 1;
const int GETFILE_MEMORY_ERROR      = -1;
const int GETFILE_FILL_FIELDS_ERROR = -2;

// structure for open/save file dialogs
struct SGetFileName
{
  HWND hwndOwner;
  wchar_t *path;
  size_t pathSize;
  wchar_t *initialDir;
  wchar_t *filter;
  int filterIndex;
  LPCWSTR codePage;
  bool BOM;
  bool saveDialog;
};

extern int GetFileName(SGetFileName *gfn);

#endif /* __GETFILE_ */