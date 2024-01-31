#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <termios.h>

/* Смена режима консоли для чтения консоли без блокировки */
void set_conio_terminal_mode();

/* Обнаружение нажатия клавиши */
int kbhit();

/* Получение символа с консоли */
int getch();

#endif