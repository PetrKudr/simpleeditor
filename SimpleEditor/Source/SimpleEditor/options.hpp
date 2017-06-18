#ifndef __OPTIONS_
#define __OPTIONS_

#include "dialogs.hpp"
#include "tabs.hpp"
#include "base\register.h"
#include <windows.h>
#include <string>
#include <vector>
#include <fstream>

const int FILTER_SIZE = 512;


struct SProfile
{
  std::wstring name;

  CTabColors TabColors;
  COLORREF EditorSelColor;
  COLORREF EditorSelTextColor;
  COLORREF EditorTextColor;
  COLORREF EditorCaretTextColor;
  COLORREF EditorCaretBgColor;
  COLORREF EditorColor;
  COLORREF MainColor;
  COLORREF ToolbarColor;
  COLORREF NotFoundEditControlColor;

  HFONT htabFont;
  HFONT heditorFont;
  LOGFONT leditorFont;
  LOGFONT ltabFont;

  bool selectionBackgroundTransparent;
  bool selectionTextTransparent;

  int tabWindowHeight;

  int tabSize;
};

class COptions
{
  int currentSection;     // current section
  int currentProfile;     // current profile
  bool optionsChanged;
  std::vector<HFONT> holdFonts;

  std::vector<SProfile> profiles;
  bool bugWithPopupMenusExists;
  bool isToolbarTransparent;
  bool saveRecentImmediately;
  bool needBOM;
  bool createEmptyIfNoDocuments;
  bool switchToExistingOnOpen;
  bool isToolbar;
  bool isStatusBar;
  bool useRegexp;
  bool hideSpaceSyms;
  bool permanentScrollBars;  
  bool caretRealPos;
  bool kineticScroll;
  bool defaultWordWrap;
  bool defaultSmartInput;
  bool defaultDragMode;
  bool defaultScrollByLine;
  bool ownDialogs;
  bool responseOnSIP;
  bool saveSession;
  bool useFastReadMode;
  bool isFullScreen;
  bool sipOnFindBar;
  std::wstring defaultCP;
  int caretWidth;
  int leftIndent;
  int openFileFilter;
  int saveFileFilter;
  int minForVisibleTabs; // Minimum number of tabs to make them visible
  int maxRecent;
  char wrapType;     // SEDIT_WORDWRAP/SEDIT_SYMBOLWRAP
  char okButton;
  int docMenuPos;
  int qinsertMenuPos;
  int pluginsMenuPos;
  std::wstring langLib;
  std::wstring dialogPath;
  std::wstring breakSyms;
  wchar_t typeStr[FILTER_SIZE];
  size_t typeStrSize;
  size_t undoHistorySize;


private:

  int readString(std::wifstream &fIn, std::wstring &str);
  void setCommonParam(std::wstring &str);
  void setProfileParam(std::wstring &str);
  void setAppearanceParam(std::wstring &str);
  void setMiscParam(std::wstring & str);
  int getIntParam(std::wstring &str, int radix, int byDefault);
  bool getStringParam(std::wstring &str, std::wstring &param, bool skipSpaces);
  bool getStringParam(std::wstring &str, wchar_t *param, size_t len, bool skipSpaces);

  void setDefaultFont(LOGFONT &lf);
  bool areLogFontsEqual(const LOGFONT *plf1, const LOGFONT *plf2);


public:

  bool IsToolbarTransparent()            { return isToolbarTransparent;     }
  bool BugWithPopupMenusExists()         { return bugWithPopupMenusExists;  }
  bool SaveRecentImmediately()           { return saveRecentImmediately;    }
  bool NeedBOM()                         { return needBOM;                  }
  bool CreateEmptyIfNoDocuments()        { return createEmptyIfNoDocuments; }
  bool SwitchToExistingOnOpen()          { return switchToExistingOnOpen;   }
  bool IsToolbar()                       { return isToolbar;                }
  bool IsStatusBar()                     { return isStatusBar;              }
  bool UseRegexp()                       { return useRegexp;                }
  bool HideSpaceSyms()                   { return hideSpaceSyms;            }
  bool PermanentScrollBars()             { return permanentScrollBars;      }
  bool CaretRealPos()                    { return caretRealPos;             }
  bool KineticScroll()                   { return kineticScroll;            }
  bool DefaultWordWrap()                 { return defaultWordWrap;          }
  bool DefaultSmartInput()               { return defaultSmartInput;        }
  bool DefaultDragMode()                 { return defaultDragMode;          }
  bool DefaultScrollByLine()             { return defaultScrollByLine;      }
  bool OwnDialogs()                      { return ownDialogs;               }
  bool ResponseOnSIP()                   { return responseOnSIP;            }
  bool SaveSession()                     { return saveSession;              }
  bool UseFastReadMode()                 { return useFastReadMode;          }
  bool IsFullScreen()                    { return isFullScreen;             }
  bool IsSipOnFindBar()                  { return sipOnFindBar;             }
  std::wstring& DefaultCP()              { return defaultCP;                }
  int CaretWidth()                       { return caretWidth;               }
  int LeftIndent()                       { return leftIndent;               }
  int OpenFileFilter()                   { return openFileFilter;           }
  int SaveFileFilter()                   { return saveFileFilter;           }
  int MinForVisibleTabs()                { return minForVisibleTabs;        }
  int MaxRecent()                        { return maxRecent;                }
  char WrapType()                        { return wrapType;                 }
  char OkButton()                        { return okButton;                 }
  int DocMenuPos()                       { return docMenuPos;               }
  int QinsertMenuPos()                   { return qinsertMenuPos;           }
  int PluginsMenuPos()                   { return pluginsMenuPos;           }
  std::wstring& BreakSyms()              { return breakSyms;                }
  const wchar_t* TypeStr()               { return typeStr;                  }
  std::wstring& DialogPath()             { return dialogPath;               }
  std::wstring& LangLib()                { return langLib;                  }
  size_t TypeStrSize()                   { return typeStrSize;              }
  size_t UndoHistorySize()               { return undoHistorySize;          }
  
  bool ReadOptions(const wchar_t *fileName);
  bool SaveOptions(); 

  bool InitOptions();
  void FinalizeOptions();

  int NumberOfProfiles()       { return profiles.size();   }
  bool SetProfile(int i);
  const SProfile& GetProfile() { return profiles[currentProfile]; }

  void SetRegexp(bool useRegexp);
  void SetWrapType(char wrapType);
  void SetPermanentScrollBars(bool permanent);
  void SetHideSpaceSyms(bool hide);
  void SetEditorFont(const LOGFONT *newFont);
  void SetTabsFont(const LOGFONT *newFont);
  void SetTabulationSize(int tabSize);
  void SetLeftIndent(int leftIndent);
  void SetFullScreen(bool fullScreen);
  void SetSaveSession(bool save);
  void SetDefaultDragMode(bool isDragMode);
  void SetDefaultScrollByLine(bool isScrollByLine);
  void SetDefaultSmartInput(bool isSmartInput);
  void SetDefaultWordWrap(bool isWordWrap);
  void SetResponseOnSIP(bool isResopnse);
  void SetLangLib(const wchar_t *pathToLib);

  void SetToolbar(bool isToolbar);
  void SetStatusBar(bool isStatusBar);


  void DeleteOldFonts();

  COptions();
};


#endif /* __OPTIONS_ */