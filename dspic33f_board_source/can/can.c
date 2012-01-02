/* #if defined(__dsPIC33FJ128MC804__) */
#include <p33Fxxxx.h>
#include "can.h"
#include "../osc/osc.h"
#include "../common/inttypes.h" 


/* notes on dma
   . refer to  figure 4.5 for the data memory map.
   . in brief dma ram spans [0x4000 - 0x4800[
   that is 2k.
   . can buffers are part of dma memory, not special
   purpose registers.
 */

#define ECAN_BUFFER_SIZE (8 * 2) /* 8 x 16 bits words */
#define ECAN_BUFFER_COUNT 4

#if 1
uint16_t dma_tx_buffer[8] __attribute__((space(dma), aligned(16)));
uint16_t dma_tx_buffer2[8] __attribute__((space(dma), aligned(16)));
uint16_t dma_rx_buffer[8] __attribute__((space(dma), aligned(16)));
uint16_t dma_rx_buffer2[8] __attribute__((space(dma), aligned(16)));
#define ecan1msgBuf dma_tx_buffer
#else /* packed */
typedef unsigned int ECAN1MSGBUF [4][8];
extern ECAN1MSGBUF  ecan1msgBuf __attribute__((space(dma)));
ECAN1MSGBUF ecan1msgBuf __attribute__((space(dma), aligned(4 * 16)));
#define dma_tx_buffer ecan1msgBuf[0]
#endif

/* local sid */
static uint16_t can_sid;

static int switch_mode(uint8_t mode)
{
  C1CTRL1bits.REQOP = mode;
  while (C1CTRL1bits.OPMODE != mode) ;
  return 0;
}

static void setup_dma(void)
{
  /* setup dma area with dma_buffers */

  /* setup the ecan tx dma channel */
  DMACS0 = 0; /* clear collision flags */
  DMA0CON = 0x2020; /* indirect addressing, normal word operation */
  DMA0PAD = (unsigned int)&C1TXD; /* set the C1TXD periph addr for channel 0 */
  DMA0CNT = 7; /* transfert size == 8 words */
  DMA0REQ = 0x0046; /* automatic tx init by dma request */
  DMA0STA = __builtin_dmaoffset(&ecan1msgBuf); /* offset in the dma space */
  DMA0CONbits.CHEN = 1; /* enable the channel */

  /* setup the ecan rx dma channel */
  DMACS0 = 0;
  DMA2CON = 0x0020; /* select rx to peripheral */
  DMA2PAD = (unsigned int)&C1RXD; /* set the C1RXD periph addr for channel 0 */
  DMA2CNT = 7;
  DMA2REQ = 0x0022;
  DMA2STA = __builtin_dmaoffset(&ecan1msgBuf);
  DMA1CONbits.CHEN = 1;
}

static void setup_filters(void)
{
  /* filter and mask defines */

  /* Macro used to write filter/mask ID to Register CiRXMxSID and 
     CiRXFxSID. For example to setup the filter to accept a value of 
     0x123, the macro when called as CAN_FILTERMASK2REG_SID(0x123) will 
     write the register space to accept message with ID ox123 
     USE FOR STANDARD MESSAGES ONLY */
#define CAN_FILTERMASK2REG_SID(x) ((x & 0x07FF)<< 5)

  /* the Macro will set the "MIDE" bit in CiRXMxSID */
#define CAN_SETMIDE(sid) (sid | 0x0008)

  /* the macro will set the EXIDE bit in the CiRXFxSID to 
     accept extended messages only */
#define CAN_FILTERXTD(sid) (sid | 0x0008)

  /* the macro will clear the EXIDE bit in the CiRXFxSID to 
     accept standard messages only */
#define CAN_FILTERSTD(sid) (sid & 0xFFF7)

  /* Macro used to write filter/mask ID to Register CiRXMxSID, CiRXMxEID and 
     CiRXFxSID, CiRXFxEID. For example to setup the filter to accept a value of 
     0x123, the macro when called as CAN_FILTERMASK2REG_SID(0x123) will 
     write the register space to accept message with ID 0x123 
     USE FOR EXTENDED MESSAGES ONLY */
#define CAN_FILTERMASK2REG_EID0(x) (x & 0xFFFF)
#define CAN_FILTERMASK2REG_EID1(x) (((x & 0x1FFC)<< 3)|(x & 0x3))

  /* Filter configuration */
  /* enable window to access the filter configuration registers */
  C1CTRL1bits.WIN = 1;

  /* select acceptance mask 0 filter 0 buffer 1 */
  C1FMSKSEL1bits.F0MSK = 0;

  /* configure accpetence mask 0 - match the id in filter 0 
     setup the mask to check every bit of the standard message, 
     the macro when called as CAN_FILTERMASK2REG_SID(0x7FF) will 
     write the register C1RXM0SID to include every bit in filter comparison 
  */ 	
  C1RXM0SID=CAN_FILTERMASK2REG_SID(0x7FF);

  /* configure accpetence filter 0 
     setup the filter to accept a standard id of 0x123, 
     the macro when called as CAN_FILTERMASK2REG_SID(0x123) will 
     write the register C1RXF0SID to accept only standard id of 0x123 	
  */ 	
  C1RXF0SID = CAN_FILTERMASK2REG_SID(0x123);

  /* set filter to check for standard ID and accept standard id only */
  C1RXM0SID=CAN_SETMIDE(C1RXM0SID);
  C1RXF0SID=CAN_FILTERSTD(C1RXF0SID);

  /* acceptance filter to use buffer 1 for incoming messages */
  C1BUFPNT1bits.F0BP = 1;

  /* enable filter 0 */
  C1FEN1bits.FLTEN0 = 1;
	
  /* select acceptance mask 1 filter 1 and buffer 2 */
  C1FMSKSEL1bits.F1MSK = 1;

  /* configure accpetence mask 1 - match id in filter 1 	
     setup the mask to check every bit of the extended message, 
     the macro when called as CAN_FILTERMASK2REG_EID0(0xFFFF) 
     will write the register C1RXM1EID to include extended 
     message id bits EID0 to EID15 in filter comparison. 
     the macro when called as CAN_FILTERMASK2REG_EID1(0x1FFF) 
     will write the register C1RXM1SID to include extended 
     message id bits EID16 to EID28 in filter comparison. 	
  */ 			
  C1RXM1EID = CAN_FILTERMASK2REG_EID0(0xFFFF);
  C1RXM1SID = CAN_FILTERMASK2REG_EID1(0x1FFF);

  /* configure acceptance filter 1 
     configure accpetence filter 1 - accept only XTD ID 0x12345678 
     setup the filter to accept only extended message 0x12345678, 
     the macro when called as CAN_FILTERMASK2REG_EID0(0x5678) 
     will write the register C1RXF1EID to include extended 
     message id bits EID0 to EID15 when doing filter comparison. 
     the macro when called as CAN_FILTERMASK2REG_EID1(0x1234) 
     will write the register C1RXF1SID to include extended 
     message id bits EID16 to EID28 when doing filter comparison. 	
  */ 
  C1RXF1EID = CAN_FILTERMASK2REG_EID0(0x5678);
  C1RXF1SID = CAN_FILTERMASK2REG_EID1(0x1234);

  /* filter to check for extended ID only */
  C1RXM1SID = CAN_SETMIDE(C1RXM1SID);
  C1RXF1SID = CAN_FILTERXTD(C1RXF1SID);

  /* acceptance filter to use buffer 2 for incoming messages */
  C1BUFPNT1bits.F1BP = 0x2;
  /* enable filter 1 */
  C1FEN1bits.FLTEN1 = 1;
	
  /* select acceptance mask 1 filter 2 and buffer 3 */
  C1FMSKSEL1bits.F2MSK = 0x1;

  /* configure acceptance filter 2 
     configure accpetence filter 2 - accept only XTD ID 0x12345679 
     setup the filter to accept only extended message 0x12345679, 
     the macro when called as CAN_FILTERMASK2REG_EID0(0x5679) 
     will write the register C1RXF1EID to include extended 
     message id bits EID0 to EID15 when doing filter comparison. 
     the macro when called as CAN_FILTERMASK2REG_EID1(0x1234) 
     will write the register C1RXF1SID to include extended 
     message id bits EID16 to EID28 when doing filter comparison. 	
  */ 
  C1RXF2EID = CAN_FILTERMASK2REG_EID0(0x5679);
  C1RXF2SID = CAN_FILTERMASK2REG_EID1(0x1234);

  /* filter to check for extended ID only */	
  C1RXF2SID = CAN_FILTERXTD(C1RXF2SID);

  /* acceptance filter to use buffer 3 for incoming messages */
  C1BUFPNT1bits.F2BP = 0x3;

  /* enable filter 2 */
  C1FEN1bits.FLTEN2 = 1;
	         
  /* clear window bit to access ECAN control registers */
  C1CTRL1bits.WIN = 0;
}

static void remap_ecan_pins(void)
{
  /* remap the can registers to  */

  RPINR26bits.C1RXR = 6; /* c1rx mapped to rb6 */
  RPOR3bits.RP7R = 0x10; /* c1tx mapped to rb7. refer to table 11-2. */

  /* testing */
  TRISBbits.TRISB6 = 1;
  TRISBbits.TRISB7 = 0;
}

int can_setup(uint16_t sid, uint8_t mode)
{
  /* addr the node address
     mode the CAN_MODE_xxx mode
   */

  /* io mode pins */
  AD1PCFGL = 0xffff;

  remap_ecan_pins();

  if (switch_mode(CAN_MODE_CONFIG) == -1)
    return -1;

#if 1
  /* datasheet says to not use this bit. but sapmles use it. sources from
     microchip confirmed it is a dont care bit and should not be used.
     at the end, we use it since the only working code seems to use it.
   */
  C1CTRL1bits.CANCKS = 1;
#endif

  /* baud rate and segment time configuration.
     . segment times:
     .. Tsync = 2 * Tq;
     .. TPropagation = 7 * Tq;
     .. TPhase1 = 8 * Tq;
     .. TPhase2 = 8 * Tq;
     . baudrate:
     .. according to doc, Tq = ((BRP + 1) * 2) / Fcan
     .. we have Tbit = NTQ * Tq, thus: Tbit = NTQ * ((BRP + 1) * 2) / Fcan
     .. thus BRP = Fcan / (NTq * 2 * BITRATE) - 1;
  */


/* warning: setting a value above 10 Nq at 125000 bps overflows
   in the above formula if Fcan == Fcy. recheck computation before
   modifying.
 */

#define NSync 2
#define NProp 2
#define NPhase1 3
#define NPhase2 3
#define Nq (NSync + NProp + NPhase1 + NPhase2)

#define BITRATE 125000

#define Fosc OSC_FOSC
#define Fcy OSC_FCY
#define Fcan (Fcy)

#define BRP_VAL ((Fcan / (Nq * 2 * BITRATE)) - 1)

  /* baud rate */
  C1CFG1bits.BRP = BRP_VAL;
  C1CFG1bits.SJW = NSync - 1;

  /* segment times */
  C1CFG2bits.PRSEG = NProp - 1;
  C1CFG2bits.SEG1PH = NPhase1 - 1;
  C1CFG2bits.SEG2PHTS = 1; /* phase2 segment time is programmable */
  C1CFG2bits.SEG2PH = NPhase2 - 1;
  C1CFG2bits.SAM = 1; /* bus line sampled 3 time at sample point */

  C1FCTRLbits.DMABS = 0; /* 4 buffers in dma ram */

  setup_filters();

  /* put the module in normal mode */
  if (switch_mode(mode) == -1)
    return -1;

  /* saved node sid */
  can_sid = sid;

  /* clear the buffer and overflow flags */
  C1RXFUL1 = 0;
  C1RXFUL2 = 0;
  C1RXOVF1 = 0;
  C1RXOVF2 = 0;

  /* ECAN1, Buffer 0 is a Transmit Buffer */
  C1TR01CONbits.TXEN0 = 1;

  /* ECAN1, Buffer 1 is a Receive Buffer */
  C1TR01CONbits.TXEN1 = 0;

  /* ECAN1, Buffer 2 is a Receive Buffer */
  C1TR23CONbits.TXEN2 = 0;

  /* ECAN1, Buffer 3 is a Receive Buffer */
  C1TR23CONbits.TXEN3 = 0;

  /* Message Buffer 0 Priority Level */
  C1TR01CONbits.TX0PRI = 3;
		
  /* configure the device to interrupt on the receive buffer full flag */
  /* clear the buffer full flags */
  C1RXFUL1 = 0;
  C1INTFbits.RBIF = 0;

  setup_dma();

  /* enable interrupts */
  IEC2bits.C1IE = 1;
  C1INTEbits.TBIE = 1;	
  C1INTEbits.RBIE = 1;

  return 0;
}

int can_read_addr(uint16_t* addr, uint8_t* buffer)
{
#if 0 /* todo */
  uint8_t* const dma_buffer = dma_rx_buffer;
  uint8_t prev_error = C1ECbits.RERRCNT;
  unsigned int i;

  /* wait for one of the receive buffer to be full */
  while (C1RXFUL1bits.RXFUL2 == 0)
  {
    if ((C1ECbits.RERRCNT) != prev_error)
      return -1;
  }

  /* todo: handle errors */

  /* sid */
  if (addr != 0)
    *addr = ((uint16_t)dma_buffer[0] << 6) | ((uint16_t)dma_buffer[1] >> 2);

  for (i = 0; i < CAN_DATA_SIZE; ++i)
    buffer[i] = dma_buffer[6 + i];

  C1RXFUL1bits.RXFUL0 = 0;
#endif

  return 0;
}

int can_read(uint8_t* buffer)
{
  return can_read_addr(0, buffer);
}

int can_write(uint8_t* buffer)
{
#if 0
  /* sid */
  dma_tx_buffer[0] = can_sid << 2;

  /* eid, unused */
  dma_tx_buffer[1] = 0;

  /* dlc */
  dma_tx_buffer[2] = 8; /* in bytes */
#else
  /* get the extended message id EID28..18*/
  /* set the SRR and IDE bit */
  dma_tx_buffer[0] = ((can_sid & 0x1ffc0000) >> 16) | 0x0003;

  /* the the value of EID17..6 */
  dma_tx_buffer[1] = (can_sid & 0x0003ffc0) >> 6;

  /* get the value of EID5..0 for word 2 and length (ie. 8 bytes) */
  dma_tx_buffer[2] = ((can_sid & 0x0000003f) << 10) | 8;
#endif

  /* data */
  dma_tx_buffer[3] = 0x55;
  dma_tx_buffer[4] = 0x55;
  dma_tx_buffer[5] = 0x2a;
  dma_tx_buffer[6] = 0x55;

  C1TR01CONbits.TXREQ0 = 1;
#if 0
  while (C1TR01CONbits.TXREQ0)
  {
    if (C1TR01CONbits.TXERR0 || C1TR01CONbits.TXLARB0)
    {
      /* abort transmission */
      C1TR01CONbits.TXREQ0 = 0;
      return -1;
    }
  }
#endif

  return 0;
}

uint16_t can_get_errors(void)
{
 return C1EC;
}


uint16_t can_get_rx_errors(void)
{
#if 0 /* todo */
  uint16_t saved_errors = C1INTF;
  C1INTF = 0;
  return saved_errors;
#else
  return 0;
#endif
}

void can_flush_input(void)
{
#if 0 /* todo */
  uint8_t buf[CAN_DATA_SIZE];
  while (C1RX0CONbits.RXFUL)
    can_read(buf);
#endif
}

unsigned int can_is_bus_off(void)
{
#if 0 /* todo */
  return C1INTFbits.TXBO;
#else
  return 0;
#endif
}

unsigned int can_is_rx_passive(void)
{
#if 0 /* todo */
  /* return non zero if rx error passive state */
  return C1INTFbits.RXEP;
#else
  return 0;
#endif
}

unsigned int can_is_tx_passive(void)
{
#if 0 /* todo */
  /* return non zero if tx error passive state */
  return C1INTFbits.TXEP;
#else
  return 0;
#endif
}
