#include "..\debug.h"

#include "editor.h"
#include "selection.h"
#include "seregexp.h"
#include "undo.h"

#define REPLACE_POS_START 20  // количество позиций, выделяемых сначала
#define REPLACE_POS_KOEF 2  // каждый раз выделяется в 2 раза больше, чем уже есть



static void deleteReplacePositions(memory_system_t *pmemSys, replace_list_t *rlist)
{
  unsigned int i;

  if (!rlist->founded) // если регекспы включены, то строки хранятся в самих позициях
  {
    for (i = 0; i < rlist->number; i++)
    {
      _ASSERTE(rlist->positions[i].str != NULL);
      MemorySystemFree(pmemSys, rlist->positions[i].str);
    }
  }
  if (rlist->positions)
    MemorySystemFree(pmemSys, rlist->positions);
}

static void deleteUndoCellData(memory_system_t *pmemSys, undo_t *pundoCell)
{
  if (pundoCell->dataType == UNDO_DATA_NOT_INITIALIZED)
  {
    _ASSERTE(pundoCell->data == NULL);
    return;
  }

  switch (pundoCell->dataType)
  {
  case UNDO_DATA_TEXT:
    if (((undo_data_text_t*)pundoCell->data)->text)
      MemorySystemFree(pmemSys, ((undo_data_text_t*)pundoCell->data)->text);
    break;

  case UNDO_DATA_REPLACE_LIST:
    {
      replace_list_t* rlist = (replace_list_t*)pundoCell->data;

      if (rlist->Find)
        MemorySystemFree(pmemSys, rlist->Find);
      if (rlist->Replace)
        MemorySystemFree(pmemSys, rlist->Replace);
      if (rlist->founded) // замена была без регулярных выражений
        MasDestroyMassive(pmemSys, rlist->founded);
      deleteReplacePositions(pmemSys, rlist);
    }
    break;
  }
  MemorySystemFree(pmemSys, pundoCell->data);
  pundoCell->data = NULL;
  pundoCell->dataType = UNDO_DATA_NOT_INITIALIZED;
}

static void deleteUndoCell(undo_info_t *pundo, undo_t *pcell)
{
  if (pcell->prev)
    pcell->prev->next = pcell->next;
  else
    pundo->head = pcell->next;

  if (pcell->next)
    pcell->next->prev = pcell->prev;
  else
    pundo->tail = pcell->prev;

  if (pundo->current == pcell)
    pundo->current = NULL;
  if (pundo->unmodAction == pcell)
    pundo->unmodAction = NULL;

  deleteUndoCellData(&pundo->memory, pcell);
  MemorySystemFree(&pundo->memory, pcell);
}

static void deleteUndoList(undo_info_t *pundo, undo_t *pcell)
{
  undo_t *ptr;

  if (pcell->prev)
    pcell->prev->next = NULL;

  while (pcell)
  {
    ptr = pcell;
    pcell = pcell->next;
    deleteUndoCell(pundo, ptr);
  }
}

static char deleteLastUndoAction(undo_info_t *pundo)
{
  // first of all we should try to delete all next cells if current is not the last
  if (pundo->current->next != NULL && pundo->current->next->actionType != ACTION_NOT_INITIALIZED)
  {
    _ASSERTE(pundo->current->actionType != ACTION_NOT_INITIALIZED);
    deleteUndoList(pundo, pundo->current->next);

    pundo->current->next = MemorySystemCallocate(&pundo->memory, 1, sizeof(undo_t));
    _ASSERTE(pundo->current->next != NULL);
    pundo->current->next->prev = pundo->current;
    pundo->tail = pundo->current->next;
  }
  else if (pundo->head->actionModifier == UNDO_GROUP_BEGIN)
  {
    // delete a group if the first action is its beginning (note that group extinction policy differs from auto group's one)
    while (pundo->head && pundo->head != pundo->current &&
      (pundo->head->actionModifier == UNDO_GROUP_BEGIN || pundo->head->actionModifier == UNDO_GROUP_MEMBER || pundo->head->actionModifier == UNDO_GROUP_END))
      deleteUndoCell(pundo, pundo->head);
  }
  else if (pundo->head != pundo->current)
  {
    _ASSERTE(pundo->head->actionModifier != UNDO_GROUP_MEMBER && pundo->head->actionModifier != UNDO_GROUP_END);

    if (pundo->head->actionModifier == UNDO_AUTOGROUP_BEGIN)
    {
      // Auto group extinction policy differs from group policy - auto groups should be cut, not deleted.

      _ASSERTE(pundo->head->next->actionModifier == UNDO_AUTOGROUP_END || pundo->head->next->actionModifier == UNDO_AUTOGROUP_MEMBER);
      if (pundo->head->next->actionModifier == UNDO_AUTOGROUP_MEMBER)
        pundo->head->next->actionModifier = UNDO_AUTOGROUP_BEGIN;
      else
        pundo->head->next->actionModifier = UNDO_SINGLE_ACTION;
    }
    deleteUndoCell(pundo, pundo->head);
  }
  else 
    return 0; // head == current

  return 1;
}

// Функция выделяет память из системы памяти и обнуляет её. Если память не выделить, то
// она удаляет самые старые действия
static void* undoAllocate(undo_info_t *pundo, size_t size)
{
  void *ptr = NULL;

  while ((ptr = MemorySystemCallocate(&pundo->memory, 1, size)) == NULL)
  {
    if (!deleteLastUndoAction(pundo))
      break; // all undo actions were freed, but we still don't have enough memory
  }

  return ptr;
}

// Функция перевыделяет память из системы памяти. Если память не выделить, то
// она удаляет самые старые действия
static void* undoReallocate(undo_info_t *pundo, void *p, size_t size)
{
  void *ptr = NULL;

  while ((ptr = MemorySystemReallocate(&pundo->memory, p, size)) == NULL)
  {
    if (!deleteLastUndoAction(pundo))
      break; // all undo actions were freed, but we still don't have enough memory
  }

  return ptr;
}

extern void ClearUndo(undo_info_t *pundo)
{
  _ASSERTE(pundo != NULL);
  if (!pundo->undoAvailable)
    return;

  deleteUndoList(pundo, pundo->head);
  pundo->head = pundo->current = pundo->tail = undoAllocate(pundo, sizeof(undo_t));

  if (!pundo->head)
  {
    MemorySystemClose(&pundo->memory);
    pundo->undoAvailable = 0;
  }
}

// У одного документа флаг модификации не установлен. Функция устанавливает его
extern void RemoveUnmodAction(undo_info_t *pundo)
{
  pundo->unmodAction = NULL;
}

extern void DestroyUndo(undo_info_t *pundo)
{
  if (pundo->undoAvailable)
  {
    ClearUndo(pundo);
    MemorySystemClose(&pundo->memory);
    pundo->undoAvailable = 0;
    pundo->head = pundo->current = pundo->tail = NULL;
  }
}

extern char PutSymbol(undo_info_t *pundo, unsigned int number, unsigned int index, wchar_t sym)
{
  undo_data_symbol_t *pdata;

  if (!pundo->isSaveAllowed || !pundo->undoAvailable) // сохранение действий запрещено
    return 0;

  _ASSERTE(pundo->current != NULL);

  if (pundo->current->dataType != UNDO_DATA_SYMBOL)
  {
    deleteUndoCellData(&pundo->memory, pundo->current);

    if ((pdata = undoAllocate(pundo, sizeof(undo_data_symbol_t))) == NULL)
    {
      ClearUndo(pundo);
      return 0;
    }
    pundo->current->data = pdata;
    pundo->current->dataType = UNDO_DATA_SYMBOL;
  }
  else
  {
    _ASSERTE(pundo->current->data != NULL);
    pdata = (undo_data_symbol_t*)pundo->current->data;
  }

  pdata->pos.x = index;
  pdata->pos.y = number;
  pdata->sym = sym;

  return 1;
}

extern void NextCell(undo_info_t *pundo, char actionType, char isDocModified)
{
  if (!pundo->isSaveAllowed || !pundo->undoAvailable) // сохранение действий запрещено
    return;

  if (pundo->current->dataType == UNDO_DATA_NOT_INITIALIZED)
  {
    pundo->current->actionType = ACTION_NOT_INITIALIZED; // параноя
    return;
  }

  if (!isDocModified)
    pundo->unmodAction = pundo->current;
  else if (pundo->unmodAction == pundo->current) // теоретически этого не должно быть
    pundo->unmodAction = NULL;
  
  pundo->current->actionType = actionType;
  pundo->current->actionModifier = UNDO_SINGLE_ACTION;

  if (pundo->isGrouping)
  {
    undo_t *pcell = pundo->current->prev;

    pundo->current->actionModifier = UNDO_GROUP_BEGIN;
    if (pcell)
    {
      _ASSERTE(pcell->actionType != ACTION_NOT_INITIALIZED);
      if (pcell->actionModifier == UNDO_GROUP_BEGIN || pcell->actionModifier == UNDO_GROUP_MEMBER)
        pundo->current->actionModifier = UNDO_GROUP_MEMBER;
    }
  }

  // если были отменены действия и продолжено редактирование => надо удалить из стека отменённые действия
  if (pundo->current != pundo->tail) 
    deleteUndoList(pundo, pundo->current->next);

  if ((pundo->tail->next = undoAllocate(pundo, sizeof(undo_t))) == NULL)
  {
    ClearUndo(pundo);
    return;
  }
  else
  {
    pundo->tail = pundo->tail->next;
    pundo->tail->prev = pundo->current;
    pundo->current = pundo->tail;

    pundo->current->actionType = ACTION_NOT_INITIALIZED; 
    pundo->current->dataType = UNDO_DATA_NOT_INITIALIZED; 
  }

  // тут будем автоматически объединять в группы определённые действия
  if (!pundo->isGrouping && pundo->current->prev != pundo->head)
  {
    undo_t *pcurrent = pundo->current->prev, *pcell = pcurrent->prev;

    // 1. подряд идущие символы
    if (pcurrent->actionType == ACTION_SYMBOL_INPUT && pcell->actionType == ACTION_SYMBOL_INPUT)
    {
      undo_data_symbol_t *pdata = (undo_data_symbol_t*)pcell->data;

      if (pdata->pos.y == ((undo_data_symbol_t*)pcurrent->data)->pos.y && 
          pdata->pos.x+1 == ((undo_data_symbol_t*)pcurrent->data)->pos.x)
      {
        if (pcell->actionModifier == UNDO_AUTOGROUP_END)
        {
          pcurrent->actionModifier = UNDO_AUTOGROUP_END;
          pcell->actionModifier = UNDO_AUTOGROUP_MEMBER;
        }
        else if (pcell->actionModifier == UNDO_SINGLE_ACTION)
        {
          pcurrent->actionModifier = UNDO_AUTOGROUP_END;
          pcell->actionModifier = UNDO_AUTOGROUP_BEGIN;
        }
      }
    }
    //while (pcell->actionType == ACTION_SYMBOL_INPUT && pcell->)
  }
}

// функция копирует в буфер текст из указанного диапазона
static wchar_t* copyText(undo_info_t *pundo, pline_t psLine, pline_t peLine, unsigned int si, unsigned int ei)
{
  wchar_t *buff;
  size_t size;

  _ASSERTE(psLine != peLine || si != ei);

  size = GetTextSize(psLine, si, peLine, ei);

  if ((buff = MemorySystemAllocate(&pundo->memory, (size + 1) * sizeof(wchar_t))) == NULL)
    return NULL;

  // скопируем текст в буфер
  CopyTextToBuffer(psLine, si, peLine, ei, buff);
  
  return buff;
}

extern char PutText(undo_info_t *pundo,
                    line_info_t *piStartLine, unsigned int si, 
                    line_info_t *piEndLine, unsigned int ei)
{
  wchar_t *buff;
  undo_data_text_t *pdata;

  if (!pundo->isSaveAllowed || !pundo->undoAvailable) // сохранение действий запрещено
    return 0;

  if (pundo->current->dataType != UNDO_DATA_TEXT)
  {
    deleteUndoCellData(&pundo->memory, pundo->current);

    if ((pdata = undoAllocate(pundo, sizeof(undo_data_text_t))) == NULL)
    {
      ClearUndo(pundo);
      return 0;
    }
    pundo->current->data = pdata;
    pundo->current->dataType = UNDO_DATA_TEXT;
  }
  else
  {
    pdata = (undo_data_text_t*)pundo->current->data;
    if (pdata->text)
    {
      MemorySystemFree(&pundo->memory, pdata->text);
      pdata->text = NULL;
    }
  }

  if ((buff = copyText(pundo, piStartLine->pline, piEndLine->pline, si, ei)) == NULL)
  {
    ClearUndo(pundo);
    return 0;
  }

  pdata->pos.x = si;
  pdata->pos.y = piStartLine->realNumber;
  pdata->text = buff;  

  return 1;
}

extern char PutArea(undo_info_t *pundo, 
                    unsigned int sNumber, unsigned int sIndex,
                    unsigned int eNumber, unsigned int eIndex)
{
  undo_data_area_t *pdata;

  if (!pundo->isSaveAllowed || !pundo->undoAvailable) // сохранение действий запрещено
    return 0;

  if (pundo->current->dataType != UNDO_DATA_AREA)
  {
    deleteUndoCellData(&pundo->memory, pundo->current);

    if ((pdata = undoAllocate(pundo, sizeof(undo_data_area_t))) == NULL)
    {
      ClearUndo(pundo);
      return 0;
    }
    pundo->current->data = pdata;
    pundo->current->dataType = UNDO_DATA_AREA;
  }
  else
  {
    _ASSERTE(pundo->current->data != NULL);
    pdata = (undo_data_area_t*)pundo->current->data;
  }

  pdata->startPos.x = sIndex;
  pdata->startPos.y = sNumber;
  pdata->endPos.x = eIndex;
  pdata->endPos.y = eNumber;

  return 1;
}

extern char PutFindReplaceText(undo_info_t *pundo, const wchar_t *Find, const wchar_t *Replace, char useRegexp)
{
  wchar_t *text;
  replace_list_t *pdata;

  if (!pundo->isSaveAllowed || !pundo->undoAvailable) // сохранение действий запрещено
    return 0;

  if (pundo->current->dataType != UNDO_DATA_NOT_INITIALIZED)
    deleteUndoCellData(&pundo->memory, pundo->current);

  if ((pdata = undoAllocate(pundo, sizeof(replace_list_t))) == NULL)
  {
    ClearUndo(pundo);
    return 0;
  }
  pundo->current->data = (void*)pdata;
  pundo->current->dataType = UNDO_DATA_REPLACE_LIST;

  //if ((pundo->stack[pundo->stackIndex].data.rlist = calloc(1, sizeof(replace_list_t))) == NULL)
  //{
  //  ClearUndo(pundo);
  //  return 0;
  //}

  pdata->number = 0;  // текущее количество позиций
  pdata->size = 0;    // всего выделено памяти под позиции
  pdata->founded = NULL; // показатель того, что используются регекспы
  if (!useRegexp && (pdata->founded = MasInitMassive(&pundo->memory)) == NULL)
  {
    ClearUndo(pundo);
    return 0;
  }

  if ((text = undoAllocate(pundo, sizeof(wchar_t) * (wcslen(Find) + 1))) == NULL)
  {
    ClearUndo(pundo);
    return 0;
  }
  wcscpy(text, Find);
  pdata->Find = text;

  if ((text = undoAllocate(pundo, sizeof(wchar_t) * (wcslen(Replace) + 1))) == NULL)
  {
    ClearUndo(pundo);
    return 0;
  }
  wcscpy(text, Replace);
  pdata->Replace = text;

  return 1;
}

// Функция для заполнения полей positions и founded структуры replace_list_t.
// До вызова это функции структура replace_list_t уже должна существовать,
// то есть должна быть вызвана функция PutFindReplaceText
extern char PutReplacePos(undo_info_t *pundo, const wchar_t *str, unsigned int len, position_t *pos)
{
  replace_list_t *pdata;

  if (!pundo->isSaveAllowed || !pundo->undoAvailable)
    return 0;

  pdata = (replace_list_t*)pundo->current->data;
  _ASSERTE(pdata != NULL && pundo->current->dataType == UNDO_DATA_REPLACE_LIST);

  if (pdata->number == pdata->size)
  {
    undo_rlist_position_t *tmp = pdata->positions;
    if (pdata->size)
    {
      pdata->size *= REPLACE_POS_KOEF;
      pdata->positions = undoReallocate(pundo, tmp, sizeof(replace_list_t) * pdata->size);
    }
    else
    {
      pdata->size = REPLACE_POS_START;
      pdata->positions = undoAllocate(pundo, sizeof(replace_list_t) * REPLACE_POS_START);
    }

    if (!pdata->positions)
    {
      if (tmp)
        MemorySystemFree(&pundo->memory, tmp);
      ClearUndo(pundo);
      return 0;
    }
  }

  // сохраним позицию, где произошла замена
  pdata->positions[pdata->number].x = pos->x;  
  pdata->positions[pdata->number].y = pos->y;

  if (pdata->founded)
  {
    // вставим то, что заменили, в массив
    if ((pdata->positions[pdata->number].str = MasAddString(&pundo->memory, pdata->founded, str, len)) == NULL)
    {
      ClearUndo(pundo);
      return 0;
    }
  }
  else  
  {
    // используются регекспы, надо сохранять строку прямо в структуре позиции
    if ((pdata->positions[pdata->number].str = undoAllocate(pundo, sizeof(wchar_t) * (len + 1))) == NULL)
    {
      ClearUndo(pundo);
      return 0;
    }
    wcsncpy(pdata->positions[pdata->number].str, str, len);
    pdata->positions[pdata->number].str[len] = L'\0';
  }

  
  pdata->number++;

  return 1;
}

static char getOperation(undo_info_t *pundo, undo_t *paction)
{
  if (!pundo->current || pundo->current->actionType == ACTION_NOT_INITIALIZED)
    return 0;
  memcpy(paction, pundo->current, sizeof(undo_t));
  return 1;
}

static char stepBack(undo_info_t *pundo)
{
  if (!pundo->current || !pundo->current->prev)
    return 0;
  _ASSERTE(pundo->current->prev->actionType != ACTION_NOT_INITIALIZED);
  pundo->current = pundo->current->prev;
  return 1;
}

static char stepForward(undo_info_t *pundo)
{
  if (!pundo->current || pundo->current == pundo->tail)
    return 0;
  pundo->current = pundo->current->next;
  return 1;
}

extern char IsUndoAvailable(undo_info_t *pundo)
{
  if (!pundo->undoAvailable)
    return 0;

  if (!pundo->current || pundo->current == pundo->head)
    return 0;

  return 1;
}

extern char IsRedoAvailable(undo_info_t *pundo)
{
  if (!pundo->undoAvailable)
    return 0;

  if (!pundo->current || pundo->current == pundo->tail)
    return 0;
  return 1;
}

extern void StartUndoGroup(undo_info_t *pundo)
{
  if (pundo->undoAvailable)
    pundo->isGrouping = 1;
}

extern void EndUndoGroup(undo_info_t *pundo)
{
  if (pundo->undoAvailable && pundo->isGrouping && pundo->current)
  {
    undo_t *pcell = pundo->current->prev;
    while (pcell && pcell->actionModifier == UNDO_GROUP_MEMBER)
      pcell = pcell->prev;

    if (pcell)
    {
      // first case: we have successfully found the beginning 

      _ASSERTE(pcell->actionType != ACTION_NOT_INITIALIZED);
      _ASSERTE(pcell->actionModifier == UNDO_GROUP_BEGIN);
      if (pundo->current->prev == pcell)
        pcell->actionModifier = UNDO_SINGLE_ACTION;
      else
        pundo->current->prev->actionModifier = UNDO_GROUP_END;
    }
    else  
    {
      // second case: all actions are not initialized or group too big and we have lost the beginning
      // (but theoretically could be only the first supposition)
      
      ClearUndo(pundo);
    }
  }

  pundo->isGrouping = 0;
}

// Функция отменяет/применяет замену. isUndo = 1/0 - отменить/применить
static char undoRedoReplaceAll(common_info_t *pcommon, caret_info_t *pcaret, pselection_t psel,
                               line_info_t *pfirstLine, pline_t head, undo_t *paction, char isUndo)
{
  HDC hdc;
  pos_info_t caretPos, flinePos;
  int i, reduced, added, end;   //  end - докуда изменяется i (end не включён в границу). i - счетчик замен
  unsigned int oldStrLen, newStrLen, index = 0, len, number = 0;
  wchar_t *newStr = NULL;
  pline_t pline = head->next, ptmp = NULL;  
  replace_list_t *pdata = (replace_list_t*)paction->data;   // информация о заменах
  long int maxLength = pcommon->isWordWrap ? pcommon->clientX - pcommon->di.leftIndent : MAX_LENGTH;
  sedit_regexp_t regexp;

  _ASSERTE(paction->dataType == UNDO_DATA_REPLACE_LIST && pdata != NULL);
  if (!pdata->number)
    return 0;

  pcaret->type |= CARET_IS_MODIFIED;
  caretPos.piLine = &pcaret->line;
  caretPos.index = pcaret->index;
  SetStartLine(&caretPos);

  flinePos.piLine = pfirstLine;
  flinePos.index = pfirstLine->pline->begin;
  SetStartLine(&flinePos);

  if (pcommon->useRegexp)
    RegexpCompile(&regexp, pdata->Find);

  if (isUndo)  // надо отменять замены
  {
    oldStrLen = wcslen(pdata->Replace);  // в режиме с регекспами пересчитывается

    // дойдём до последней замены, тк отменять надо с конца
    while (number != pdata->positions[pdata->number - 1].y)
    {
      pline = pline->next;
      _ASSERTE(pline != NULL);
      if (pline->prealLine != pline->prev->prealLine)
        number++;
    }
    i = pdata->number - 1;   // последняя позиция
    end = -1;                // чтобы i изменялось до 0 включительоно
  }
  else  // надо применить отменённые замены
  {
    // newStr и newStrLen имеет смысл считать тут только для режима без регекспов
    if (!pcommon->useRegexp)
    {
      newStrLen = wcslen(pdata->Replace);  
      newStr = pdata->Replace;
    }

    i = 0;
    end = pdata->number;
  }

  hdc = GetDC(pcommon->hwnd);
  SelectObject(hdc, pcommon->hfont);

  while (i != end)
  {
    if (isUndo)
    {
      // получим строку, которая была в тексте до замены, и её длину
      newStr = pdata->positions[i].str;
      newStrLen = wcslen(newStr);

      // получим длину строки, которая сейчас в тексте
      if (pcommon->useRegexp)
      {
        RegexpGetMatch(&regexp, pdata->positions[i].str, 0);
        oldStrLen = RegexpGetReplaceString(pdata->Replace, newStr, NULL, regexp.subExpr, regexp.nSubExpr);
      }

      while (number != pdata->positions[i].y)
      {
        pline = pline->prev;
        if (pline->prealLine != pline->next->prealLine)
          number--;
      }
    }
    else
    {
      // надо удалить то, что было найдено при поиске. Для этого нужна длина
      oldStrLen = wcslen(pdata->positions[i].str);

      // теперь надо получить то, на что будем заменять
      if (pcommon->useRegexp)
      {
        RegexpGetMatch(&regexp, pdata->positions[i].str, 0);
        newStrLen = RegexpGetReplaceString(pdata->Replace, pdata->positions[i].str, &newStr,
                                           regexp.subExpr, regexp.nSubExpr);
      }

      while (number != pdata->positions[i].y)
      {
        pline = pline->next;
        _ASSERTE(pline);
        if (pline->prealLine != pline->prev->prealLine)
          number++;
      }
    }
    index = pdata->positions[i].x;

    if (!ptmp || ptmp->prealLine != pline->prealLine)  // новая строка
    {
      if (isUndo)
      {
        while (pline->prev->prealLine == pline->prealLine)
          pline = pline->prev;
      }
      pcommon->nLines -= UniteLine(pline);

      if (ptmp)
      { 
        // обработаем строку, в которой все замены уже сделаны
        ReduceLine(hdc, &pcommon->di, ptmp, maxLength, &reduced, &added);
        pcommon->nLines += added;
      }

      ptmp = pline;
      len = RealStringLength(pline);
    }

    DeleteSubString(pline, len, index, oldStrLen);
    InsertSubString(pline, len - oldStrLen, index, newStr, newStrLen);
    len += newStrLen - oldStrLen;

    // при redo для каждой замены мы получаем строку, теперь её надо освободить
    if (pcommon->useRegexp && !isUndo)  
    {
      _ASSERTE(newStr != NULL);
      free(newStr);
      newStr = NULL;
    }

    i = (isUndo == 1) ? i - 1 : i + 1;  // если отмена замены, то бежим с конца в начало. Иначе наоборот.
  }

  if (pcommon->useRegexp)
    RegexpFree(&regexp);

  // обработаем последнюю строку
  ReduceLine(hdc, &pcommon->di, ptmp, maxLength, &reduced, &added);  
  pcommon->nLines += added;

  UpdateLine(&flinePos);
  GetNumbersFromLines(head->next, 0, 1, &pfirstLine->pline, &pfirstLine->number);

  if (!UpdateLine(&caretPos))
    pcaret->index = pcaret->line.pline->end;
  GetNumbersFromLines(head->next, 0, 1, &pcaret->line.pline, &pcaret->line.number);
  UpdateCaretSizes(hdc, pcommon, pcaret);
  ReleaseDC(pcommon->hwnd, hdc); // освободим контекст устройства

  if (IS_SHIFT_PRESSED)
    StartSelection(psel, &pcaret->line, pcaret->index);

  SEInvalidateRect(pcommon, NULL, FALSE);
  SEUpdateWindow(pcommon, pcaret, pfirstLine);
  UpdateScrollBars(pcommon, pcaret, pfirstLine);

  //GotoCaret(pcommon, pcaret, pfirstLine);
  UpdateCaretPos(pcommon, pcaret, pfirstLine);

  CaretModified(pcommon, pcaret, pfirstLine, 0);

  return 1;
}

// функция устанавливает каретку в сохранённое место
static void setCaretToAppropriatePos(undo_t *paction,
                                     common_info_t *pcommon, 
                                     caret_info_t *pcaret, 
                                     line_info_t *pfirstLine,
                                     text_t *ptext)
{
  line_info_t oldLine, newLine;
  unsigned int index;
  HDC hdc;

  if (paction->actionType == ACTION_REPLACE_ALL) // при ACTION_REPLACE_ALL позиция каретки не запоминается
    return;

  _ASSERTE(paction->dataType != UNDO_DATA_REPLACE_LIST);

  oldLine = pcaret->line;

  if (paction->dataType == UNDO_DATA_SYMBOL)
  {
    undo_data_symbol_t *pdata = (undo_data_symbol_t*)paction->data;
    newLine.realNumber = pdata->pos.y;
    index = pdata->pos.x;
  }
  else if (paction->dataType == UNDO_DATA_TEXT)
  {
    undo_data_text_t *pdata = (undo_data_text_t*)paction->data;
    newLine.realNumber = pdata->pos.y;
    index = pdata->pos.x;
  }
  else if (paction->dataType == UNDO_DATA_AREA)
  {
    undo_data_area_t *pdata = (undo_data_area_t*)paction->data;
    newLine.realNumber = pdata->startPos.y;
    index = pdata->startPos.x;
  }

  hdc = GetDC(pcommon->hwnd);
  SelectObject(hdc, pcommon->hfont);
  newLine.pline = GetLineFromRealPos(GetNearestRealLine(pcaret, pfirstLine, ptext, pcaret->line.realNumber),
                                     newLine.realNumber, index, &newLine.number);
  pcaret->line = newLine;
  pcaret->index = index;
  pcaret->type = pcaret->type | CARET_IS_MODIFIED;
  UpdateCaretSizes(hdc, pcommon, pcaret);
  ReleaseDC(pcommon->hwnd, hdc);

  GotoCaret(pcommon, pcaret, pfirstLine);

#ifdef _DEBUG
  {
    int error;
    _ASSERTE((error = SendMessage(pcommon->hwnd, SE_DEBUG, 0, 0)) == SEDIT_NO_ERROR);
  }
#endif

  if (oldLine.realNumber != pcaret->line.realNumber)
  {
    // перерисуем старую и новую линии с кареткой
    RedrawCaretLine(pcommon, pcaret, pfirstLine, &oldLine);
    RedrawCaretLine(pcommon, pcaret, pfirstLine, &pcaret->line);    
  }
}

static char undoOneOperation(undo_info_t *pundo, common_info_t *pcommon, caret_info_t *pcaret, 
                             line_info_t *pfirstLine, selection_t *psel, text_t *ptext, undo_t *paction)
{
  char isDocModified = pcommon->isModified; 

  setCaretToAppropriatePos(paction, pcommon, pcaret, pfirstLine, ptext);

  if (psel->isSelection)
  {
    psel->isSelection = 0;
    UpdateSelection(pcommon, pcaret, pfirstLine, psel, NULL);
  }

  UNDO_RESTRICT_SAVE(pundo);
  RESTRICT_SMART_INPUT(pcommon);
  switch (paction->actionType)
  {
  case ACTION_SYMBOL_INPUT:
  case ACTION_NEWLINE_ADD:
    //SendMessage(pcommon->hwnd, WM_KEYDOWN, VK_DELETE, 0);
    DeleteNextChar(pcommon, pcaret, psel, pfirstLine, pundo);
    break;

  case ACTION_SYMBOL_DELETE:
    InputChar(pcommon, pcaret, psel, pfirstLine, pundo, ((undo_data_symbol_t*)paction->data)->sym);
    break;

  case ACTION_NEWLINE_DELETE:
    NewLine(pcommon, pcaret, psel, pfirstLine, pundo);
    break;

  case ACTION_TEXT_DELETE:
    {
      pos_info_t pos;            // для сохранения позиции в которую вставляем
      line_info_t selStartLine;
      //char shiftPressed = IS_SHIFT_PRESSED;

      //StartSelection(psel, &pcaret->line, pcaret->index);
      //psel->shiftPressed = 0;
      //pos.piLine = &psel->pos.startLine;
      //pos.index = psel->pos.startIndex;
      pos.piLine = &selStartLine;
      pos.index = pcaret->index;
      selStartLine = pcaret->line;
      SetStartLine(&pos);

      PasteFromBuffer(pcommon, pcaret, psel, pfirstLine, pundo, ((undo_data_text_t*)paction->data)->text);
      //psel->shiftPressed = shiftPressed;

      UpdateLine(&pos);

      UNDO_ALLOW_SAVE(pundo);
      if (PutArea(pundo, selStartLine.realNumber, pos.index, pcaret->line.realNumber, pcaret->index))
        pundo->current->actionType = ACTION_TEXT_PASTE;

      StartSelection(psel, &selStartLine, pos.index);
      ChangeSelection(psel, &pcaret->line, pcaret->index);
      UpdateSelection(pcommon, pcaret, pfirstLine, psel, NULL);
    }
    break;

  case ACTION_TEXT_PASTE:
    {
      line_info_t textEnd;

      StartSelection(psel, &pcaret->line, pcaret->index);
      textEnd.realNumber = ((undo_data_area_t*)paction->data)->endPos.y;
      textEnd.pline = GetLineFromRealPos(GetNearestRealLine(pcaret, pfirstLine, ptext, textEnd.realNumber), 
                                         textEnd.realNumber, 
                                         ((undo_data_area_t*)paction->data)->endPos.x, 
                                         &textEnd.number);
      ChangeSelection(psel, &textEnd, ((undo_data_area_t*)paction->data)->endPos.x);

      UNDO_ALLOW_SAVE(pundo);
      if (PutText(pundo, &psel->pos.startLine, psel->pos.startIndex, &psel->pos.endLine, psel->pos.endIndex))
        pundo->current->actionType = ACTION_TEXT_DELETE;
      UNDO_RESTRICT_SAVE(pundo);

      DeleteSelection(pcommon, pcaret, psel, pfirstLine, pundo);
    }
    break;

  case ACTION_REPLACE_ALL:
    undoRedoReplaceAll(pcommon, pcaret, psel, pfirstLine, ptext->pfakeHead, paction, 1);
    break;
  }
  ALLOW_SMART_INPUT(pcommon);
  UNDO_ALLOW_SAVE(pundo);

  pcommon->isModified = pundo->unmodAction == pundo->current ? 0 : 1;

  // обновим флаг модификации у операции, которая была отменена, так как он мог измениться.
  // при применении операции назад флаг будет верным
  //pundo->current->isModified = isDocModified;
  if (isDocModified && pundo->current == pundo->unmodAction)
    pundo->unmodAction = NULL;
  else if (!isDocModified)
    pundo->unmodAction = pundo->current;

#ifdef _DEBUG
  {
    int error;
    _ASSERTE((error = SendMessage(pcommon->hwnd, SE_DEBUG, 0, 0)) == SEDIT_NO_ERROR);
  }
#endif

  return 1;
}

static char redoOneOperation(undo_info_t *pundo, common_info_t *pcommon, caret_info_t *pcaret, 
                             line_info_t *pfirstLine, selection_t *psel, text_t *ptext, undo_t *paction)
{
  char isDocModified = pcommon->isModified;
  
  setCaretToAppropriatePos(paction, pcommon, pcaret, pfirstLine, ptext);

  if (psel->isSelection)
  {
    psel->isSelection = 0;
    UpdateSelection(pcommon, pcaret, pfirstLine, psel, NULL);
  }

  UNDO_RESTRICT_SAVE(pundo);
  RESTRICT_SMART_INPUT(pcommon);
  switch (paction->actionType)
  {
  case ACTION_SYMBOL_INPUT:
    InputChar(pcommon, pcaret, psel, pfirstLine, pundo, ((undo_data_symbol_t*)paction->data)->sym);
    break;

  case ACTION_NEWLINE_ADD:
    NewLine(pcommon, pcaret, psel, pfirstLine, pundo);
    break;

  case ACTION_SYMBOL_DELETE:
  case ACTION_NEWLINE_DELETE:
    //SendMessage(pcommon->hwnd, WM_KEYDOWN, VK_DELETE, 0);
    DeleteNextChar(pcommon, pcaret, psel, pfirstLine, pundo);
    break;

  case ACTION_TEXT_PASTE:  
    {
      line_info_t textEnd;

      StartSelection(psel, &pcaret->line, pcaret->index);
      textEnd.realNumber = ((undo_data_area_t*)paction->data)->endPos.y;
      textEnd.pline = GetLineFromRealPos(GetNearestRealLine(pcaret, pfirstLine, ptext, textEnd.realNumber),
                                         textEnd.realNumber, 
                                         ((undo_data_area_t*)paction->data)->endPos.x, 
                                         &textEnd.number);
      ChangeSelection(psel, &textEnd, ((undo_data_area_t*)paction->data)->endPos.x);

      UNDO_ALLOW_SAVE(pundo);
      if (PutText(pundo, &psel->pos.startLine, psel->pos.startIndex, &psel->pos.endLine, psel->pos.endIndex))
        pundo->current->actionType = ACTION_TEXT_DELETE;
      UNDO_RESTRICT_SAVE(pundo);

      DeleteSelection(pcommon, pcaret, psel, pfirstLine, pundo);
    }
    break;

  case ACTION_TEXT_DELETE:
    {
      pos_info_t pos;            // для сохранения позиции в которую вставляем
      line_info_t selStartLine;
      //char shiftPressed = psel->shiftPressed;

      //StartSelection(psel, &pcaret->line, pcaret->index);
      //psel->shiftPressed = 0;   // чтобы PasteFromBuffer не переустановила начало выделения
      //pos.piLine = &psel->pos.startLine;
      //pos.index = psel->pos.startIndex;
      pos.piLine = &selStartLine;
      pos.index = pcaret->index;
      selStartLine = pcaret->line;
      SetStartLine(&pos);

      PasteFromBuffer(pcommon, pcaret, psel, pfirstLine, pundo, ((undo_data_text_t*)paction->data)->text);
      //psel->shiftPressed = shiftPressed;

      UpdateLine(&pos);

      UNDO_ALLOW_SAVE(pundo);
      if (PutArea(pundo, selStartLine.realNumber, pos.index, pcaret->line.realNumber, pcaret->index))
        pundo->current->actionType = ACTION_TEXT_PASTE;

      StartSelection(psel, &selStartLine, pos.index);
      ChangeSelection(psel, &pcaret->line, pcaret->index);
      UpdateSelection(pcommon, pcaret, pfirstLine, psel, NULL);
    }
    break;

  case ACTION_REPLACE_ALL:
    undoRedoReplaceAll(pcommon, pcaret, psel, pfirstLine, ptext->pfakeHead, paction, 0);
    break;
  }
  ALLOW_SMART_INPUT(pcommon);
  UNDO_ALLOW_SAVE(pundo);

  pcommon->isModified = pundo->current == pundo->unmodAction ? 0 : 1;

  // обновим флаг модификации у операции, которая была применена, так как он мог измениться.
  // при отмене операции флаг будет верным
  //pundo->current->isModified = isDocModified;
  if (isDocModified && pundo->current == pundo->unmodAction)
    pundo->unmodAction = NULL;
  else if (!isDocModified)
    pundo->unmodAction = pundo->current;

#ifdef _DEBUG
  {
    int error;
    _ASSERTE((error = SendMessage(pcommon->hwnd, SE_DEBUG, 0, 0)) == SEDIT_NO_ERROR);
  }
#endif

  return 1;
}

extern char UndoOperation(undo_info_t *pundo, common_info_t *pcommon, caret_info_t *pcaret, 
                          line_info_t *pfirstLine, selection_t *psel, text_t *ptext)
{
  undo_t action;
  char isDrawingRestricted = pcommon->di.isDrawingRestricted;
  range_t range = {0};

  // first of all, if we are grouping operations now, we must stop it
  EndUndoGroup(pundo);

  if (!pundo->undoAvailable || !stepBack(pundo) || !getOperation(pundo, &action))
    return 0;

  if (action.actionModifier == UNDO_GROUP_END || action.actionModifier == UNDO_AUTOGROUP_END)
  {
    // we must undo a group of actions
    RestrictDrawing(pcommon, pcaret, pfirstLine);
    do 
    {
      undoOneOperation(pundo, pcommon, pcaret, pfirstLine, psel, ptext, &action);
      if (!stepBack(pundo) || !getOperation(pundo, &action))
      {
        // error, we must clear undo stack (for example, it could happen when inside undoOneOperation was undo memory error)
        ClearUndo(pundo);

        if (!isDrawingRestricted)
          AllowDrawing(pcommon, pcaret, pfirstLine);

        return 0;
      }
    } while (action.actionModifier != UNDO_GROUP_BEGIN && action.actionModifier != UNDO_AUTOGROUP_BEGIN);
  }

  undoOneOperation(pundo, pcommon, pcaret, pfirstLine, psel, ptext, &action);

  if (pcommon->di.isDrawingRestricted && !isDrawingRestricted)
    AllowDrawing(pcommon, pcaret, pfirstLine);

  DocUpdated(pcommon, SEDIT_ACTION_NONE, &range, pcommon->isModified);

  return 1;
}

extern char RedoOperation(undo_info_t *pundo, common_info_t *pcommon, caret_info_t *pcaret, 
                          line_info_t *pfirstLine, selection_t *psel, text_t *ptext)
{
  undo_t action;
  char isDrawingRestricted = pcommon->di.isDrawingRestricted;
  range_t range = {0};

  if (!pundo->undoAvailable || !getOperation(pundo, &action))
    return 0;

  if (action.actionModifier == UNDO_GROUP_BEGIN || action.actionModifier == UNDO_AUTOGROUP_BEGIN)
  {
    // we must redo a group of actions
    RestrictDrawing(pcommon, pcaret, pfirstLine);
    do 
    {
      redoOneOperation(pundo, pcommon, pcaret, pfirstLine, psel, ptext, &action);
      if (!stepForward(pundo) || !getOperation(pundo, &action))
      {
        // error, we must clear undo stack
        ClearUndo(pundo);

        if (!isDrawingRestricted)
          AllowDrawing(pcommon, pcaret, pfirstLine);

        return 0;
      }
    } while (action.actionModifier != UNDO_GROUP_END && action.actionModifier != UNDO_AUTOGROUP_END);
  }

  redoOneOperation(pundo, pcommon, pcaret, pfirstLine, psel, ptext, &action);
  stepForward(pundo);

  if (pcommon->di.isDrawingRestricted && !isDrawingRestricted)
    AllowDrawing(pcommon, pcaret, pfirstLine);

  DocUpdated(pcommon, SEDIT_ACTION_NONE, &range, pcommon->isModified);

  return 1;
}

extern char InitUndo(undo_info_t *pundo, size_t size)
{
  pundo->isSaveAllowed = 1;
  pundo->undoAvailable = 0;

  if (size > 0)
  {
    if (!MemorySystemOpen(&pundo->memory, size))
      return 0;

    if ((pundo->head = undoAllocate(pundo, sizeof(undo_t))) == NULL)
    {
      MemorySystemClose(&pundo->memory);
      return 0;
    }
    //MasSetMemoryFunctions(MemorySystemAllocate, MemorySystemCallocate, MemorySystemFree);

    pundo->unmodAction = pundo->current = pundo->tail = pundo->head;
    pundo->undoAvailable = 1;
  }

  return 1;
}
