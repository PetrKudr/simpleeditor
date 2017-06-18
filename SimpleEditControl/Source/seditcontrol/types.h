#ifndef __DATA_TYPES_
#define __DATA_TYPES_

#pragma warning(disable: 4996)
#include <stdlib.h>
#include "seditcontrol.h"
#include "stringmas.h"
#include "manager.h"

#if !defined(_WIN32_WCE)
#include <windows.h>
#else
#include "..\stdgui.h"
#endif

#define UNDO_DATA_RESERVED 4 // max(sizeof(unsigned int), sizeof(wchar_t*))
#define REGEXP_OPAQUE 8

/*
#define IDM_POPUP_CUT                   110
#define IDM_POPUP_COPY                  111
#define IDM_POPUP_PASTE                 112
#define IDM_POPUP_DELETE                113
#define IDM_POPUP_SELECT_ALL            114
*/

#define POS_BEGIN 0  // позиция в начале строки
#define POS_END 1  // позиция в конце строки

// размеры буфера для чтения из файла по умолчанию
#define STRING_SURPLUS 24 // количество дополнительных байт выделенных под строку

// максимальная длина строки при переносе слов
#define WRAP_MAX_LENGTH(pcommon) ((pcommon)->clientX - (pcommon)->di.leftIndent) 
   
#define REDUCE 0            // параметр функции, указывающий что строки надо урезать
#define ENLARGE 1           // параметр функции, указывающий что строки надо расширять

// значения поля структуры common_info_t SBFlags
#define PERMANENT_SCROLLBARS 1 // скроллбары всегда есть
#define UPDATE_SCROLLBARS 2    // обновить скроллбары (даже если параметры не изменились)
#define RESET_SCROLLBARS 4     // сначала включает скроллбар, затем устанавливает правильные значения

#define VSCROLL_MAX_RANGE 20000 // максимальный диапазон ползунка по Y
#define HSCROLL_MAX_RANGE 20000 // максимальный диапазон ползунка по X

// состояния каретки
#define CARET_EXIST 1              // создана
#define CARET_VISIBLE 2            // видима
#define CARET_PARTIALLY_VISIBLE 4  // частично видима
#define CARET_IS_MODIFIED 8        // при любом изменении положения каретки этот бит устанавливается (для поиска)

// для перемещения каретки
#define CARET_RIGHT                      0
#define CARET_LEFT                       1
#define CARET_RIGHT_WITHOUT_MODIFICATION 2
#define CARET_LEFT_WITHOUT_MODIFICATION  3

// параметр position у функции MoveCaretToPosition
#define POS_WORDBEGIN       0
#define POS_WORDEND         1
#define POS_STRINGBEGIN     2
#define POS_STRINGEND       3
#define POS_REALLINEBEGIN   4
#define POS_REALLINEEND     5

// значения lButtonDown структуры common_info_t
#define MOUSE_SINGLE_CLICK 1
#define MOUSE_DOUBLE_CLICK 2

#define WHEEL_CHANGE 3    
#define AUTO_SCROLL 2     // количество позиций для прокручивания, если мышка за пределами окна

#define MAX_TAB_SIZE 32
#define MIN_TAB_SIZE 1

#define MOUSE_HISTORY_SIZE    64
#define MOUSE_EVENT_ROW_TIME  50   // время в милисекундах, когда события от мыши считаются связанными

#define MM_UPDATE_PARAMS (WM_USER + 1)
//#define MM_DOUBLECLICK   (WM_USER + 2)
//#define MM_TRIPLECLICK   (WM_USER + 3)

// Значения wParam для сообщения MM_UPDATE_PARAMS
#define UPDATE_NOTHING      0  
#define UPDATE_REPAINT      1
#define UPDATE_CARET        2
#define UPDATE_TEXT         4
#define UPDATE_BITMAPS      8

// Константы для переменной action структуры common_info_t
#define ACTION_TURN_ON 1
#define ACTION_IN_PROGRESS 2 // действие в процессе выполнения.(Чтобы при выполнении действия, процедуры не назначали новое)
#define ACTION_UPCASE 4
#define ACTION_DOUBLE_UPCASE 8
#define ACTION_ADD_STRING 16
#define ACTION_BACKSPACE 32
#define ACTION_DOUBLE_BACKSPACE 64
#define ACTION_MOVE_CARET_LEFT 128

// Спецальные символы (в строке выводятся отдельно)
#define SPEC_SYMBOLS_NUMBER 2
#define SPACE 0
#define TABULATION 1

// Количество областей для сохранения
#define INVALID_RECTANGLES_HISTORY_SIZE 4

// если символ является пробельным символом, то выражение равно единице
#define isSpaceSym(x) ((char)((x) == L' ' || (x) == L'\t'))  

// если символ является символом возврата каретки или перехода на следующую строку,
// то выражение равно единице
#define isSpecialSymW(x) ((char)((x) == L'\n' || (x) == L'\r' || (x) == L'\0'))
#define isCRLFSym(x) ((char)((x) == '\n' || (x) == '\r'))

#define isFormatSym(x) ((char)((x) == L'\t'))
#define GETFORMATSYMWIDTH(pdi, x) (((x) == L'\t') ? (pdi)->specSymWidths[TABULATION] : (pdi)->specSymWidths[SPACE])

// макросы для записи нижнего и верхнего слова в переменную DWORD
#define LW(x) (WORD)x               // нижнее слово переменной типа DWORD
#define HW(x) *((WORD*)(&(x)) + 1)  // верхнее слово переменной типа DWORD

#define SESIGN(x) ((x) > 0 ? 1 : ((x) < 0 ? -1 : 0))


// структура для хранения "настоящей" строки
typedef struct tag_real_line_t
{
  wchar_t *str;                 // строка
  unsigned int surplus;         // сколько лишних байт отведено под эту строку
  struct tag_real_line_t *next; // указатель на след. строку
} real_line_t, *preal_line_t;

// структура для хранения отображаемой строки (сверстанная или нет)
typedef struct tag_line_t
{
  preal_line_t prealLine;   // указатель на строку исходного документа
  unsigned int begin, end;  // индексы сверстанной строки в ней

  struct tag_line_t *next;  // следующая отображаемая строка
  struct tag_line_t *prev;  // предыдущая отображаемая строка
} line_t, *pline_t;


// для структуры line_point_t
#define FORMAT_SYM          1      // символ является символом форматирования
#define SELECTION_BEGIN     2      // позиция является началом выделения
#define SELECTION_END       3      // позиция является концом выделения
#define COLOR_CHANGE        4      // начало вывода другим цветом

// информация о позиции в строке
typedef struct tag_line_point_t
{
  unsigned int i;         // индекс в строке
  unsigned char data[4]; 
} line_point_t;


// информация о параметрах обработки и отображения текста
typedef struct tag_display_info_t
{
  char isDrawingRestricted;          // запрет вывода на экран  

  RECT invalidRectangles[INVALID_RECTANGLES_HISTORY_SIZE];
  char numberOfInvalidRectangles;
  int scrollX;  
  int scrollY;
  //RECT invalidatedRect;        // тут накапливается область для перерисовки
  //char isInvalidatedRectEmpty; // содержит или нет прямоугольник для обновления поле invalidatedRect

  char isKineticScrollTurnedOn;   // нужна ли поддержка кинетического скролла
  char kineticScrollActivated;   
  float xKineticScrollVelocity;   // пиксели в милисекунду
  float yKineticScrollVelocity;   // пиксели в милисекунду

  unsigned int tabSize;
  unsigned int caretWidth;
  unsigned short specSymWidths[SPEC_SYMBOLS_NUMBER]; // ширины специальных символов
  char hideSpaceSyms;  // 0 - пробельные символы не прятать, 1 - прятать
  char wordWrapType;   // SEDIT_WORDWRAP / SEDIT_SYMBOLWRAP -  пословный/посимвольный перенос
  char selectionTransparency; // текст под выделением остаётся прежнего цвета

  COLORREF selectionColor;    // цвет выделения
  COLORREF selectedTextColor; // цвет выделенного текста
  COLORREF caretTextColor;    // цвет текста в строке с кареткой
  COLORREF caretBgColor;      // цвет фона строки с кареткой
  COLORREF textColor;         // цвет текста
  COLORREF bgColor;           // цвет фона

  long int leftIndent;          // отступ слева
  long int averageCharX;        // средняя ширина символа
  long int averageCharY;        // средняя высота символа
  long int maxOverhang;         // максимальный выступ за пределы глифа
  char isItalic;                // является ли шрифт курсивным

  long int bmWidth;           // ширина битмапа
  long int bmHeight;          // высота битмапа
  HBITMAP hlineBitmap;        // буфер для вывода строк
  HDC hlineDC;                // контекст устройства для вывода строк в буфер

  get_colors_function_t GetColorsForString;   // функция для получения цвета текста
  get_colors_function_t GetBkColorsForString; // функция получения цвета фона текста

  create_popup_menu_function_t CreatePopupMenuFunc;    // creates popup menu
  destroy_popup_menu_function_t DestroyPopupMenuFunc;  // destroys popup menu

  size_t dxSize;
  int *dx;    // массив типа int для работы со строками (используют разные процедуры, обрабатывающие текст)
} display_info_t;

// общая информация об окне
typedef struct tag_common_info_t
{
  HWND hwnd;                    // дескриптор окна
  HFONT hfont;                  // шрифт
  unsigned int id;              // идентификатор редактора
  long clientX;                 // размер окна по X
  long clientY;                 // по Y
  unsigned long nscreenLines;   // количество строк на экране
  unsigned long nLines;         // общее количество отображаемых строк
  unsigned long nRealLines;     // общее количество реальных строк
  unsigned long maxStringSizeX; // максимальная длина строки в документе (в пикселях)
  char isVScrollByLine;         // тип прокрутки построчно (по умолчанию попиксельно)
  char isWordWrap;              // включена ли функция переноса слов
  char isModified;              // изменялся ли файл с последнего сохранения
  char useRegexp;               // use regular expressions in Find and Replace actions
  unsigned char action;         // переменная для действия "умного" ввода.
  wchar_t actString[5];         // строка для "умного" ввода
  int vscrollPos;               // VSCROLL_INCREMENT * vscrollPos пикселей от начала текста до позиции экрана по вертикали
  unsigned short vscrollCoef;   // VSCROLL_INCREMENT * vscrollCoef пикселей в одной позиции ползунка по вертикали
  int vscrollRange;             // область допустимых значений вертикального ползунка 
  int hscrollPos;               // HSCROLL_INCREMENT * hscrollPos пикселей от начала текста до позиции экрана по горизонтали
  unsigned short hscrollCoef;   // HSCROLL_INCREMENT * hscrollCoef пикселей в одной позиции ползунка по вертикали
  int hscrollRange;             // область допустимых позиций по Х 
  char SBFlags;                 // флаги для скроллбаров; значения UPDATE_SCROLLBARS и PERMANENT_SCROLLBARS
  char isDragMode;              // режим прокрутки мышкой
  char forValidateRect;         // для разрешения вызова ValidateRect при изменении размеров
  
  size_t linePointsSize;
  line_point_t *linePoints;     // массив специальных позиций в строке(для вывода на экран)

  size_t bkLinePointsSize;
  line_point_t *bkLinePoints;   // массив специальных позиций в фоне строки строки

  receive_message_function_t ReceiveMessageFunc;       // receives messages

  display_info_t di;            // информация о параметрах отображения
} common_info_t;

// информация о сверстанной строке
typedef struct tag_line_info_t
{
  pline_t pline;        // сверстанная строка
  unsigned int number;  // номер строки в списке
  unsigned int realNumber; // номер строки в списке реальных строк
} line_info_t;

// информация о каретке
typedef struct tag_caret_info_t
{
  line_info_t line;
  unsigned int index;  // индекс в исходной строке 
  long realSizeX;      // расстояние в пикселях от начала строки до каретки перед перемещением её вверх/вниз
  long sizeX;          // текущее расстояние в пикселях от начала строки до каретки
  long posX;           // координата по X
  long posY;           // координата по Y

  unsigned int length; // длина исходной строки
  unsigned int dxSize; // размер dx
  int *dx;             // массив расстояний между ячейками

  char type; // состояние каретки: создана/не создана, отображается/не отображается, размер строки актуальный/неактуальный
} caret_info_t;

typedef struct tag_mouse_point_t 
{
  short int x;            // X координата
  short int y;            // Y координата 
  unsigned long int time; // время в милисекундах, когда сообщение пришло
} mouse_point_t;

typedef struct tag_mouse_info_t
{
  char lButtonDown;  // флаг нажатия левой кнопки мыши. Значения: MOUSE_SINGLE_CLICK или MOUSE_DOUBLE_CLICK

  mouse_point_t history[MOUSE_HISTORY_SIZE]; 
  int currentPoint;
  int historySize;

} mouse_info_t;

// информация о позиции в строке
typedef struct tag_pos_info_t 
{
  line_info_t *piLine;  // указатель на указатель на структуру с информацией о строке
  unsigned int index;   // индекс в строке
  char pos;             // тип позиции: POS_BEGIN, POS_END
} pos_info_t;

// для использования регулярных выражений
typedef struct tag_sedit_regexp_t
{
  char opaque[REGEXP_OPAQUE];
  int nSubExpr;
  position_t *subExpr;
} sedit_regexp_t;

// позиция выделения
typedef struct tag_selection_pos_t
{
  line_info_t startLine, endLine;     // информация о граничных линиях
  unsigned int startIndex, endIndex;  // индексы границ
} selection_pos_t;

// информация о выделении
typedef struct tag_selection_t
{
  selection_pos_t pos;       // позиция выделения
  char isSelection;          // флаг существования выделения
  //char shiftPressed;         // нажат ли шифт

  selection_pos_t oldPos;    // предыдущее выделение
  char isOldSelection;       // флаг существования старого выделения
} selection_t, *pselection_t;

typedef struct tag_text_t
{
  line_t *pfakeHead;
  real_line_t *prealHead;
  line_info_t textFirstLine;
} text_t;


#define ACTION_NOT_INITIALIZED  0
#define ACTION_SYMBOL_INPUT     1
#define ACTION_SYMBOL_DELETE    2
#define ACTION_TEXT_PASTE       3
#define ACTION_TEXT_DELETE      4
#define ACTION_NEWLINE_ADD      5
#define ACTION_NEWLINE_DELETE   6
#define ACTION_REPLACE_ALL      7

#define UNDO_DATA_NOT_INITIALIZED 0
#define UNDO_DATA_SYMBOL          1
#define UNDO_DATA_AREA            2
#define UNDO_DATA_TEXT            3
#define UNDO_DATA_REPLACE_LIST    4

#define UNDO_SINGLE_ACTION      0
#define UNDO_GROUP_BEGIN        1
#define UNDO_GROUP_MEMBER       2
#define UNDO_GROUP_END          3
#define UNDO_AUTOGROUP_BEGIN    4
#define UNDO_AUTOGROUP_MEMBER   5
#define UNDO_AUTOGROUP_END      6

typedef struct tag_undo_rlist_position_t 
{
  unsigned int x, y;
  wchar_t *str;
} undo_rlist_position_t;

typedef struct tag_replace_list_t
{
  wchar_t *Replace;
  wchar_t *Find;
  undo_rlist_position_t *positions;
  string_massive_t *founded;    // сохраняет ещё и длины строк, что не нужно для обычного режима поиска

  unsigned int number;  // сколько позиций сейчас есть
  unsigned int size;    // под сколько позиций выделено памяти

} replace_list_t;

typedef struct tag_undo_data_symbol_t
{
  position_t pos;
  wchar_t sym;
} undo_data_symbol_t;

typedef struct tag_undo_data_area_t
{
  position_t startPos;
  position_t endPos;
} undo_data_area_t;

typedef struct tag_undo_data_text_t
{
  position_t pos;
  wchar_t *text;
} undo_data_text_t;

typedef struct tag_undo_t
{
  char actionType;
  char actionModifier;
  char dataType;
  void *data;

  struct tag_undo_t *next,
                    *prev;
} undo_t;


typedef struct tag_undo_info_t
{
  undo_t *head,
         *current,
         *tail,
         *unmodAction; // действие со сброшенным флагом модификации (а-ля "документ не изменён")

  memory_system_t memory;

  char isGrouping;   // действия будут группироваться вплоть до вызова EndGrouping
  char isSaveAllowed;
  char undoAvailable;
} undo_info_t;



#define LOCKED_BY_APPEND   1   // окно заблокировано сообщением SE_BEGINAPPEND
#define LOCKED_BY_RECEIVE  2   // окно заблокировано сообщением SE_BEGINRECEIVE


typedef struct tag_window_t
{
  common_info_t common;
  caret_info_t caret;
  mouse_info_t mouse;
  text_t lists;
  selection_t selection;
  undo_info_t undo;
  line_info_t firstLine;
  line_info_t searchLine;
  pos_info_t searchPos;

  char isLocked;       // флаг для блокировки всех сообщений, кроме некоторых(зависит от значения)
  void *reserved;      // зарезервированный указатель. 
} window_t;

#endif /* __DATA_TYPES_ */

