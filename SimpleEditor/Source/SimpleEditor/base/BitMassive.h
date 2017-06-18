#ifndef __BITMASSIVE_
#define __BITMASSIVE_

#ifdef __cplusplus
extern "C" {
#endif


typedef unsigned char* bitmas_t;

// инициализация массива, size - количество элементов
extern bitmas_t InitBitmas(size_t size);

// инициализация массива со сброшенными битами, size - количество элементов
extern bitmas_t CInitBitmas(size_t size);

// в массив destination будет скопирован массив source
extern void CopyBitmas(bitmas_t destination, bitmas_t source, size_t size);

// установка бита с номером element в массиве
extern void SetBit(bitmas_t bitmas, unsigned int element);

// сброс бита с номером element в массиве
extern void ClearBit(bitmas_t bitmas, unsigned int element);

// проверка установлен ли бит с номером element. Возвращает: 1 - да, 0 - нет
extern char TestBit(bitmas_t bitmas, unsigned int element);

// сброс всех битов в массиве, size - количество элементов
extern void ClearBitmas(bitmas_t bitmas, size_t size);

// циклический сдвиг влево (count от 0 до 8)
extern void ShlBitmas(bitmas_t bitmas, size_t size, unsigned int count);

// циклический сдвиг вправо
//extern void ShrBitmas(bitmas_t bitmas, size_t size, unsigned int count);

// получить число из битов массива
extern unsigned int GetNumberFromBitmas(bitmas_t bitmas, unsigned int start, unsigned int end);

// удаление массива
extern void DestroyBitmas(bitmas_t bitmas);

#ifdef __cplusplus
}
#endif


#endif __BITMASSIVE_
