/* 
    Клиент.

    Клиент по нажатию кнопки или по таймеру высылает на сервер 
    матрицу размером 6x6 со случайными значениями типа double.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <float.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <signal.h>

#include "array_memory.h"
#include "keyboard.h"

/*  Параметры коммуникации */
#define IP_ADDRESS "127.0.0.1"  // Адрес сервера для связи
#define PORT 10001              // Порт сервера для связи свой клиент получит от ОС автоматически

/* Периодичность посылки сообщения клиентом */
#define TIMEOUT_S 1         // Интервал (сек.) через который клиент высылает матрицу не дожидаясь нажатия кнопки

/*  Параметры входных данных */
#define MATRIX_SIZE 6       // Ширина и высота квадратной матрицы (определитель имеет смысл только для квадратных матриц)
#define RANDOM_MAX sqrt(sqrt(sqrt(DBL_MAX))) / 720  // Максимальное (по модулю) случайное значение double 

/* Глобальные переменные */
int sock;                       // Сокет
double** matrix = NULL;         // Контейнер матрицы
unsigned char* buffer = NULL;   // Буфер для сериализации матрицы

/* Генерирует значение double равномерно в диапазононе [-DBL_MAX ....DBL_MAX] */
double RandomDoubleFullRange(void) {
    union {
        float value;
        unsigned char uc[sizeof(float)];
    } u;
  
    //srand(time(NULL));  // Инициализация рандомайзера
    do {
        for (unsigned int i = 0; i < sizeof(float) / sizeof(int); ++i) {
            int random = rand();
            for (unsigned int j = 0; j < sizeof(int); ++j) {
                u.uc[i * sizeof(int) + j] = (unsigned char) random;
                random = random >> 8;
            }
        }
    } while (!isfinite(u.value));
  
    return u.value;
}

/* Генерирует случайное значение в диапазоне [-RAND_MAX ....RAND_MAX] */
double RandomDouble(void) {
    return RandomDoubleFullRange() * (RANDOM_MAX / FLT_MAX);
}

/* Заполняет квадратную матрицу случайными значениями double */
void FillMatrix(double** matrix, size_t matrix_size) {
    for (unsigned int i = 0; i < matrix_size; ++i) {
        for (unsigned int j = 0; j < matrix_size; ++j) {
            double d = RandomDouble();
            matrix[i][j] = d;
        }
    }
}

/* Вывод матрицы в консоль */
void PrintMatrix(double** matrix, size_t matrix_size) {
    for (unsigned int i = 0; i < matrix_size; ++i) {
        for (unsigned int j = 0; j < matrix_size; ++j) {
            printf("%.2e, ", matrix[i][j]);
        }
        printf("\n\r");
    }
}

/* Тестовая матрица. Определитель = 11456 */
void GetTestMatrix(double** matrix) {
    double arr[6][6]  = {{2, 3,	5,	3,	4,	7},
                         {9, 6,	3,	2,	12,	13},
                         {2, 4,	6,	3,	2,	5},
                         {12,2, 6,	8,	3,	1},
                         {7, 9,	4,	6,	3,	0},
                         {7, 2,	8,	9,	0,	2}};

    for (int i = 0; i < MATRIX_SIZE; ++i) {
        for (int j = 0; j < MATRIX_SIZE; ++j) {
            matrix[i][j] = arr[i][j];
        }
    }
}

/* Сериализация двумерной квадратной матрицы double в двоичный буфер */
void SerilializeMatrix(double** matrix, size_t matrix_size, unsigned char* buffer) {
    union UDouble {
        double d;
        unsigned char uc[sizeof(double)];
    };

    for (unsigned int i = 0; i < matrix_size; ++i) {
        for (unsigned int j = 0; j < matrix_size; ++j) {
            union UDouble* p_double = (union UDouble*) &(matrix[i][j]);
            for (unsigned int k = 0; k < sizeof(double); ++k) {
                *buffer = p_double->uc[k];
                ++buffer;
            }
        }
    }
}

/* Отправка всех данных буфера */
int SendAllData(int sock, char* buffer, int length, int flags) {
    int total = 0;
    int n;

    while (total < length) {
        n = send(sock, buffer + total, length - total, flags);
        if (n == -1) break;
        total += n;
    }

    return (n == -1) ? -1 : total;
}

/* Выполнение основной задачи: Передача матрицы. */
void SendMatrix() {
    /* Отсылка сериализованной матрицы и проверка результата */
    if (SendAllData(sock, buffer, sizeof(double) * MATRIX_SIZE * MATRIX_SIZE, 0) < 0) {
        printf("Connection lost.\n");
        perror("Send error:");
        exit(EXIT_FAILURE);
    }
    PrintMatrix(matrix, MATRIX_SIZE);

    /* Подготовка новых данных */
    FillMatrix(matrix, MATRIX_SIZE);
    //GetTestMatrix(matrix);
    SerilializeMatrix(matrix, MATRIX_SIZE, buffer);
}

/* Вызов по таймеру */
void ALARMhandler(int sig) {
    printf("timer\n\r");
    SendMatrix();
    alarm(TIMEOUT_S);           // Перезапуск таймера
}

/* Установка соединения клиента */
int ClientConnection() {
    /* Создание сокета клиента */
    int sock = socket(AF_INET, SOCK_STREAM, 0); // 1) AF_INET - Internet-домен
                                                // 2) SOCK_STREAM - Передача потока данных с предварительной установкой соединения
                                                // 3) 0 - Протокол по умолчанию
    if (sock < 0) {
        perror("Socket error: ");
        exit(EXIT_FAILURE);
    }

    /* Cтруктура с адресом сервера и клиента */
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;                      // AF_INET - Internet-домен
    addr.sin_port = htons(PORT);                    // Порт
    addr.sin_addr.s_addr = inet_addr(IP_ADDRESS);   // ip-address

    /* Установка соединения со стороны клиента */
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Client connect: ");
        exit(EXIT_FAILURE);
    }

    printf("Client connection established. %e, %e, %e\n", DBL_MAX, RANDOM_MAX / 720, (RANDOM_MAX / DBL_MAX));
    return sock;
}

/* Освобождение ресурсов перед выходом */
void ReleaseResoursesAtExit() {
    /* Закрытие сокета */
    close(sock);
    
    /* Освобождение памяти контейнеров */
    free(buffer);
    FreeArray(matrix, MATRIX_SIZE);
}

int main (int argc, char **argv) {
    /* Установка соединения клиента */
    sock = ClientConnection();

    /* Подготовка контейнеров для данных */
    matrix = AllocArrayDouble(MATRIX_SIZE, MATRIX_SIZE);
    buffer = malloc(sizeof(double) * MATRIX_SIZE * MATRIX_SIZE);

    /* Регистрация функции для освобождения ресурсов перед выходом */
    atexit(ReleaseResoursesAtExit);

    /* Начальное состояние матрицы */
    // FillMatrix(matrix, MATRIX_SIZE);
    GetTestMatrix(matrix);
    SerilializeMatrix(matrix, MATRIX_SIZE, buffer);

    /* Смена режима терминала */
    set_conio_terminal_mode();

    /* Запуск таймера - периодическая генерация и отсылка сообщений клиентом */
    signal(SIGALRM, ALARMhandler);
    alarm(TIMEOUT_S);

    /* Основной цикл клиента */
    for (int ch; (ch = getchar()) != EOF;) {
        alarm(0);                   // Останов всех таймеров
        if (ch == 0x03) break;      // Прерывание работы по Ctrl+C
        printf("key\n\r");
        SendMatrix();
        alarm(TIMEOUT_S);           // Перезапуск таймера
    }

    return EXIT_SUCCESS;
}