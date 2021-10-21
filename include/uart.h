#ifndef _UART_H
#define _UART_H

#include <avr/io.h>
#include <stdio.h>

void uart_init(void);
int uart_putchar(char c, FILE *stream);

#endif /* UART_H */
