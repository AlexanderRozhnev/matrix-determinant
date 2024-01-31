/* 
    Выделение и освобождение памяти для динамических двумерных массивов
*/

#ifndef ARRAY_MEMORY_H
#define ARRAY_MEMORY_H

#include <stdlib.h>

/*  Выделение памяти 2-мерного массива
    Возвращает указатель на 2-мерный динамический массив */
double** AllocArrayDouble(size_t N, size_t M);

/*  Освобождение памяти 2-мерного массива */
void FreeArray(double** array, size_t N);

#endif