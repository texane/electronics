#include <p33Fxxxx.h>
#include "uart.h"
#include "../osc/osc.h"

#define BAUDRATE 38400
#define BRGVAL ((OSC_FCY / (16 * BAUDRATE)) - 1)

char buff_uart[BUFF_SIZE];
char is_buffer_ready;
int buff_index;

static void send(char c)
{
	unsigned int j;
	
	while (U1STAbits.UTXBF);
	for(j = 0; j < 3000; j++);
	U1TXREG = c;
}

void uart_sendstr(char *s)
{
	while (*s) {
		send(*s);
		s++;
	}	
}

void uart_send(char c)
{
  send(c);
}

#if 1 /* dspic33f_board */

#define RXPIN 15
#define RXTRIS TRISBbits.TRISB15
#define TXTRIS TRISBbits.TRISB14
#define TXPIN RPOR7bits.RP14R

#elif 0 /* CARTE_2010 */

#define RXPIN 5
#define RXTRIS TRISBbits.TRISB5
#define TXTRIS TRISBbits.TRISB4
#define TXPIN RPOR2bits.RP4R

#elif 0 /* CARTE_2009 */

#define RXPIN 3
#define RXTRIS TRISBbits.TRISB3
#define TXPIN RPOR1bits.RP2R

#endif

//initialisation de l'UART
void uart_init(void)
{
  /* io UART port */
  AD1PCFGL=0xFFFF;

		/* Uart */
	RPINR18bits.U1RXR = RXPIN; /* RX */
	RXTRIS = 1;
/* 	TXTRIS = 0; */
	TXPIN = 3; /*  TX */
	
	U1MODEbits.STSEL = 0;// 1-stop bit
	U1MODEbits.PDSEL = 0;// No Parity, 8-data bits
	U1MODEbits.ABAUD = 0;// Auto-Baud Disabled
	U1MODEbits.BRGH = 0;// Low Speed mode
	U1BRG = BRGVAL;  // BAUD Rate Setting for 9600
	
	U1MODEbits.UARTEN = 1;
	U1STAbits.UTXEN = 1;
	
	//initialisation des interruptions en reception
	IFS0bits.U1RXIF = 0;      // Reset U1RXIF interrupt flag
	IPC2bits.U1RXIP = 3;      // U1RXIF Interrupt priority level=3
 	IEC0bits.U1RXIE = 1;      // Enable U1RX interrupt
	
	is_buffer_ready=0;
	buff_index=0;
	int j;
	for(j = 0; j < 3000; j++);
}

//*********************//
//**interruption sur U1RX ***//		
void __attribute__((__interrupt__, no_auto_psv)) _U1RXInterrupt(void)
{
    // Interrupt Service Routine code goes here */
    IFS0bits.U1RXIF=0;
    while(U1STAbits.URXDA)// tant que le buffer est plein
    {
    	char read=U1RXREG;
    	
    	buff_uart[buff_index]=read;
    	buff_index++;
    	if((buff_index > 0) && ((read=='\n') || (read=='\r')))
    	{
	    		buff_uart[buff_index-1]=0;
	    		is_buffer_ready=1;
	    		buff_index=0;
	    }
    }
    
    
    return;
}
