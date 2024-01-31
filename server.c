/*  
    Сервер
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>

#include "array_memory.h"
#include "server_output.h"

/*  Параметры коммуникации */
#define IP_ADDRESS "127.0.0.1"  // адрес сервера для связи
#define PORT 10001              // порт сервера для связи свой клиент получит от ОС автоматически

/*  Параметры входных данных */
#define MATRIX_SIZE 6           // Ширина и высота матрицы (определитель имеет смысл только для квадратных матриц)
#define NUM_OF_DETERMINANTS 5   // Кол-во сохраняемых определителей матриц

/* Глобальные переменные */
int sock;                       // Сокет
double** matrix = NULL;         // Контейнер матрицы
unsigned char* buffer = NULL;   // Буфер приёма сериализованной матрицы

double last_determinants[NUM_OF_DETERMINANTS] = {0}; // Определители последних поступивших матриц
unsigned int next_cell = 0;     // Номер ячейки массива для записи очередного значения

// Данные для вывода в консоль    
struct ServerOutput output_data = {
    .determinant = .0,
    .avg_determinant = .0,
    .prev_Nth_determinant = .0,
    .full_data = false
};

/*  Получение субматрицы путём исключения ряда row_exclude и столбца col_exclude */
void SubMatrix(double** matrix, unsigned int matrix_size, unsigned int row_exclude, unsigned int col_exclude, double** submatrix) {
    int sub_i = 0;
    for (int i = 0; i < matrix_size; ++i) {
        if (i == row_exclude) continue;
        int sub_j = 0;
        for (int j = 0; j < matrix_size; ++j) {
            if (j == col_exclude) continue;
            submatrix[sub_i][sub_j] = matrix[i][j];
            ++sub_j;
        }
        ++sub_i;
    }
}

/*  Вычисление определителя матрицы методом разложения по строке.
    Рекурсивный алгоритм */
double CalculateDeterminant(double** matrix, unsigned int matrix_size) {
    if (matrix_size == 0) return .0;
    if (matrix_size == 1) return 1.;

    if (matrix_size == 2) {
        return matrix[0][0] * matrix[1][1] - matrix[1][0] * matrix[0][1];
    }

    double determinant = .0;
    for (unsigned int i = 0; i < matrix_size; ++i) {
        unsigned int submatrix_size = matrix_size - 1;
        double** submatrix = AllocArrayDouble(submatrix_size, submatrix_size);
        SubMatrix(matrix, matrix_size, 0, i, submatrix);
        int sign = (i % 2) ? -1 : 1;
        determinant += sign * matrix[0][i] * CalculateDeterminant(submatrix, submatrix_size);
        FreeArray(submatrix, submatrix_size);
    }

    return determinant;
}

/* Вывод матрицы в консоль */
void PrintMatrix(double** matrix, size_t matrix_size) {
    for (unsigned int i = 0; i < matrix_size; ++i) {
        for (unsigned int j = 0; j < matrix_size; ++j) {
            printf("%.2e, ", matrix[i][j]);
        }
        printf("\n");
    }
}

/* Десериализация двумерной квадратной матрицы double */
void DeserializeMatrix(unsigned char* buffer, double** matrix, size_t matrix_size) {
    for (unsigned int i = 0; i < matrix_size; ++i) {
        for (unsigned int j = 0; j < matrix_size; ++j) {
            size_t offset = i * matrix_size * sizeof(double) + j * sizeof(double);
            double d = *((double*) (buffer + offset));
            matrix[i][j] = d;
        }
    }
}

/* Обработка сообщения */
void ProcessMessage(char* message, int bytes_read) {
    DeserializeMatrix(message, matrix, MATRIX_SIZE);
    
    // printf("Read bytes: %d\n", bytes_read);
    // PrintMatrix(matrix, MATRIX_SIZE);

    output_data.prev_Nth_determinant = last_determinants[next_cell];
    last_determinants[next_cell] = CalculateDeterminant(matrix, MATRIX_SIZE);
    output_data.determinant = last_determinants[next_cell];
    output_data.avg_determinant = output_data.avg_determinant - output_data.prev_Nth_determinant * (1. / NUM_OF_DETERMINANTS) 
                                    + last_determinants[next_cell] * (1. / NUM_OF_DETERMINANTS);
    PrintOutputs(&output_data);

    next_cell = ++next_cell % NUM_OF_DETERMINANTS;
    if (next_cell == 0) output_data.full_data = true;
}

int ServerConnection() {
    /* Создание сокета */
    int listener = socket(AF_INET, SOCK_STREAM, 0); // 1) AF_INET - домен Internet
                                                    // 2) Stream socket 
                                                    // 3) Протокол по умолчанию (TCP)
    if (listener < 0) {
        perror("Socket error: ");
        exit(EXIT_FAILURE);
    }

    /* Cтруктура с адресом сервера и клиента */
    struct sockaddr_in addr;                    
    addr.sin_family = AF_INET;                  // AF_INET - Семейство адресов Internet
    addr.sin_port = htons(PORT);                // Порт
    addr.sin_addr.s_addr = htonl(INADDR_ANY);   // Принимать любой входящий адрес
    // addr.sin_addr.s_addr = inet_addr(IP_ADDRESS);

    /* Связывание сокета с адресом */
    if (bind(listener, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Bind error: ");
        exit(EXIT_FAILURE);
    }

    /* Создание очереди запросов и перевод в режим ожидания от клиента */
    if (listen(listener, 1) != 0) {
        printf("Server listening error: ");
        exit(EXIT_FAILURE);
    }
    
    printf("Connection established.\n");

    return listener;
}

/* Освобождение ресурсов перед выходом */
void ReleaseResoursesAtExit() {
    /* Закрытие сокета */
    close(sock);
    
    /* Освобождение памяти контейнеров */
    free(buffer);
    FreeArray(matrix, MATRIX_SIZE);
}

int main (int argc, char** argv) {

    int listener = ServerConnection();

    matrix = AllocArrayDouble(MATRIX_SIZE, MATRIX_SIZE);
    buffer = malloc(sizeof(double) * MATRIX_SIZE * MATRIX_SIZE);
    
    /* Регистрация функции для освобождения ресурсов перед выходом */
    atexit(ReleaseResoursesAtExit);

    /* Основной цикл сервера */
    while (1) {
        /* Создаём новый сокет для входящего соединения */
        sock = accept(listener, NULL, NULL);    // дескриптор сокета
        if (sock < 0) {
            perror("Accept failure: ");
            exit(EXIT_FAILURE);
        }

        /* Приём сообщений */
        while (1) {
            /* Получаем сообщение от сервера, оно может быть меньше, но не больше чем размер массива buf */
            int bytes_read = recv(sock, buffer, sizeof(double) * MATRIX_SIZE * MATRIX_SIZE, 0);
            if (bytes_read <= 0) break;

            /* Обработка сообщения */
            ProcessMessage(buffer, bytes_read);
        }
    }
    return EXIT_SUCCESS;
}