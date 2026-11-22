#ifndef UART_TOUCHSCREEN_H
#define UART_TOUCHSCREEN_H
#include <stddef.h>
#include <stdint.h>
#include <termios.h>
#ifndef SERIAL_PORT
#define SERIAL_PORT "/dev/serial0"
#endif
#define BAUDRATE B9600
#define MSG_LENGTH 8
#define TOUCH_TIMEOUT_MS 100
typedef struct { double x, y; } Pair;
typedef struct { unsigned char bytes[MSG_LENGTH]; size_t used; } TouchParser;
/* Feed one byte; returns 1 only for a complete valid legacy frame. */
int touch_parser_feed(TouchParser *parser, unsigned char byte, Pair *point);
extern int calibrating;
extern int pressing;
int init_uart(void);
void close_uart(void);
/* Blocks for a frame; returns 0 on success, -1 on an I/O error. */
int getxy(Pair *point);
#endif
