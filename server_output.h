/* 
    Вывод данных сервера в консоль 
*/

#ifndef SERVER_OUTPUT_H
#define SERVER_OUTPUT_H

#include <stdbool.h>

/* Данные для вывода сервера */
struct ServerOutput {
    double determinant;             // Текущий определитель
    double avg_determinant;         // Среднее значение определителя среди NUM_OF_DETERMINANTS
    double prev_Nth_determinant;    // Определитель, полученный NUM_OF_DETERMINANTS матриц назад
    bool full_data;                 // Валидность значений avg_determinant и prev_Nth_determinant
};

/* Вывод на консоль */
void PrintOutputs(const struct ServerOutput* output_data);

#endif