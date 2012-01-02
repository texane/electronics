/* can communication unit test.
   notes:
   . osc setup is changed in this code as compared with the aversive project. take
   care of modifying what has to be before trying to integrate the code in aversive.
 */


#include <p33Fxxxx.h>
#include "common/inttypes.h"
#include "uart/uart.h"
#include "ecan/ecan.h"
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


/* rx or tx pinpong side */
#define CONFIG_CAN_TX 0

static void send_can_message(mID* m)
{
  /* configure and send a message */
  m->message_type = CAN_MSG_DATA;
  /* m->message_type=CAN_MSG_RTR; */
  /* m->frame_type = CAN_FRAME_EXT; */
  m->frame_type=CAN_FRAME_STD;
  m->buffer = 0;
  m->id = 0x123;
  m->data[0] = 'd';
  m->data[1] = 'e';
  m->data[2] = 'a';
  m->data[3] = 'd';
  m->data[4] = 'b';
  m->data[5] = 'e';
  m->data[6] = 'e';
  m->data[7] = 'f';
  m->data_length = 8;
		
  /* send a CAN message */
  sendECAN(m);
}

#if CONFIG_CAN_TX

int process_message_user(mID* q, mID* r)
{
  /* dont process any message */
  return 0;
}

#else

int process_message_user(mID* q, mID* r)
{
  /* tell message has been processed
     q the query
     m the reply
   */

  static unsigned int i;
  for (i = 0; i < 8; ++i) uart_send(q->data[i]);

#if 0 /* if a reply is needed */
  /* prepare the reply frame */
  r->message_type = CAN_MSG_DATA;
  r->frame_type = CAN_FRAME_STD;
  r->buffer = 0;
  r->id = 0x123;
  r->data_length = 8;
  r->data[0] = buf[0];
  r->data[1] = buf[1];
  *(uint16_t*)(r->data + 2 + 0 * sizeof(uint16_t)) = i2c_val;
  *(uint16_t*)(r->data + 2 + 1 * sizeof(uint16_t)) = i2c_val2;
  *(uint16_t*)(r->data + 2 + 2 * sizeof(uint16_t)) = i2c_val3;
#endif

  return 0;
}

#endif

extern mID* getECANTxMessage(void);

static inline void do_can_test(void)
{
  led_init();
  led_enable();

#if (CONFIG_CAN_TX == 0)
  uart_init();
#endif

  ecan_init();

  while (1)
  {
    /* can message send loop */
#if CONFIG_CAN_TX
    led_enable();
    send_can_message(getECANTxMessage());
    wait();
    led_disable();
    send_can_message(getECANTxMessage());
    wait();
#else
    if (C1RXFUL1bits.RXFUL1)
    {
      uart_send('1');
      C1RXFUL1bits.RXFUL1 = 0;
    }
    if (C1RXFUL1bits.RXFUL2)
    {
      uart_send('2');
      C1RXFUL1bits.RXFUL2 = 0;
    }
    if (C1RXFUL1bits.RXFUL3)
    {
      uart_send('3');
      C1RXFUL1bits.RXFUL3 = 0;
    }
    uart_send('*');
    wait();
#endif
  }
}

static inline void do_uart_test(void)
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

static inline void do_led_test(void)
{
  led_init();
  led_enable();

  while (1)
  {
    led_enable();
    wait();

    led_disable();
    wait();
  }
}

/* main
 */

int main(int ac, char** av)
{
  osc_setup();
  /* do_uart_test(); */
  do_can_test();
  /* do_led_test(); */
  while (1) ;
  return 0;
}

void __attribute__((__interrupt__, no_auto_psv)) _U1TXInterrupt(void)
{
  IFS0bits.U1TXIF = 0;
}
