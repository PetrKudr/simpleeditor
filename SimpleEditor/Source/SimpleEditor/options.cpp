#include "notepad.hpp"

#ifndef _WIN32_WCE
#include "base\codecvt.hpp"
#endif

#ifndef _WIN32_WCE
#define DIALOG_PATH L"C:\\"
#else
#define DIALOG_PATH L"\\"
#endif


const int END_OF_FILE        = 0;
const int SECTION_SKIP       = 1;
const int SECTION_COMMON     = 2;
const int SECTION_APPEARANCE = 3;
const int SECTION_PROFILE    = 4;
const int SECTION_MISC       = 5;


// Constructor sets all options by default
COptions::COptions()
{
  optionsChanged = false;

  isToolbarTransparent = true;
  bugWithPopupMenusExists = false;
  saveRecentImmediately = false;
  needBOM = true;
  createEmptyIfNoDocuments = true;
  switchToExistingOnOpen = true;
  isFullScreen = false;
  isToolbar = true;
  isStatusBar = false;
  useRegexp = false;
  hideSpaceSyms = true;
  permanentScrollBars = false;  
  caretRealPos = false;
  kineticScroll = true;
  defaultWordWrap = false;
  defaultSmartInput = false;
  defaultDragMode = false;
  defaultScrollByLine = false;
  ownDialogs = true;
  responseOnSIP = true;
  saveSession = false;
  useFastReadMode = false;
  sipOnFindBar = true;
  defaultCP = CCodingManager::CODING_DEFAULT_ID;
  caretWidth = 2;
  leftIndent = 0;
  minForVisibleTabs = 1;
  maxRecent = 10;
  openFileFilter = 0;
  saveFileFilter = 0;
  okButton = OK_BUTTON_SMARTCLOSE;
  docMenuPos = MENUPOS_SCRDOWNRIGHT;
  qinsertMenuPos = MENUPOS_SCRDOWNLEFT;
  pluginsMenuPos = MENUPOS_SCRDOWNRIGHT;
  wrapType = SEDIT_WORDWRAP;    
  langLib = DEFAULT_LANGUAGE_PATH;
  breakSyms = L" \t,;.";
  dialogPath = DIALOG_PATH;
  undoHistorySize = 20 * 1024;   // 20 kb

  wcsncpy(typeStr, L"All%*%Text%*.txt%", 18);
  typeStrSize = 0;
  while (typeStr[typeStrSize])
  {
    if (typeStr[typeStrSize] == L'%')
      typeStr[typeStrSize] = L'\0';
    ++typeStrSize;
  }
  ++typeStrSize;

  CNotepad &Notepad = CNotepad::Instance();
  SProfile profile;
 
  profile.TabColors = Notepad.Interface.Tabs.GetColors();
  profile.htabFont = Notepad.Interface.Tabs.GetFont();
  profile.heditorFont = (HFONT)GetStockObject(SYSTEM_FONT);
  profile.tabSize = 4;
  profile.tabWindowHeight = 0;
  profile.EditorTextColor = RGB(0, 0, 0);
  profile.EditorCaretTextColor = RGB(0, 0, 0);
  profile.EditorCaretBgColor = RGB(255, 255, 255);
  profile.EditorSelColor = RGB(70, 140, 255);
  profile.EditorSelTextColor = RGB(255, 255, 255);
  profile.EditorColor = RGB(255, 255, 255);
  profile.MainColor = RGB(255, 255, 255);
  profile.NotFoundEditControlColor = RGB(0xfe, 0xbe, 0x9e);
  profile.ToolbarColor = GetSysColor(COLOR_3DFACE);
  profile.selectionBackgroundTransparent = false;
  profile.selectionTextTransparent = true;
  profile.name = L"default";

  setDefaultFont(profile.ltabFont);
  setDefaultFont(profile.leditorFont);

  profiles.push_back(profile);
  currentProfile = 0;
}

// Function reads strings until eof, returns section id
int COptions::readString(std::wifstream &fIn, std::wstring &str)
{
  if (fIn.eof())
    return END_OF_FILE;
  std::getline(fIn, str);

  size_t firstCharacter = str.find_first_not_of(L"\t ", 0, 2);
  size_t comments = str.find(L'#', 0);

  if (firstCharacter == comments)
    return SECTION_SKIP;  // skip this string if it contains only tabs and spaces or it is comment

  size_t b, e;
  b = str.find_first_of('[');
  e = str.find_first_of(']', b);

  if (b != str.npos && e != str.npos && b == firstCharacter)
  {
    ++b;
    if (e - b == strlen("COMMON") && _wcsnicmp(&(str.c_str()[b]), L"COMMON", strlen("COMMON")) == 0)
      currentSection = SECTION_COMMON;
    else if (e - b == strlen("APPEARANCE") && _wcsnicmp(&(str.c_str()[b]), L"APPEARANCE", strlen("APPEARANCE")) == 0)
      currentSection = SECTION_APPEARANCE;
    else if (e - b == strlen("PROFILES") && _wcsnicmp(&(str.c_str()[b]), L"PROFILES", strlen("PROFILES")) == 0)
      currentSection = SECTION_PROFILE;
    else if (e - b == strlen("MISC") && _wcsnicmp(&(str.c_str()[b]), L"MISC", strlen("MISC")) == 0)
      currentSection = SECTION_MISC;

    return SECTION_SKIP;
  }

  return currentSection; // it's not a comment and not a section => it's parameter
}

// functin gets integer param from the string
int COptions::getIntParam(std::wstring &str, int radix, int byDefault)
{
  int result;

  size_t i = str.find(L'=');
  if (i == str.npos)
    return byDefault;

  if ((i = str.find_first_not_of(L"\t ", i + 1)) == str.npos)
    return byDefault;

  wchar_t *endPtr;
  result = wcstol(&(str.c_str()[i]), &endPtr, radix);

  return result;
}

// function sets string param from the input string
bool COptions::getStringParam(std::wstring &str, std::wstring &param, bool skipSpaces)
{
  size_t i = str.find(L'=');
  if (i == str.npos)
    return false;

  ++i;
  if (skipSpaces && (i = str.find_first_not_of(L"\t\r\n ", i)) == str.npos)
    return false;
  if (i >= str.size())
    return false;

  size_t k = str.find_last_not_of(L"\t\r\n ");
  if (k == str.npos)
    return false;

  ++k;
  if (k < i)
   return false;

  param = k > i ? str.substr(i, k - i) : L"";
  return true;
}

bool COptions::getStringParam(std::wstring &str, wchar_t *param, size_t len, bool skipSpaces)
{
  size_t i = str.find(L'=');
  if (i == str.npos)
    return false;

  ++i;
  if (skipSpaces && (i = str.find_first_not_of(L"\t ", i)) == str.npos)
    return false;
  if (i >= str.size() || str.size() - i >= len)
    return false;

  wcsncpy(param, &str.c_str()[i], str.size() - i);
  
  i = str.size() - i;
  param[i] = L'\0';

  for (int k = 0; k < 2; ++k);
  {
    if (i > 0)
      --i;
    if (i >= 0 && (param[i] == L'\n' || param[i] == L'\r'))
      param[i] = 0;
  }

  return true;
}

void COptions::setDefaultFont(LOGFONT &lf)
{
  lf.lfWidth = 0;
  lf.lfEscapement = 0;
  lf.lfOrientation = 0;
  lf.lfWeight = FW_NORMAL;
  lf.lfItalic = FALSE;
  lf.lfStrikeOut = FALSE;
  lf.lfUnderline = FALSE;
  lf.lfCharSet = ANSI_CHARSET;
  lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
  lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
  lf.lfQuality = DRAFT_QUALITY;
  lf.lfPitchAndFamily = VARIABLE_PITCH | FF_DONTCARE;

/*#ifdef _WIN32_WCE
  lf.lfHeight = -MulDiv(8, g_LogPixelsY, 72);
#else
  lf.lfHeight = -MulDiv(10, g_LogPixelsY, 72);
#endif*/
  lf.lfHeight = 0;

  wcscpy(lf.lfFaceName, L"Tahoma");
}

// function compares two LOGFONT structures
// returns true if they are equal, false otherwise
bool COptions::areLogFontsEqual(const LOGFONT *plf1, const LOGFONT *plf2)
{
  return (memcmp(plf1, plf2, sizeof(LOGFONT)) == 0);
  /*const char *c1 = reinterpret_cast<const char*>(plf1), *c2 = reinterpret_cast<const char*>(plf2);
  for (int i = 0; i < sizeof(LOGFONT); i++)
    if (*c1++ != *c2++)
      return false;
  return true;*/
}

void COptions::setCommonParam(std::wstring &str)
{
  size_t b = str.find_first_not_of(L"\t ", 0, 2);
  size_t e = str.find_first_of(L"\t =", b, 3);

  if (str.compare(b, e - b, L"BugWithPopupMenusExists") == 0)
    bugWithPopupMenusExists = getIntParam(str, 10, bugWithPopupMenusExists ? 1 : 0) ? true : false;
  else if (str.compare(b, e - b, L"SaveRecentImmediately") == 0)
    saveRecentImmediately = getIntParam(str, 10, saveRecentImmediately ? 1 : 0) ? true : false;
  else if (str.compare(b, e - b, L"BOM") == 0)
    needBOM = getIntParam(str, 10, needBOM ? 1 : 0) ? true : false;
  else if (str.compare(b, e - b, L"CreateEmptyIfNoDocuments") == 0)
    createEmptyIfNoDocuments = getIntParam(str, 10, createEmptyIfNoDocuments ? 1 : 0) ? true : false;
  else if (str.compare(b, e - b, L"SwitchToExistingOnOpen") == 0)
    switchToExistingOnOpen = getIntParam(str, 10, switchToExistingOnOpen ? 1 : 0) ? true : false;
  else if (str.compare(b, e - b, L"DefaultWordWrap") == 0)
    defaultWordWrap = getIntParam(str, 10, defaultWordWrap ? 1 : 0) ? true : false;
  else if (str.compare(b, e - b, L"DefaultSmartInput") == 0)
    defaultSmartInput = getIntParam(str, 10, defaultSmartInput ? 1 : 0) ? true : false;
  else if (str.compare(b, e - b, L"DefaultDragMode") == 0)
    defaultDragMode = getIntParam(str, 10, defaultDragMode ? 1 : 0) ? true : false;
  else if (str.compare(b, e - b, L"DefaultScrollByLine") == 0)
    defaultScrollByLine = getIntParam(str, 10, defaultScrollByLine ? 1 : 0) ? true : false;
  else if (str.compare(b, e - b, L"OwnFileDialogs") == 0)
    ownDialogs = getIntParam(str, 10, ownDialogs ? 1 : 0) ? true : false;
  else if (str.compare(b, e - b, L"ResponseOnSIP") == 0)
    responseOnSIP = getIntParam(str, 10, responseOnSIP ? 1 : 0) ? true : false;
  else if (str.compare(b, e - b, L"SaveSession") == 0)
    saveSession = getIntParam(str, 10, saveSession ? 1 : 0) ? true : false;
  else if (str.compare(b, e - b, L"UseFastReadMode") == 0)
    useFastReadMode = getIntParam(str, 10, useFastReadMode ? 1 : 0) ? true : false;
  else if (str.compare(b, e - b, L"DefaultCP") == 0)
    getStringParam(str, defaultCP, true);
  else if (str.compare(b, e - b, L"MaxRecent") == 0)
    maxRecent = getIntParam(str, 10, maxRecent);
  else if (str.compare(b, e - b, L"OkButton") == 0)
    okButton = (char)getIntParam(str, 10, okButton);
  else if (str.compare(b, e - b, L"OpenFilter") == 0)
    openFileFilter = getIntParam(str, 10, openFileFilter);
  else if (str.compare(b, e - b, L"SaveFilter") == 0)
    saveFileFilter = getIntParam(str, 10, saveFileFilter);
  else if (str.compare(b, e - b, L"CaretRealPos") == 0)
    caretRealPos = getIntParam(str, 10, caretRealPos ? 1 : 0) ? true : false;
  else if (str.compare(b, e - b, L"KineticScroll") == 0)
    kineticScroll = getIntParam(str, 10, kineticScroll ? 1 : 0) ? true : false;
  else if (str.compare(b, e - b, L"WrapType") == 0)
    wrapType = getIntParam(str, 10, wrapType);
  else if (str.compare(b, e - b, L"UndoHistorySize") == 0)
    undoHistorySize = (size_t)getIntParam(str, 10, undoHistorySize);
  else if (str.compare(b, e - b, L"LangLib") == 0)
    getStringParam(str, langLib, true);
  else if (str.compare(b, e - b, L"BreakSyms") == 0)
    getStringParam(str, breakSyms, false);
  else if (str.compare(b, e - b, L"TypeStr") == 0)
  {
    getStringParam(str, typeStr, FILTER_SIZE, true);
    typeStrSize = 0;
    while (typeStr[typeStrSize])
    {
      if (typeStr[typeStrSize] == L'%')
        typeStr[typeStrSize] = L'\0';
      ++typeStrSize;
    }
    ++typeStrSize;
  }
}


void COptions::setProfileParam(std::wstring &str)
{
  size_t b = str.find_first_not_of(L"\t ", 0, 2);
  size_t e = str.find_first_of(L"\t =", b, 3);
  int i = profiles.size() - 1;

  _ASSERTE(i >= 0);

  if (str.compare(b, e - b, L"EditorFontName") == 0)
    getStringParam(str, profiles[i].leditorFont.lfFaceName, 32, true);
  else if (str.compare(b, e - b, L"EditorFontHeight") == 0)
    profiles[i].leditorFont.lfHeight = -MulDiv(getIntParam(str, 10, profiles[i].leditorFont.lfHeight), g_LogPixelsY, 72);
  else if (str.compare(b, e - b, L"EditorFontWeight") == 0)
    profiles[i].leditorFont.lfWeight = getIntParam(str, 10, profiles[i].leditorFont.lfWeight);
  else if (str.compare(b, e - b, L"EditorFontCharset") == 0)
    profiles[i].leditorFont.lfCharSet = getIntParam(str, 10, profiles[i].leditorFont.lfCharSet);
  else if (str.compare(b, e - b, L"EditorFontItalic") == 0)
    profiles[i].leditorFont.lfItalic = getIntParam(str, 10, profiles[i].leditorFont.lfItalic);
  else if (str.compare(b, e - b, L"TabFontName") == 0)
    getStringParam(str, profiles[i].ltabFont.lfFaceName, 32, true);
  else if (str.compare(b, e - b, L"TabFontHeight") == 0)
    profiles[i].ltabFont.lfHeight = -MulDiv(getIntParam(str, 10, profiles[i].ltabFont.lfHeight), g_LogPixelsY, 72);
  else if (str.compare(b, e - b, L"TabFontWeight") == 0)
    profiles[i].ltabFont.lfWeight = getIntParam(str, 10, profiles[i].ltabFont.lfWeight);
  else if (str.compare(b, e - b, L"TabFontCharset") == 0)
    profiles[i].ltabFont.lfCharSet = getIntParam(str, 10, profiles[i].ltabFont.lfCharSet);
  else if (str.compare(b, e - b, L"TabFontItalic") == 0)
    profiles[i].ltabFont.lfItalic = getIntParam(str, 10, profiles[i].ltabFont.lfItalic);
  else if (str.compare(b, e - b, L"TabSize") == 0)
    profiles[i].tabSize = getIntParam(str, 10, profiles[i].tabSize);
  else if (str.compare(b, e - b, L"MainColor") == 0)
    profiles[i].MainColor = getIntParam(str, 16, profiles[i].MainColor);
  else if (str.compare(b, e - b, L"ToolbarColor") == 0)
    profiles[i].ToolbarColor = getIntParam(str, 16, profiles[i].ToolbarColor);
  else if (str.compare(b, e - b, L"EditorColor") == 0)
    profiles[i].EditorColor = getIntParam(str, 16, profiles[i].EditorColor);
  else if (str.compare(b, e - b, L"EditorTextColor") == 0)
    profiles[i].EditorTextColor = getIntParam(str, 16, profiles[i].EditorTextColor);
  else if (str.compare(b, e - b, L"EditorCaretTextColor") == 0)
    profiles[i].EditorCaretTextColor = getIntParam(str, 16, profiles[i].EditorCaretTextColor);
  else if (str.compare(b, e - b, L"EditorCaretBgColor") == 0)
    profiles[i].EditorCaretBgColor = getIntParam(str, 16, profiles[i].EditorCaretBgColor);
  else if (str.compare(b, e - b, L"EditorSelColor") == 0)
    profiles[i].EditorSelColor = getIntParam(str, 16, profiles[i].EditorSelColor);
  else if (str.compare(b, e - b, L"EditorSelTextColor") == 0)
    profiles[i].EditorSelTextColor = getIntParam(str, 16, profiles[i].EditorSelTextColor);
  else if (str.compare(b, e - b, L"NotFoundEditControlColor") == 0)
    profiles[i].NotFoundEditControlColor = getIntParam(str, 16, profiles[i].NotFoundEditControlColor);
  else if (str.compare(b, e - b, L"TabBGColor") == 0)
    profiles[i].TabColors.BGColor = getIntParam(str, 16, profiles[i].TabColors.BGColor);
  else if (str.compare(b, e - b, L"TabTextColor") == 0)
    profiles[i].TabColors.textColor = getIntParam(str, 16, profiles[i].TabColors.textColor);
  else if (str.compare(b, e - b, L"TabActTextColor") == 0)
    profiles[i].TabColors.activeTextColor = getIntParam(str, 16, profiles[i].TabColors.activeTextColor);
  else if (str.compare(b, e - b, L"TabPBColor") == 0)
    profiles[i].TabColors.progressBarColor = getIntParam(str, 16, profiles[i].TabColors.progressBarColor);
  else if (str.compare(b, e - b, L"TabFrameColor") == 0)
    profiles[i].TabColors.frameColor = getIntParam(str, 16, profiles[i].TabColors.frameColor);
  else if (str.compare(b, e - b, L"TabActFrameColor") == 0)
    profiles[i].TabColors.activeFrameColor = getIntParam(str, 16, profiles[i].TabColors.activeFrameColor);
  else if (str.compare(b, e - b, L"TabColor") == 0)
    profiles[i].TabColors.normColor = getIntParam(str, 16, profiles[i].TabColors.normColor);
  else if (str.compare(b, e - b, L"TabModColor") == 0)
    profiles[i].TabColors.modColor = getIntParam(str, 16, profiles[i].TabColors.modColor);
  else if (str.compare(b, e - b, L"TabActColor") == 0)
    profiles[i].TabColors.activeNormColor = getIntParam(str, 16, profiles[i].TabColors.activeNormColor);
  else if (str.compare(b, e - b, L"TabActModColor") == 0)
    profiles[i].TabColors.activeModColor = getIntParam(str, 16, profiles[i].TabColors.activeModColor);
  else if (str.compare(b, e - b, L"TabBusyColor") == 0)
    profiles[i].TabColors.busyColor = getIntParam(str, 16, profiles[i].TabColors.busyColor);
  else if (str.compare(b, e - b, L"TabActiveBusyColor") == 0)
    profiles[i].TabColors.activeBusyColor = getIntParam(str, 16, profiles[i].TabColors.activeBusyColor);
  else if (str.compare(b, e - b, L"SelectionBackgroundTransparent") == 0)
    profiles[i].selectionBackgroundTransparent = getIntParam(str, 10, 1) != 0;
  else if (str.compare(b, e - b, L"SelectionTextTransparent") == 0)
    profiles[i].selectionTextTransparent = getIntParam(str, 10, 0) != 0;
  else if (str.compare(b, e - b, L"TabWindowHeight") == 0)
    profiles[i].tabWindowHeight = getIntParam(str, 10, profiles[i].tabWindowHeight);
}

void COptions::setAppearanceParam(std::wstring &str)
{
  size_t b = str.find_first_not_of(L"\t ", 0, 2);
  size_t e = str.find_first_of(L"\t =", b, 3);

  if (str.compare(b, e - b, L"IsToolbar") == 0)
    isToolbar = getIntParam(str, 10, isToolbar ? 1 : 0) ? true : false;
  else if (str.compare(b, e - b, L"IsToolbarTransparent") == 0)
    isToolbarTransparent = getIntParam(str, 10, isToolbarTransparent ? 1 : 0) ? true : false;
  else if (str.compare(b, e - b, L"IsStatusBar") == 0)
    isStatusBar = getIntParam(str, 10, isStatusBar ? 1 : 0) ? true : false;
  else if (str.compare(b, e - b, L"CaretWidth") == 0)
    caretWidth = getIntParam(str, 10, caretWidth);
  else if (str.compare(b, e - b, L"LeftIndent") == 0)
    leftIndent = getIntParam(str, 10, leftIndent);
  else if (str.compare(b, e - b, L"DocMenuPos") == 0)
    docMenuPos = (char)getIntParam(str, 10, docMenuPos);
  else if (str.compare(b, e - b, L"QinsertMenuPos") == 0)
    qinsertMenuPos = (char)getIntParam(str, 10, qinsertMenuPos);
  else if (str.compare(b, e - b, L"PluginsMenuPos") == 0)
    pluginsMenuPos = (char)getIntParam(str, 10, pluginsMenuPos);
  else if (str.compare(b, e - b, L"PermanentScrollBars") == 0)
    permanentScrollBars = getIntParam(str, 10, permanentScrollBars ? 1 : 0) ? true : false;
  else if (str.compare(b, e - b, L"MinForVisibleTabs") == 0)
    minForVisibleTabs = getIntParam(str, 10, minForVisibleTabs);
  else if (str.compare(b, e - b, L"FullScreen") == 0)
    isFullScreen = getIntParam(str, 10, isFullScreen ? 1 : 0) ? true : false;
  else if (str.compare(b, e - b, L"SIPOnFindBar") == 0)
    sipOnFindBar = getIntParam(str, 10, sipOnFindBar ? 1 : 0) ? true : false;
}

void COptions::setMiscParam(std::wstring &str)
{
  size_t b = str.find_first_not_of(L"\t ", 0, 2);
  size_t e = str.find_first_of(L"\t =", b, 3);

  if (str.compare(b, e - b, L"DialogsPath") == 0)
    getStringParam(str, dialogPath, true);
}

bool COptions::ReadOptions(const wchar_t *fileName)
{
  std::wstring path = g_execPath;
  path += FILE_SEPARATOR;
  path += fileName;
  std::wifstream fIn(path.c_str(), std::ios_base::binary);

  if (fIn.is_open())
  {
    int section;
    wchar_t bom;
    std::wstring str;

#ifndef _WIN32_WCE
    fIn.imbue(std::locale(fIn.getloc(), new ucs2_conversion));
#endif

    bom = fIn.get();
    if (bom != 0xfeff) 
    {
      fIn.close();
      CError::SetError(NOTEPAD_CORRUPTED_OPTIONS_FILE);
      return false;
    }

    currentSection = SECTION_SKIP;
    while ((section = readString(fIn, str)) != END_OF_FILE)
    {
      switch (section)
      {
      case SECTION_COMMON:
        setCommonParam(str);
        break;

      case SECTION_PROFILE:
        setProfileParam(str);
        break;

      case SECTION_APPEARANCE:
        setAppearanceParam(str);
        break;

      case SECTION_MISC:
        setMiscParam(str);
        break;
      }
    }
    fIn.close();
  }

  return true;
}

bool COptions::InitOptions()
{
  bool retval = true;

  for (unsigned i = 0; i < profiles.size(); i++)
  {
    if (0 == profiles[i].ltabFont.lfHeight)
      profiles[i].ltabFont.lfHeight = -MulDiv(8, g_LogPixelsY, 72);

    profiles[i].htabFont = CreateFontIndirect(&profiles[i].ltabFont);
    if (!profiles[i].htabFont)
    {
      profiles[i].htabFont = (HFONT)GetStockObject(SYSTEM_FONT);
      CError::SetError(NOTEPAD_CREATE_RESOURCE_ERROR);
      retval = false;
    }


    if (0 == profiles[i].leditorFont.lfHeight)
      profiles[i].leditorFont.lfHeight = -MulDiv(8, g_LogPixelsY, 72);

    profiles[i].heditorFont = CreateFontIndirect(&profiles[i].leditorFont);
    if (!profiles[i].heditorFont)
    {
      profiles[i].heditorFont = (HFONT)GetStockObject(SYSTEM_FONT);
      CError::SetError(NOTEPAD_CREATE_RESOURCE_ERROR);
      retval = false;
    }
  }

  return retval;
}

bool COptions::SaveOptions()
{
  std::wstring path;
  FILE *fOut;

  if (optionsChanged)
  {
    path = g_execPath;
    path += FILE_SEPARATOR;
    path += L"properties.ini";
    fOut = _wfopen(path.c_str(), L"wb");
    if (!fOut)
    {
      CError::SetError(NOTEPAD_CANNOT_SAVE_OPTIONS);
      return false;
    }

    CNotepad &Notepad = CNotepad::Instance();

    // write BOM
    Notepad.Documents.CodingManager.GetEncoderById(CCodingManager::CODING_UTF_16_LE_ID)->WriteBOM(fOut);

    // save common options
    fwprintf(fOut, L"[COMMON]\n");
    fwprintf(fOut, L"SaveSession = %i\n", saveSession ? 1 : 0);
    fwprintf(fOut, L"DefaultWordWrap = %i\n", defaultWordWrap ? 1 : 0);
    fwprintf(fOut, L"DefaultSmartInput = %i\n", defaultSmartInput ? 1 : 0);
    fwprintf(fOut, L"DefaultDragMode = %i\n", defaultDragMode ? 1 : 0);
    fwprintf(fOut, L"DefaultScrollByLine = %i\n", defaultScrollByLine ? 1 : 0);
    fwprintf(fOut, L"ResponseOnSIP = %i\n", responseOnSIP ? 1 : 0);
    fwprintf(fOut, L"LangLib = %s\n", langLib.c_str());

    // save appearance options
    fwprintf(fOut, L"\n[APPEARANCE]\n");
    fwprintf(fOut, L"IsToolbar = %i\n", isToolbar ? 1 : 0);
    fwprintf(fOut, L"IsStatusBar = %i\n", isStatusBar ? 1 : 0);
    fwprintf(fOut, L"PermanentScrollBars = %i\n", permanentScrollBars ? 1 : 0);
    fwprintf(fOut, L"LeftIndent = %i\n", leftIndent);
    fwprintf(fOut, L"FullScreen = %i\n", isFullScreen ? 1 : 0);

    // save profiles options
    fwprintf(fOut, L"\n[PROFILES]\n");
    fwprintf(fOut, L"EditorFontName = %s\n", profiles[currentProfile].leditorFont.lfFaceName);
    fwprintf(fOut, L"EditorFontHeight = %i\n", -MulDiv(profiles[currentProfile].leditorFont.lfHeight, 72, g_LogPixelsY));
    fwprintf(fOut, L"EditorFontWeight = %i\n", profiles[currentProfile].leditorFont.lfWeight);
    fwprintf(fOut, L"TabFontName = %s\n", profiles[currentProfile].ltabFont.lfFaceName);
    fwprintf(fOut, L"TabFontHeight = %i\n", -MulDiv(profiles[currentProfile].ltabFont.lfHeight, 72, g_LogPixelsY));
    fwprintf(fOut, L"TabFontWeight = %i\n", profiles[currentProfile].ltabFont.lfWeight);
    fwprintf(fOut, L"TabSize = %i\n", profiles[currentProfile].tabSize);

    fclose(fOut);
  }

  CNotepad &Notepad = CNotepad::Instance();
  if (wcscmp(Notepad.Interface.Dialogs.GetDialogsPath(), dialogPath.c_str()) != 0)
  {
    path = g_execPath;
    path += FILE_SEPARATOR;
    path += L"paths.ini";

    fOut = _wfopen(path.c_str(), L"wb");
    if (!fOut)
    {
      CError::SetError(NOTEPAD_CANNOT_SAVE_OPTIONS);
      return false;
    }

    Notepad.Documents.CodingManager.GetEncoderById(CCodingManager::CODING_UTF_16_LE_ID)->WriteBOM(fOut);

    fwprintf(fOut, L"[MISC]\n");
    fwprintf(fOut, L"DialogsPath = %s\n", Notepad.Interface.Dialogs.GetDialogsPath());

    fclose(fOut);
  }


  return true;
}

void COptions::FinalizeOptions()
{
  HFONT hsysFont = (HFONT)GetStockObject(SYSTEM_FONT);  // default font
  for (unsigned i = 0; i < profiles.size(); i++)
  {
    if (profiles[i].htabFont && hsysFont != profiles[i].htabFont)
      DeleteObject(hsysFont);
  }
}


void COptions::SetRegexp(bool useRegexp)
{
  if (useRegexp != this->useRegexp)
  {
    this->useRegexp = useRegexp;

    CNotepad &Notepad = CNotepad::Instance();
    if (Notepad.Documents.GetDocsNumber() > 0)
    {
      Notepad.Documents.SetRegexp(Notepad.Documents.Current());
      Notepad.Documents.SetFlags(DOC_SETREGEXP, false);
    }
  }
}

void COptions::SetWrapType(char wrapType)
{
  if (wrapType != this->wrapType)
  {
    optionsChanged = true;
    this->wrapType = wrapType;

    CNotepad &Notepad = CNotepad::Instance();
    if (Notepad.Documents.GetDocsNumber() > 0)
    {
      Notepad.Documents.SetWrapType(Notepad.Documents.Current());
      Notepad.Documents.SetFlags(DOC_SETWRAPTYPE, false);
    }
  }
}

void COptions::SetPermanentScrollBars(bool permanent)
{
  if (permanent != this->permanentScrollBars)
  {
    optionsChanged = true;
    this->permanentScrollBars = permanent;

    CNotepad &Notepad = CNotepad::Instance();
    if (Notepad.Documents.GetDocsNumber() > 0)
    {
      Notepad.Documents.SetPermanentScrollBars(Notepad.Documents.Current());
      Notepad.Documents.SetFlags(DOC_SETSCROLLBARS, false);
    }
  }
}

void COptions::SetHideSpaceSyms(bool hide)
{
  if (hide != this->hideSpaceSyms)
  {
    this->hideSpaceSyms = hide;

    CNotepad &Notepad = CNotepad::Instance();
    if (Notepad.Documents.GetDocsNumber() > 0)
    {
      Notepad.Documents.SetHideSpaces(Notepad.Documents.Current());
      Notepad.Documents.SetFlags(DOC_SETHIDESPACES, false);
    }
  }
}

void COptions::SetFullScreen(bool fullScreen)
{
  if (this->isFullScreen != fullScreen)
  {
    this->isFullScreen = fullScreen;
    optionsChanged = true;

#ifdef UNDER_CE
    CNotepad &Notepad = CNotepad::Instance();
    Notepad.Interface.SwitchFullScreen(fullScreen);
#endif
  }
}

void COptions::SetSaveSession(bool save)
{
  if (this->saveSession != save)
  {
    this->saveSession = save;
    optionsChanged = true;
  }
}

void COptions::SetDefaultDragMode(bool isDragMode)
{
  if (this->defaultDragMode != isDragMode)
  {
    this->defaultDragMode = isDragMode;
    optionsChanged = true;
  }
}

void COptions::SetDefaultScrollByLine(bool isScrollByLine)
{
  if (this->defaultScrollByLine = isScrollByLine)
  {
    this->defaultScrollByLine = isScrollByLine;
    optionsChanged = true;
  }
}

void COptions::SetDefaultSmartInput(bool isSmartInput)
{
  if (this->defaultSmartInput != isSmartInput)
  {
    this->defaultSmartInput = isSmartInput;
    optionsChanged = true;
  }
}

void COptions::SetDefaultWordWrap(bool isWordWrap)
{
  if (this->defaultWordWrap != isWordWrap)
  {
    this->defaultWordWrap = isWordWrap;
    optionsChanged = true;
  }
}

void COptions::SetResponseOnSIP(bool isResponse)
{
  if (this->responseOnSIP != isResponse)
  {
    this->responseOnSIP = isResponse;
    optionsChanged = true;
  }  
}

void COptions::SetLangLib(const wchar_t *pathToLib)
{
  if (langLib.compare(pathToLib) != 0)
  {
    langLib = pathToLib;
    optionsChanged = true;
  }  
}

void COptions::SetEditorFont(const LOGFONT *newFont)
{
  if (areLogFontsEqual(&profiles[currentProfile].leditorFont, newFont))
    return;

  HFONT hfont = CreateFontIndirect(newFont);
  CNotepad &Notepad = CNotepad::Instance();

  if (hfont)
  {
    memcpy(&profiles[currentProfile].leditorFont, newFont, sizeof(LOGFONT));

    bool isBusy = false;
    int number = Notepad.Documents.GetDocsNumber();
    for (int i = 0; i < number; i++)
    {
      if (Notepad.Documents.IsBusy(i))
      {
        Notepad.Documents.WaitAll(NPD_DELETEFONTS);
        isBusy = true;
        break;
      }
    }

    if (!isBusy)
      DeleteObject(profiles[currentProfile].heditorFont);
    else
      holdFonts.push_back(profiles[currentProfile].heditorFont);

    profiles[currentProfile].heditorFont = hfont;
    Notepad.Documents.SetDocumentsFont(hfont);

    optionsChanged = true;
  }
  else
  {
    std::wstring message;
    CError::GetErrorMessage(message, NOTEPAD_CREATE_RESOURCE_ERROR);
    Notepad.Interface.Dialogs.ShowError(Notepad.Interface.GetHWND(), message);
  }
}

void COptions::SetTabsFont(const LOGFONT *newFont)
{
  if (areLogFontsEqual(&profiles[currentProfile].ltabFont, newFont))
    return;

  HFONT hfont = CreateFontIndirect(newFont);
  CNotepad &Notepad = CNotepad::Instance();

  if (hfont)
  {
    memcpy(&profiles[currentProfile].ltabFont, newFont, sizeof(LOGFONT));

    Notepad.Interface.Tabs.SetFont(hfont);
    DeleteObject(profiles[currentProfile].htabFont);
    profiles[currentProfile].htabFont = hfont;
    Notepad.Interface.Resize(Notepad.Interface.GetWidth(), Notepad.Interface.GetHeight());

    optionsChanged = true;
  }
  else
  {
    std::wstring message;
    CError::GetErrorMessage(message, NOTEPAD_CREATE_RESOURCE_ERROR);
    Notepad.Interface.Dialogs.ShowError(Notepad.Interface.GetHWND(), message);
  }
}

void COptions::SetTabulationSize(int tabSize)
{
  if (profiles[currentProfile].tabSize == tabSize)
    return;

  CNotepad &Notepad = CNotepad::Instance();
  profiles[currentProfile].tabSize = tabSize;
  Notepad.Documents.SetDocumentsTabSize(tabSize);

  optionsChanged = true;
}

void COptions::SetLeftIndent(int leftIndent)
{
  if (this->leftIndent == leftIndent)
    return;

  CNotepad &Notepad = CNotepad::Instance();
  this->leftIndent = leftIndent;
  Notepad.Documents.SetDocumentsLeftIndent(leftIndent);

  optionsChanged = true;
}

void COptions::SetToolbar(bool isToolbar)
{
  if (isToolbar == this->isToolbar)
    return;

  optionsChanged = true;
  this->isToolbar = isToolbar;
  CNotepad &Notepad = CNotepad::Instance();

  if (isToolbar)
  {
#ifdef _WIN32_WCE
    Notepad.Interface.Controlbar.DestroyMenuBar(Notepad.Interface.GetHWND());
#endif

    if (!Notepad.Interface.Controlbar.CreateToolbar(Notepad.Interface.GetHWND()))
    {
      Notepad.Interface.Dialogs.ShowError(Notepad.Interface.GetHWND(), 
        CError::GetError().message);
      CError::SetError(NOTEPAD_NO_ERROR);

#ifdef _WIN32_WCE
      // try to recreate menubar
      if (!Notepad.Interface.Controlbar.CreateMainMenu(Notepad.Interface.GetHWND()))
      {
        Notepad.Interface.Dialogs.ShowError(Notepad.Interface.GetHWND(), 
          CError::GetError().message);
        CError::SetError(NOTEPAD_NO_ERROR);
        Notepad.OnCmdExit(); // this is a critical error, we should save documents and exit
      }
#endif
    }
  }
  else if (!isToolbar)
  {
    Notepad.Interface.Controlbar.DestroyToolbar();

#ifdef _WIN32_WCE
    if (!Notepad.Interface.Controlbar.CreateMainMenu(Notepad.Interface.GetHWND()))
    {
      Notepad.Interface.Dialogs.ShowError(Notepad.Interface.GetHWND(), 
        CError::GetError().message);
      CError::SetError(NOTEPAD_NO_ERROR);
      Notepad.OnCmdExit(); // this is a critical error, we should save documents and exit
    }
#endif
  }
}

void COptions::SetStatusBar(bool isStatusBar)
{
  if (isStatusBar == this->isStatusBar)
    return;

  optionsChanged = true;
  this->isStatusBar = isStatusBar;
  CNotepad &Notepad = CNotepad::Instance();

  if (isStatusBar)
  {
    if (!Notepad.Interface.StatusBar.Create(Notepad.Interface.GetHWND()))
    {
      Notepad.Interface.Dialogs.ShowError(Notepad.Interface.GetHWND(), 
        CError::GetError().message);
      CError::SetError(NOTEPAD_NO_ERROR);
    }
    else
      Notepad.Interface.Resize(Notepad.Interface.GetWidth(), Notepad.Interface.GetHeight());
  }
  else
  {
    Notepad.Interface.StatusBar.Destroy();
    Notepad.Interface.Resize(Notepad.Interface.GetWidth(), Notepad.Interface.GetHeight());
  }
}


// Function deletes all old fonts. The can be accumulated because when we change font, some documents
// may be busy, so we can't delete old font
void COptions::DeleteOldFonts()
{
  for (size_t i = 0; i < holdFonts.size(); i++)
    DeleteObject(holdFonts[i]);
  holdFonts.resize(0);
}