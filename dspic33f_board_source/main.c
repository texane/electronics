/* can communication unit test.
   notes:
   . osc setup is changed in this code as compared with the aversive project. take
   care of modifying what has to be before trying to integrate the code in aversive.
 */


#include <p33Fxxxx.h>
#include "common/inttypes.h"
#include "uart/uart.h"
#include "osc/osc.h"
#include "delay/delay.h"


/* internal registers
 */

#if 1
_FOSCSEL(FNOSC_FRCPLL)
_FOSC(FCKSM_CSECMD & IOL1WAY_OFF & OSCIOFNC_ON & POSCMD_NONE)
_FWDT(FWDTEN_OFF);
/* _FBS(BWRP_WRPROTECT_OFF); */
/* _FGS(GSS_OFF & GCP_OFF & GWRP_OFF); */
/* _FPOR(FPWRT_PWR1); */
#else
_FOSCSEL(FNOSC_FRC) 
_FOSC(FCKSM_CSECMD & POSCMD_XT)
_FWDT(FWDTEN_OFF)
_FPOR(FPWRT_PWR1)
_FICD(JTAGEN_OFF & ICS_PGD1)
#endif

#if 0 /* ecan tx */

#include "ecan/ecan.h"

static void do_test(void)
{
  uart_init();

  ecan_init();

  /* configure and send a message */
  canTxMessage.message_type=CAN_MSG_DATA;
  //canTxMessage.message_type=CAN_MSG_RTR;
  canTxMessage.frame_type=CAN_FRAME_EXT;
  //canTxMessage.frame_type=CAN_FRAME_STD;
  canTxMessage.buffer=0;
  canTxMessage.id=0x123;
  canTxMessage.data[0]=0x55;
  canTxMessage.data[1]=0x55;
  canTxMessage.data[2]=0x55;
  canTxMessage.data[3]=0x55;
  canTxMessage.data[4]=0x55;
  canTxMessage.data[5]=0x55;
  canTxMessage.data[6]=0x55;
  canTxMessage.data[7]=0x55;
  canTxMessage.data_length=8;

  /* Delay for a second */
  Delay(Delay_1S_Cnt);
		
  /* send a CAN message */
  sendECAN(&canTxMessage);

  while(1)
  {
    /* check to see when a message is received and move the message 
       into RAM and parse the message */ 
    if (canRxMessage.buffer_status==CAN_BUF_FULL)
    {
      rxECAN(&canRxMessage);
      /* reset the flag when done */
      canRxMessage.buffer_status=CAN_BUF_EMPTY;
    }
    else ;
    /*
      {
      // delay for one second 
      Delay(1);
      // send another message 
      canTxMessage.id++;
      sendECAN(&canTxMessage);
      }
    */
  }
}

#elif 0 /* ecan, pingpong */

#include "ecan/ecan.h"

#if 0 /* UNUSED led routines */

static void led_init(void)
{
  AD1PCFGL=0xFFFF;

  TRISBbits.TRISB8 = 0;
  PORTBbits.RB8 = 0;

  TRISBbits.TRISB9 = 0;
  PORTBbits.RB9 = 0;
}

static void led_enable(unsigned int i)
{
  if (i == 0)
    PORTBbits.RB8 = 1;
  else
    PORTBbits.RB9 = 1;
}

static void led_disable(unsigned int i)
{
  if (i == 0)
    PORTBbits.RB8 = 0;
  else
    PORTBbits.RB9 = 0;
}

#endif /* UNUSED led routines */

static void do_test(void)
{
  ecan_init();

#if 0
  /* configure and send a message */
#if 0
  /* canTxMessage.message_type=CAN_MSG_RTR; */
  /* canTxMessage.frame_type=CAN_FRAME_EXT; */
#endif
  canTxMessage.message_type=CAN_MSG_DATA;
  canTxMessage.frame_type=CAN_FRAME_STD;
  canTxMessage.buffer=0;
  canTxMessage.id=0x123;
  canTxMessage.data[0]=0x55;
  canTxMessage.data[1]=0x55;
  canTxMessage.data[2]=0x55;
  canTxMessage.data[3]=0x55;
  canTxMessage.data[4]=0x55;
  canTxMessage.data[5]=0x55;
  canTxMessage.data[6]=0x55;
  canTxMessage.data[7]=0x55;
  canTxMessage.data_length=8;

  /* Delay for a second */
  Delay(Delay_1S_Cnt);

  /* send a CAN message */
  sendECAN(&canTxMessage);
#endif

#if 0
  uart_init();
  while (1)
  {
    volatile unsigned int i;
    for (i = 0; i < 5000; ++i) ;
    uart_send('c');
  }
#endif

  while (1) ;
}

#elif 0 /* can tx */

#include "can/can.h"

static void do_test(void)
{
  uint8_t buf[CAN_DATA_SIZE];

  unsigned int i;
  for (i = 0; i < CAN_DATA_SIZE; ++i)
    buf[i] = 0x55;

#define LADDR 0x123
  can_setup(LADDR, CAN_MODE_NORMAL);

  Delay(Delay_1S_Cnt);

  while (1)
  {
    volatile unsigned int i, j;
    for (j = 0; j < 1000; ++j)
      for (i = 0; i < 10000; ++i) ;
    can_write(buf);
  }
}


/* Interrupt Service Routine 1                      */
/* No fast context save, and no variables stacked   */
void __attribute__((interrupt,no_auto_psv))_C1Interrupt(void)  
{
  /* check to see if the interrupt is caused by receive */     	 
  if(C1INTFbits.RBIF)
  {
    /* check to see if buffer 1 is full */
    if(C1RXFUL1bits.RXFUL1)
    {
      C1RXFUL1bits.RXFUL1 = 0;
    }		
    /* check to see if buffer 2 is full */
    else if(C1RXFUL1bits.RXFUL2)
    {
      C1RXFUL1bits.RXFUL2 = 0;
    }
    /* check to see if buffer 3 is full */
    else if(C1RXFUL1bits.RXFUL3)
    {
      C1RXFUL1bits.RXFUL3 = 0;
    }

    C1INTFbits.RBIF = 0;
  }
  else if(C1INTFbits.TBIF)
  {
    /* clear flag */
    C1INTFbits.TBIF = 0;	    
  }
	
  /* clear interrupt flag */
  IFS2bits.C1IF=0;
}

#elif 0 /* pin io testing */

static void do_test(void)
{
  /* pin io testing */

  /* io UART port */
  AD1PCFGL=0xFFFF;

  TRISBbits.TRISB5 = 0;
  TRISBbits.TRISB6 = 0;
  TRISBbits.TRISB8 = 0;
  TRISBbits.TRISB9 = 0;

  LATBbits.LATB5 = 0;
  LATBbits.LATB6 = 1;

  LATBbits.LATB8 = 1;
  LATBbits.LATB9 = 0;

  while (1)
  {
    volatile unsigned int i;
    for (i = 0; i < 10000; ++i) ;
    LATBbits.LATB5 ^= 1;
    LATBbits.LATB6 ^= 1;
  }
}

void __attribute__((interrupt, no_auto_psv)) _C1Interrupt(void)  
{
  if (C1INTFbits.RBIF)
  {
#if 0 /* todo */
    /* check to see if buffer 1 is full */
    if (C1RXFUL1bits.RXFUL1)
    {
      /* set the buffer full flag and the buffer received flag */
      canRxMessage.buffer_status = CAN_BUF_FULL;
      canRxMessage.buffer = 1;
    }
    /* check to see if buffer 2 is full */
    else if (C1RXFUL1bits.RXFUL2)
    {
      /* set the buffer full flag and the buffer received flag */
      canRxMessage.buffer_status = CAN_BUF_FULL;
      canRxMessage.buffer = 2;
    }
    /* check to see if buffer 3 is full */
    else if (C1RXFUL1bits.RXFUL3)
    {
      /* set the buffer full flag and the buffer received flag */
      canRxMessage.buffer_status = CAN_BUF_FULL;
      canRxMessage.buffer = 3;			
    }
#else
    if (C1RXFUL1bits.RXFUL1)
      C1RXFUL1bits.RXFUL1 = 0;
    else if (C1RXFUL1bits.RXFUL2)
      C1RXFUL1bits.RXFUL2 = 0;
    else if(C1RXFUL1bits.RXFUL3)
      C1RXFUL1bits.RXFUL3 = 0;
#endif /* todo */

    C1INTFbits.RBIF = 0;
  }
  else if (C1INTFbits.TBIF)
  { 
    C1INTFbits.TBIF = 0;
  }

  IFS2bits.C1IF = 0;
}

#elif 0 /* fosc test */

#define wait()					\
do {						\
  __asm__ ("nop");				\
  __asm__ ("nop");				\
  __asm__ ("nop");				\
  __asm__ ("nop");				\
} while (0)

static void do_test(void)
{
  AD1PCFGL=0xFFFF;

  TRISBbits.TRISB5 = 0;

 redo:
  {
    LATBbits.LATB5 = 1;
    wait();
    LATBbits.LATB5 = 0;
    wait();

    goto redo;
  }
}

#elif 1 /* blink ra0 */

static inline void led_init(void)
{
  AD1PCFGL=0xFFFF;
  TRISAbits.TRISA0 = 0;
  TRISAbits.TRISA1 = 0;
  TRISBbits.TRISB0 = 0;
  TRISBbits.TRISB13 = 0;
}

static inline void led_enable(void)
{
  LATAbits.LATA0 = 1;
  LATAbits.LATA1 = 1;
  LATBbits.LATB0 = 1;
  LATBbits.LATB13 = 1;
}

static inline void led_disable(void)
{
  LATAbits.LATA0 = 0;
  LATAbits.LATA1 = 0;
  LATBbits.LATB0 = 0;
  LATBbits.LATB13 = 0;
}

static inline void wait(void)
{
  static volatile unsigned int i;
  static volatile unsigned int j;

  for (j = 0; j < 100; ++j)
    for (i = 0; i < 60000; ++i)
      __asm__ ("nop");
}

static inline void do_test(void)
{
  led_init();
  led_enable();

  uart_init();

  while (1)
  {
    led_enable();
    uart_send('x');
    wait();

    led_disable();
    uart_send('o');
    wait();
  }
}

#endif


/* main
 */

int main(int ac, char** av)
{
  /* disable all interrupts */
  _IPL |= 7;

  osc_setup();
  do_test();
  while (1) ;
  return 0;
}
