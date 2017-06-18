#ifndef __RESOURCES_
#define __RESOURCES_

#define RESOURCES_VERSION   L"1.0.4"

#define CMD_NEWDOC          101
#define CMD_CLOSEDOC        102
#define CMD_OPENDOC         103
#define CMD_SAVEDOC         104
#define CMD_SAVEASDOC       105
#define CMD_UNDO            106
#define CMD_REDO            107
#define CMD_EXIT            108
#define CMD_SAVEALL         109
#define CMD_CLOSEALL        110
#define CMD_COPY            111
#define CMD_CUT             112
#define CMD_PASTE           113
#define CMD_DELETE          114
#define CMD_SELECTALL       115
#define CMD_WORDWRAP        116
#define CMD_SMARTINPUT      117
#define CMD_DRAGMODE        118
#define CMD_SCROLLBYLINE    119
#define CMD_REOPEN_AUTO     120
#define CMD_CLEARRECENT     127
#define CMD_GOTOBEGIN       128
#define CMD_GOTOEND         129
#define CMD_FULLSCREEN      130
#define CMD_SHOWDOCMENU     131
#define CMD_SHOWPLUGINSMENU 132

//#define CMD_NEXTDOC         106
//#define CMD_TEST 127

#define IDM_MAIN_MENU          200
#define IDM_FILE               201
#define IDM_MENU               202
#define IDS_FILE               203
#define IDS_MENU               204
#define IDM_FIND               205
#define IDM_REPLACE            206
#define IDM_ABOUT              207
#define IDM_QINSERT_M          208  // qinsert is called from menu
#define IDM_QINSERT_T          209  // qinsert is called from toolbar
#define IDS_OK                 210
#define IDS_CANCEL             211
#define IDM_OKCANCEL_MENU      212
#define IDM_OPTIONS            213
#define IDM_GOTOLINE           214
#define IDM_DOCPOPUP_MENU      215
#define IDM_EDITORPOPUP_MENU   216
#define IDM_PLUGINSPOPUP_MENU  217
#define IDM_TABPOPUP_MENU      218
#define IDM_FILEINFO           219


// identifiers
#define IDD_TEXT_0          300
#define IDD_TEXT_1          301
#define IDD_TEXT_2          302
#define IDD_TEXT_3          303
#define IDD_TEXT_4          304
#define IDD_TEXT_5          305
#define IDD_TEXT_6          306
#define IDD_TEXT_7          307
#define IDD_TEXT_8          308
#define IDD_BTN_0           310
#define IDD_BTN_1           311
#define IDD_BTN_2           312
#define IDD_BTN_3           313
#define IDD_BTN_4           314
#define IDD_EDIT_0          320
#define IDD_EDIT_1          321
#define IDD_EDIT_2          322
#define IDD_COMBOBOX_0      330
#define IDD_COMBOBOX_1      331
#define IDD_COMBOBOX_2      332
#define IDD_LIST            335
#define IDD_CHECKBOX_0      340
#define IDD_CHECKBOX_1      341
#define IDD_CHECKBOX_2      342
#define IDD_CHECKBOX_3      343
#define IDD_CHECKBOX_4      344
#define IDD_CHECKBOX_5      345
#define IDD_CHECKBOX_6      346
#define IDD_CHECKBOX_7      347
#define IDD_CHECKBOX_8      348
#define IDD_CHECKBOX_9      349

#define IDS_ERROR_CAPTION       101
#define IDS_WARNING_CAPTION     102
#define IDS_INFO_CAPTION        103
#define IDS_DOC_FILE_NOT_SAVED  104
#define IDS_DOC_REPLACED        105
#define IDS_DOC_TEXT_NOT_FOUND  106
#define IDS_NOTEPAD_FORCE_EXIT  107
#define IDS_GOTOLINE_MESSAGE    108
#define IDS_LANGUAGE_NAME       109
#define IDS_MENU_EMPTY          110
#define IDS_LANGUAGE_ID         111
#define IDS_DOC_REWRITE_FILE    112
#define IDS_VERSION             113


// ABOUT Dialog identifiers
#define IDD_PROGRAM_NAME     IDD_TEXT_0
#define IDD_AUTHOR           IDD_TEXT_1
#define IDD_EMAIL            IDD_TEXT_2
#define IDD_DONATE_MSG       IDD_TEXT_3
#define IDD_DONATE_YM        IDD_TEXT_4
#define IDD_DONATE_WMR       IDD_TEXT_5
#define IDD_DONATE_WMZ       IDD_TEXT_6
#define IDD_REGEXP_COPYRIGHT IDD_TEXT_7

// File info Dialog identifiers
#define IDD_PATHTOFILE_TITLE IDD_TEXT_0
#define IDD_PATHTOFILE_FIELD IDD_EDIT_0
#define IDD_FILESIZE_TITLE   IDD_TEXT_1
#define IDD_FILESIZE_FIELD   IDD_TEXT_2

// Get file dialog identifiers
#define IDD_PATH             IDD_EDIT_0
#define IDD_PATH_MSG         IDD_TEXT_0
#define IDD_FILTER           IDD_COMBOBOX_0
#define IDD_CODING           IDD_COMBOBOX_1
#define IDD_DIRECTORIES      IDD_COMBOBOX_2
#define IDD_NAME             IDD_EDIT_1
#define IDD_BOM              IDD_CHECKBOX_0

// Options->Common dialog identifiers
#define IDD_SAVE_SESSION       IDD_CHECKBOX_0
#define IDD_DEF_WORDWRAP       IDD_CHECKBOX_1
#define IDD_DEF_SMARTINPUT     IDD_CHECKBOX_2
#define IDD_DEF_DRAGMODE       IDD_CHECKBOX_3
#define IDD_DEF_SCROLLBYLINE   IDD_CHECKBOX_4
#define IDD_RESPONSE_ON_SIP    IDD_CHECKBOX_5

// Options->Font dialog identifiers
#define IDD_EDITOR_FONT        IDD_COMBOBOX_0
#define IDD_TAB_FONT           IDD_COMBOBOX_1
#define IDD_EDITOR_SIZE        IDD_EDIT_0
#define IDD_TAB_SIZE           IDD_EDIT_1
#define IDD_TABULATION         IDD_EDIT_2

// Options->view dialog identifiers
#define IDD_STATUSBAR         IDD_CHECKBOX_0
#define IDD_TOOLBAR           IDD_CHECKBOX_1
#define IDD_PERMSCROLLBARS    IDD_CHECKBOX_2
#define IDD_INDENT_TEXT       IDD_TEXT_0
#define IDD_INDENT            IDD_EDIT_0

// Options->language dialog identifiers
#define IDD_LANGUAGE          IDD_COMBOBOX_0
#define IDD_RESTART_WARNING   IDD_TEXT_0

// id of the first document
#define IDM_FIRST_DOCUMENT  400

// if of the first recent (MAX_RECENT == 50)
#define IDM_FIRST_RECENT    440


#define NOTEPAD_ACCELLERATORS L"Accelerators"
#define APPLICATION_ICON_0 100
#define APPLICATION_ICON_1 101

// extensions for associations dialog
#define EXT_TXT   100 
#define EXT_INI   101
#define EXT_C     102
#define EXT_CPP   103
#define EXT_H     104
#define EXT_HPP   105
#define EXT_HTM   106
#define EXT_HTML  107
#define EXT_PHP   108
#define EXT_ASM   109
#define EXT_PAS   110


// templates
#define DLG_FINDBAR              500
#define DLG_REPLACEBAR           501
#define DLG_MSGBOX_OK            502
#define DLG_MSGBOX_YESNO         503
#define DLG_MSGBOX_YESNOCANCEL   504
#define DLG_ABOUT                505
#define DLG_GETFILE              506
#define DLG_GETCODING            507
#define DLG_COMMON_PAGE          508
#define DLG_ASSOCIATIONS_PAGE    509
#define DLG_APPEARANCE_PAGE      510
#define DLG_FONT_PAGE            511
#define DLG_MISC_PAGE            512
#define DLG_GOTOLINE             513
#define DLG_LANGUAGE_PAGE        514
#define DLG_FILEINFO             515

#define IDB_GETFILE16 1
#define IDB_GETFILE32 2

// plugin's commands
#define IDM_FIRST_PLUGIN_COMMAND 1000

// plugin's codings
#define IDM_FIRST_PLUGIN_CODING  1400

// quick insert identifiers
#define QINSERT_FIRST_ID         1600

#endif /*__RESOURCES_*/
