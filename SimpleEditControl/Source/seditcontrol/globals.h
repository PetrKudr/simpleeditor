#ifndef __GLOBALS_
#define __GLOBALS_

#include "seditcontrol.h"

#define DEFAULT_CAPTION L"SimpleEdit"
#define SEDIT_ERROR_CAPTION L"SimpleEdit error"
#define SEDIT_WARNING_CAPTION L"Warning"
#define CRLF_SIZE 2            // размер подстроки CRLF
#define CRLF_SEQUENCE L"\r\n"  // подстрока CRLF

// значение опций по умолчанию
#define BACKGROUND_COLOR    RGB(255, 255, 255)
#define TEXT_COLOR          RGB(0, 0, 0)
#define SELECTION_COLOR     RGB(0, 50, 255)
#define SELECTED_TEXT_COLOR RGB(255, 255, 255)
#define TAB_SIZE 2
#define CARET_WIDTH 1
#define DEFAULT_LEFTINDENT    0  // отступ слева
#define DEFAULT_WORDWRAP      0  // перенос слов
#define DEFAULT_WORDWRAP_TYPE SEDIT_WORDWRAP // тип переноса слов
#define DEFAULT_SMARTINPUT    0  // дружественный ввод (ACTION_TURN_ON для включения)
#define UNDO_HISTORY_SIZE     0  // без undo. Устанавливается извне сообщением SE_SETUNDOSIZE

// для переменной forValidateRect
#define SEDIT_NOT_REPAINTED 0
#define SEDIT_REPAINTED 1
#define SEDIT_RESTRICT_VALIDATE 2

#define SEDIT_ALLOCATE_MEMORY_ERRMSG L"Memory allocation error!"
#define SEDIT_CREATE_BRUSH_ERRMSG L"Cannot create the brush!"
#define SEDIT_CREATE_PEN_ERRMSG L"Cannot create the pen!"

#define MAX_LENGTH 256000      // максимальная длина отображаемой строки в пикселях
#define MAX_CHARACTERS 65536   // максимальное количество символов в отображаемой строке

#define DEFAULT_VSCROLL_COEF 4 // начальное значение vscrollCoef
#define DEFAULT_HSCROLL_COEF 4 // начальное значение hscrollCoef
#define VSCROLL_LINE_UPDOWN  0.5 // коэффициент для изменения позиции по вертикали при SB_LINEUP/SB_LINEDOWN
#define HSCROLL_LINE_UPDOWN  1 // коэффициент для изменение позиции по горизонтали при SB_LINEUP/SB_LINEDOWN

#define LOCK_TYPES     2
#define MAX_UNBLOCKED  3  // максимальное число заблокированных сообщений + 1

#define KINETIC_TIMER_ID          1
#define KINETIC_TIMER_TIME        16
#define KINETIC_SCROLL_FRICTION   0.001f


extern HINSTANCE g_hinst;
extern unsigned int g_unblocked[LOCK_TYPES][MAX_UNBLOCKED];

#endif /* __GLOBALS_ */