#include <avr/io.h>
#include <stdio.h>

#define BAUD 57600
#include <util/setbaud.h>

/* simple USART output, from avr-libc examples */

void uart_init(void) {
    UBRR0H = UBRRH_VALUE; /* get baud rate settings from setbaud.h */
    UBRR0L = UBRRL_VALUE;

#if USE_2X
    UCSR0A |= (1 << U2X0);
#else
    UCSR0A &= ~(1 << U2X0);
#endif

    UCSR0C = _BV(UCSZ01) | _BV(UCSZ00); /* 8N1 */
    UCSR0B = _BV(RXEN0) | _BV(TXEN0);   /* Enable RX (do I really need it here?) and TX */
}

int uart_putchar(char c, FILE *stream)
{
    if (c == '\n')
        uart_putchar('\r', stream);
    loop_until_bit_is_set(UCSR0A, UDRE0); /* empty buffer */
    UDR0 = c;                             /* trasmit */

    return 0;
}
