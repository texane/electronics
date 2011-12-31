#ifndef UART_IS_INCLUDED
#define UART_IS_INCLUDED

#define BUFF_SIZE 80

void uart_init(void);
void uart_sendstr(char*);
void uart_send(char);

#endif // UART_IS_INCLUDED
