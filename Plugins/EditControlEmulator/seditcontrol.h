#ifndef _CROSSPAD_ENGINE_
#define _CROSSPAD_ENGINE_

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SEDIT_CLASS L"SimpleEditControl"

#define SEAPIENTRY __stdcall

/****************************************************************************************************/
/*                                 Messages                                                         */
/****************************************************************************************************/

/** 
 * First editor message. Doesn't do anything.
 */
#define SE_FIRST           (WM_USER + 9)

/** 
 * Clears document.
 */
#define SE_CLEARTEXT       (WM_USER + 10)

/** 
 * Sets new id to the editor.
 * Params: 
 *   wParam - id of the window
 */
#define SE_SETID           (WM_USER + 11)

/** 
 * Turns on/off scrolling by line/pixel.
 * Params: 
 *   wParam - 1 means on, 0 means off.
 */
#define SE_SETSCROLLBYLINE (WM_USER + 12)

/** 
 * Turns on/off word wrap
 * Params: 
 *   wParam - 1 means on, 0 means off.
 */
#define SE_SETWORDWRAP     (WM_USER + 13)

/** 
 * Turns on/off smart input
 * Params: 
 *   wParam - 1 means on, 0 means off.
 */
#define SE_SETSMARTINPUT   (WM_USER + 14)

/** 
 * Turns on/off drag mode
 * Params: 
 *   wParam - 1 means on, 0 means off.
 */
#define SE_SETDRAGMODE     (WM_USER + 15)

/** 
 * Sets modification flag
 * Params: 
 *   wParam - 1 means document is modified, 0 means not modified.
 */
#define SE_SETMODIFY       (WM_USER + 16) // wParam - 0/1

/** 
 * Sets new positions to the editor (Caret and view positions)
 * Params: 
 *   wParam - pointer to doc_info_t
 */
#define SE_SETPOSITIONS    (WM_USER + 17)

/** 
 * Sets different options.
 * Params: 
 *   wParam - pointer to options_t
 */
#define SE_SETOPTIONS      (WM_USER + 18)

/** 
 * Sets undo size.
 * Params: 
 *   wParam - size of undo history in bytes
 */
#define SE_SETUNDOSIZE     (WM_USER + 19)

/** 
 * Launches find/replace operation.
 * Params: 
 *   wParam - pointer to FINDREPLACE.
 *   lParam - in case of 'Replace All' can be pointer to the integer, which would represent number of replaces. Can be NULL.
 * Returns:
     1 if match found, 0 if not found, < 0 in case of error
 */
#define SE_FINDREPLACE     (WM_USER + 20)

/** 
 * Copy selection to the clipboard.
 */
#define SE_COPY            (WM_USER + 21)

/** 
 * Cut selection to the clipboard.
 */
#define SE_CUT             (WM_USER + 22)

/** 
 * Paste text from clipboard to the editor.
 */
#define SE_PASTE           (WM_USER + 23)

/** 
 * Delete selection or char after caret.
 */
#define SE_DELETE          (WM_USER + 24)

/** 
 * Selects all text.
 */
#define SE_SELECTALL       (WM_USER + 25)

/** 
 * Undo last operation
 */
#define SE_UNDO            (WM_USER + 26)

/** 
 * Redo last undo operation
 */
#define SE_REDO            (WM_USER + 27)

/** 
 * Returns:
 *   1 if editor is empty, 0 otherwise
 */
#define SE_ISEMPTY         (WM_USER + 28)

/** 
 * Returns:
 *   1 if scrolling by line turned on, 0 otherwise
 */
#define SE_ISSCROLLBYLINE  (WM_USER + 29)

/** 
 * Returns:
 *   1 if word wrap turned on, 0 otherwise
 */
#define SE_ISWORDWRAP      (WM_USER + 30)

/** 
 * Returns:
 *   1 if smart input turned on, 0 otherwise
 */
#define SE_ISSMARTINPUT    (WM_USER + 31)

/** 
 * Returns:
 *   1 if drag mode turned on, 0 otherwise
 */
#define SE_ISDRAGMODE      (WM_USER + 32)

/** 
 * Returns:
 *   1 if modification flag is set, 0 otherwise
 */
#define SE_ISMODIFIED      (WM_USER + 33)

/** 
 * Returns:
 *   1 if some text is selected, 0 otherwise
 */
#define SE_ISSELECTION     (WM_USER + 34)

/** 
 * Returns:
 *   1 if undo operation is available, 0 otherwise
 */
#define SE_CANUNDO          (WM_USER + 35)

/** 
 * Returns:
 *   1 if redo operation is available, 0 otherwise
 */
#define SE_CANREDO          (WM_USER + 36)

/** 
 * Returns:
 *   1 if caret is visible, 0 otherwise
 */
#define SE_ISCARETVISIBLE  (WM_USER + 37)

/** 
 * Retrieves information about document.
 * Params: 
 *   wParam - pointer to the structure doc_info_t
 * Returns:
 *   information about document into the passed structure
 */
#define SE_GETDOCINFO      (WM_USER + 38)

/** 
 * Retrieves options.
 * Params: 
 *   wParam - pointer to the structure options_t
 * Returns:
 *   options into the passed structure
 */
#define SE_GETOPTIONS      (WM_USER + 39)

/** 
 * Params: 
 *   wParam - line number
 *   lParam - SEDIT_VISUAL_LINE or SEDIT_REALLINE
 * Returns:
 *   length of the line
 */
#define SE_GETLINELENGTH   (WM_USER + 40)

/** 
 * Retrieves block with text
 * Params: 
 *   wParam - pointer to text_block_t
 *   lParam - SEDIT_VISUAL_LINE or SEDIT_REALLINE
 * Returns:
 *   <0 in case of error, 0 on success, >0 size of buffer for text, if passed too small.
 */
#define SE_GETTEXT         (WM_USER + 41)

/** 
 * Retrieves selected text
 * Params: 
 *   wParam - pointer to buffer of wchars (wchar_t *)
 *   lParam - size of buffer in wchars
 * Returns:
 *   <0 in case of error, 0 on success, >0 size of buffer for text, if passed too small.
 */
#define SE_GETSELECTEDTEXT (WM_USER + 42)

/** 
 * Paste text from buffer to the caret position
 * Params: 
 *   wParam - pointer to buffer of wchars (wchar_t *)
 *   lParam - 0 to clear selection, 1 to delete selection
 */
#define SE_PASTEFROMBUFFER (WM_USER + 43)

/** 
 * Moves caret to right/left.
 * Params: 
 *   wParam - number of characters.
 *   lParam - 1 to move right, 0 to move left
 */
#define SE_MOVECARET       (WM_USER + 44)

/** 
 * Moves view to the caret
 */
#define SE_GOTOCARET       (WM_USER + 45)

/** 
 * Repaints client area and updates scrollbars
 */
#define SE_REPAINT         (WM_USER + 46)

/** 
 * Repaints range in editor
 * Params:
 *   LOWORD(wParam) - SEDIT_VISUAL_LINE or SEDIT_REALLINE
 *   HIWORD(wParam) - 1 to immediate update, 0 to lazy update
 *   lParam - pointer to range_t
 */
#define SE_UPDATERANGE     (WM_USER + 47)

/** 
 * Allows to repaint only new part of the screen after window resizing. Kind of hack to avoid blinking on pocket pc when SIP pops up/down.
 * Params:
 *   wParam - 1 to allow, 0 to restrict
 */
#define SE_VALIDATE        (WM_USER + 48)

/** 
 * Sets selection.
 * Params:
 *   wParam - pointer to range_t
 *   lParam - SEDIT_VISUAL_LINE or SEDIT_REALLINE
 */
#define SE_SETSEL          (WM_USER + 49)

/** 
 * Gets selection.
 * Params:
 *   wParam - pointer to range_t
 *   lParam - SEDIT_VISUAL_LINE or SEDIT_REALLINE
 * Returns:
 *   Selection range in passed range_t
 */
#define SE_GETSEL          (WM_USER + 50)

/** 
 * Message to start appending editor's content
 */
#define SE_BEGINAPPEND     (WM_USER + 51)

/** 
 * Adds new line to the end of editor
 * Params:
 *   wParam - pointer to the buffer of wchars (wchar *)
 *   lParam - SEDIT_APPEND or SEDIT_REPLACE
 */
#define SE_APPEND          (WM_USER + 52)

/** 
 * Message to end appending editor's content
 */
#define SE_ENDAPPEND       (WM_USER + 53) 

/** 
 * Message to start retrieving editor's content
 */
#define SE_BEGINRECEIVE    (WM_USER + 54)

/** 
 * Message to get next line
 * Params:
 *   wParam - pointer to the buffer of wchars (wchar_t *)
 *   lParam - size of buffer in wchars
 * Returns:
 *   <0 in case if error, 0 on success, >0 - size of line if passed buffer too small
 */
#define SE_GETNEXTLINE     (WM_USER + 55)

/** 
 * Message to end retrieving editor's content
 */
#define SE_ENDRECEIVE      (WM_USER + 56)

/** 
 * Returns:
 *   id of the editor
 */
#define SE_GETID           (WM_USER + 57)

/** 
 * Starts grouping operations in undo history
 */
#define SE_STARTUNDOGROUP  (WM_USER + 58) 

/** 
 * Ends grouping operations in undo history
 */
#define SE_ENDUNDOGROUP    (WM_USER + 59)

/** 
 * Sets the function for coloring text
 * Params:
 *   lParam - pointer to get_colors_function_t. Could be NULL
 */
#define SE_SETCOLORSFUNC   (WM_USER + 60) 

/** 
 * Sets the function for coloring background
 * Params:
 *   lParam - pointer to get_colors_function_t. Could be NULL
 */
#define SE_SETBKCOLORSFUNC (WM_USER + 61)

/** 
 * Returns pointer to the function for coloring text
 */
#define SE_GETCOLORSFUNC   (WM_USER + 62) 

/** 
 * Returns pointer to the function for coloring background
 */
#define SE_GETBKCOLORSFUNC (WM_USER + 63)

/** 
 * Sets functions for creating and destroying pop-up menu
 * Params:
 *   lParam - pointer to an array {create_popup_menu_function_t, destroy_popup_menu_function_t}. Could be NULL
 */
#define SE_SETPOPUPMENUFUNCS   (WM_USER + 64) 

/** 
 * Gets functions for creating and destroying pop-up menu
 * Params:
 *   lParam - pointer to an array (void* arr[2]). arr[0] will be create_popup_menu_function_t and arr[1] - destroy_popup_menu_function_t
 */
#define SE_GETPOPUPMENUFUNCS (WM_USER + 65)

/** 
 * Sets function which will be called for every message
 * Params:
 *   lParam - receive_message_function_t
 */
#define SE_SETRECEIVEMSGFUNC (WM_USER + 66)

/** 
 * Gets function which is called for every message
 */
#define SE_GETRECEIVEMSGFUNC (WM_USER + 67)

/** 
 * Returns position from coordinates
 * Params:
 *   LOWORD(wParam) - x coordinate
 *   HIWORD(wParam) - y coordinate
 *   lParam - pointer to position_t (for result)
 * Returns error code
 */
#define SE_GETPOSFROMCOORDS  (WM_USER + 68)

/** 
 * Starts visual group. All operations after this message and before SE_ENDVISUALGROUP will be shown as one operation
 */
#define SE_RESTRICTDRAWING  (WM_USER + 69)

/** 
 * Ends visual group.
 */
#define SE_ALLOWDRAWING  (WM_USER + 70)

/** 
 * Sets position from search will start if caret remains unmodified
 * Params:
 *   wParam - SEDIT_VISUAL_LINE or SEDIT_REALLINE
 *   lParam - pointer to position_t
 */
#define SE_SETSEARCHPOS  (WM_USER + 71)

/** 
 * Gets position from offset. Only 'real' coordinates are supported.
 * Params:
 *   wParam - offset
 *   lParam - pointer to position_t
 */
#define SE_GETPOSFROMOFFSET  (WM_USER + 72)

/** 
 * Gets offset from position. Only 'real' coordinates are supported.
 * Params:
 *   wParam - pointer to const position_t
 *   lParam - pointer to unsigned integer
 */
#define SE_GETOFFSETFROMPOS  (WM_USER + 73)

/** 
 * Internal message. Useless in release
 */
#define SE_DEBUG           (WM_USER + 99)

/** 
 * Last message. Doesn't do anything
 */
#define SE_LAST            (WM_USER + 100)

/****************************************************************************************************/
/****************************************************************************************************/

// Result on success
#define SEDIT_NO_ERROR                   0

// Special errors (typically, they are system)
#define SEDIT_MEMORY_ERROR              -1
#define SEDIT_INITIALIZE_ERROR          -2
#define SEDIT_COPY_ERROR                -3
#define SEDIT_SEARCH_ERROR              -4
#define SEDIT_CREATE_BRUSH_ERROR        -5
#define SEDIT_CREATE_PEN_ERROR          -6
#define SEDIT_REGISTER_CLASS_ERROR      -7
#define SEDIT_APPEND_NOT_INITIALIZED    -8  
#define SEDIT_RECEIVE_NOT_INITIALIZED   -9
#define SEDIT_WINDOW_INFO_UNAVAILABLE  -10
#define SEDIT_BREAKSYMS_LIMIT_REACHED  -11
#define SEDIT_WINDOW_IS_BLOCKED        -12
#define SEDIT_WRONG_REPLACE_PATTERN    -13
#define SEDIT_INITIALIZE_UNDO_ERROR    -14

// Common errors
#define SEDIT_NO_PARAM -20
#define SEDIT_INVALID_PARAM -21
#define SEDIT_TEXT_END_REACHED -22
#define SEDIT_NO_SELECTION -23


// Length of string with delimiters.
#define SEDIT_BREAKSYMSTR_SIZE 256


// Constants which represents lines (SEDIT_VISUAL_LINE) and paragraphs (SEDIT_REALLINE).
#define SEDIT_VISUAL_LINE        0
#define SEDIT_REALLINE    1


// Values for lParam for message SE_APPEND
#define SEDIT_APPEND  0  // append new line
#define SEDIT_REPLACE 1  // replace last line with the new one.


// Values for wrapType field in options_t
#define SEDIT_WORDWRAP 0
#define SEDIT_SYMBOLWRAP 1

// Values for selectionTransparency field in options_t 
#define SEDIT_SELBGTRANSPARENT   1
#define SEDIT_SELTEXTTRANSPARENT 2

// Values of lineBreakType field in options_t
#define SEDIT_VISUAL_LINEBREAK_CRLF  0
#define SEDIT_VISUAL_LINEBREAK_CR    1
#define SEDIT_VISUAL_LINEBREAK_LF    2

// Options
#define SEDIT_SETFONT 1                     // hfont
#define SEDIT_SETTAB 2                      // tabSize
#define SEDIT_SETCARETWIDTH 4               // caretWidth
#define SEDIT_SETLEFTINDENT 8               // leftIndent
#define SEDIT_SETSELCOLOR 16                // selectionColor
#define SEDIT_SETSELTEXTCOLOR 32            // selectedTextColor
#define SEDIT_SETBGCOLOR 64                 // bgColor
#define SEDIT_SETTEXTCOLOR 128              // textColor
#define SEDIT_SETCARETTEXTCOLOR 256         // caretTextColor
#define SEDIT_SETCARETBGCOLOR 512           // caretBgColor
#define SEDIT_SETSELTEXTTRANSPARENCY 1024   // selectionTransparency
#define SEDIT_SETWRAPTYPE 2048              // wrapType
#define SEDIT_SETHIDESPACES 4096            // hideSpaceSyms
#define SEDIT_SETSCROLLBARS 8192            // permanentScrollBars
#define SEDIT_SETUSEREGEXP 16384            // useRegexp
#define SEDIT_SETKINETICSCROLL 32768        // kinetic scroll
#define SEDIT_SETLINEBREAK 65536            // lineBreakType
#define SEDIT_OPTIONSALL (SEDIT_SETLINEBREAK * 2 - 1)

typedef struct tag_options_t
{
  unsigned int type;

  HFONT hfont;
  unsigned int tabSize;
  unsigned int caretWidth;  
  long int leftIndent;

  COLORREF selectionColor;
  COLORREF selectedTextColor;
  COLORREF bgColor;
  COLORREF textColor;
  COLORREF caretTextColor;
  COLORREF caretBgColor;

  char selectionTransparency;       // combination of SEDIT_SELBGTRANSPARENT and SEDIT_SELTEXTTRANSPARENT
  char wrapType;                    // SEDIT_WORDWRAP/SEDIT_SYMBOLWRAP
  char hideSpaceSyms;               // 0/1   
  char permanentScrollBars;         // 0/1
  char useRegexp;                   // 0/1
  char lineBreakType;               // SEDIT_VISUAL_LINEBREAK_CRLF / SEDIT_VISUAL_LINEBREAK_CR / SEDIT_VISUAL_LINEBREAK_LF
  char kineticScroll;               // 0/1
} options_t;


// Position in text
typedef struct tag_position_t
{
  unsigned int x, y;
} position_t;

// Position in text in pixels
typedef struct tag_pix_position_t
{
  long int x, y;
} pix_position_t;

// Range in text 
typedef struct tag_range_t
{
  position_t start;
  position_t end;
} range_t;

// Block with text 
typedef struct tag_text_block_t 
{
  wchar_t *text;
  size_t size;
  range_t position;
} text_block_t;


// Information about document
#define SEDIT_GETWINDOWPOS      1   // affects on windowPos;
#define SEDIT_GETCARETPOS       2   // affects on caretPos (excludes SEDIT_GETCARETREALPOS)
#define SEDIT_GETCARETREALPOS   4   // affects on caretPos (excludes SEDIT_GETCARETPOS)

#define SEDIT_SETWINDOWPOS     1  // moves view to the given position
#define SEDIT_SETCARETPOS      2  // moves caret to the given position (caretPos, excludes SEDIT_GETCARETREALPOS)
#define SEDIT_SETCARETREALPOS  4  // moves caret to the given position (caretPos, excludes SEDIT_SETCARETPOS)

typedef struct tag_doc_info_t
{
  unsigned int flags;

  unsigned int nLines;
  unsigned int nRealLines;
  position_t caretPos;
  position_t windowPos;
} doc_info_t;


// Values for field code in structure NMHDR on message WM_NOTIFY
#define SEN_CARETMODIFIED     1
#define SEN_FOCUSCHANGED      2
#define SEN_DOCCHANGED        3
#define SEN_DOCUPDATED        4
#define SEN_DOCCLOSED         5


// Notification about caret modification
typedef struct tag_sen_caret_modified_t
{
  NMHDR header;
  position_t realPos;          // position in coordinates of paragraphs
  position_t screenPos;        // position in coordinates of lines
} sen_caret_modified_t;


// Notification about focus change
typedef struct tag_sen_focus_changed_t
{
  NMHDR header;
  char focus;           
} sen_focus_changed_t;


// Values of action field in sen_doc_changed_t
#define SEDIT_ACTION_NONE        0
#define SEDIT_ACTION_INPUTCHAR   1
#define SEDIT_ACTION_DELETECHAR  2
#define SEDIT_ACTION_PASTE       3
#define SEDIT_ACTION_DELETEAREA  4
#define SEDIT_ACTION_NEWLINE     5
#define SEDIT_ACTION_DELETELINE  6
#define SEDIT_ACTION_REPLACEALL  7

// Notification about changes in text (sends before screen update)
typedef struct tag_sen_doc_changed_t
{
  NMHDR header;
  unsigned int action;    
  range_t range;           // affected range (in paragraph's coordinates)
} sen_doc_changed_t;


// Notification about changes in text or just changes of modification flag. (sends after screen update)
typedef struct tag_sen_doc_updated_t
{
  NMHDR header;
  unsigned int action;    
  range_t range;           // affected range (in paragraph's coordinates)
  char isDocModified;      // modification flag
} sen_doc_updated_t;

// Notification about closed document (it means that all text has been deleted)
typedef struct tag_sen_doc_closed_t
{
  NMHDR header;
} sen_doc_closed_t;


extern char SEAPIENTRY SEAPIRegister(HINSTANCE hInstance, COLORREF bgColor);
extern char SEAPIENTRY SEAPIAddBreakSyms(const wchar_t *str, unsigned short length);

/** 
 * Function for coloring.
 * Params:
 *   hwndFrom - window of the editor
 *   idFrom - id of the editor
 *   str - the whole line (paragraph)
 *   begin - index of the start of the range for coloring
 *   end - index of the end of the range for coloring
 *   realNumber - number of line (paragraph) in editor
 */
typedef const COLORREF* (SEAPIENTRY *get_colors_function_t)(HWND hwndFrom,
                                                            unsigned int idFrom,
                                                            const wchar_t *str, 
                                                            unsigned int begin, 
                                                            unsigned int end,
                                                            unsigned int realNumber);

/** 
 * Creates pop-up menu.
 * Params:
 *   hwndFrom - window of the editor
 *   idFrom - id of the editor
 */
typedef HMENU (SEAPIENTRY *create_popup_menu_function_t)(HWND hwndFrom,
                                                         unsigned int idFrom);

/** 
 * Destroys pop-up menu.
 * Params:
 *   hwndFrom - window of the editor
 *   idFrom - id of the editor
 *   hMenu - menu which was returned by create_popup_menu_function_t.
 */
typedef void (SEAPIENTRY *destroy_popup_menu_function_t)(HWND hwndFrom,
                                                         unsigned int idFrom,
                                                         HMENU hMenu);

/** 
 * Function which receives notifications about messages.
 * Params:
 *   hwndFrom - window of the editor
 *   idFrom - id of the editor
 *   message - message
 *   wParam - wParam
 *   lParam - lParam
 *   presult - if message has to be suppressed, it's the result which window function will return.
 *
 * Returns: 1 to suppress message, 0 otherwise
 */
typedef char (SEAPIENTRY *receive_message_function_t)(__in HWND hwndFrom,
                                                      __in unsigned int idFrom,
                                                      __in unsigned int message,
                                                      __in WPARAM wParam,
                                                      __in LPARAM lParam,
                                                      __out LRESULT *presult);

#ifdef __cplusplus
}
#endif

#endif