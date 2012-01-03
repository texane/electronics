#include <p33Fxxxx.h>


/* configuration bits */

_FOSCSEL(FNOSC_FRCPLL);
_FOSC(FCKSM_CSECMD & IOL1WAY_OFF & OSCIOFNC_ON & POSCMD_NONE);
_FWDT(FWDTEN_OFF);
_FBS(BWRP_WRPROTECT_OFF);


/* int types */

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;


/* static configuration */

#define CONFIG_USE_ECAN 1

#if CONFIG_USE_ECAN
# define CONFIG_USE_UART 0
#else
# define CONFIG_USE_UART 1
#endif

#define CONFIG_BOOT_ID 2 

#define CONFIG_USE_LENDIAN 1


/* software reset value */
/* todo: put at a special location */

static uint16_t is_soft_reset = 0;


/* oscillator */

#define CONFIG_OSC_FOSC 79227500
#define OSC_FCY (CONFIG_OSC_FOSC / 2)

static void osc_setup(void)
{
  /* fast rc oscillator */

  PLLFBD = 41;
  CLKDIVbits.PLLPOST = 0;
  CLKDIVbits.PLLPRE = 0;

  OSCTUN = 0;
  RCONbits.SWDTEN = 0;

  __builtin_write_OSCCONH(0x01);
  __builtin_write_OSCCONL(0x01);
  while (OSCCONbits.COSC != 1) ;
  while (OSCCONbits.LOCK != 1) ;

  return 0;
}


/* self boot id */

static inline uint8_t get_boot_id(void)
{
#if defined(CONFIG_BOOT_ID)
  return CONFIG_BOOT_ID;
#else
  /* todo: read from dip switch */
  return 0;
#endif
}


/* goto user code entrypoint */

static inline void call_user_code(void)
{
  /* todo */
  while (1) ;
}


/* delay */

static inline void delay(void)
{
  /* todo */
}


/* endianness */

#if CONFIG_USE_LENDIAN

static inline uint16_t read_uint16(uint8_t* s)
{
  return *(uint16_t*)s;
}

static inline uint32_t read_uint32(uint8_t* s)
{
  return *(uint32_t*)s;
}

static inline void write_uint16(uint8_t* s, uint16_t x)
{
  *(uint16_t*)s = x;
}

static inline void write_uint32(uint8_t* s, uint32_t x)
{
  *(uint32_t*)s = x;
}

#else /* local is big endian */

static inline uint16_t read_uint16(uint8_t* s)
{
  /* todo */
}

static inline uint32_t read_uint32(uint8_t* s)
{
  /* todo */
}

static inline void write_uint16(uint8_t* s, uint16_t x)
{
  /* todo */
}

static inline void write_uint32(uint8_t* s, uint32_t x)
{
  /* todo */
}

#endif /* CONFIG_USE_LENDIAN */


/* get 16 bits parts from 32 bits value */

#define LO(__x) (((__x) >> 0) & 0xffff)
#define HI(__x) (((__x) >> 16) & 0xffff)


/* communication layer */

#define COM_FRAME_SIZE 8

#if CONFIG_USE_ECAN

static void ecan_setup(uint8_t id)
{
  /* todo: use id as a boot command filter */
}

static void ecan_write(uint8_t* s)
{
}

static void ecan_read(uint8_t* s)
{
}

#define com_setup ecan_setup
#define com_write ecan_write
#define com_read ecan_read

#else /* CONFIG_USE_UART */

static void uart_setup(uint8_t id)
{
#define CONFIG_UART_BAUDRATE 38400
#define BRGVAL ((OSC_FCY / (16 * CONFIG_UART_BAUDRATE)) - 1)

#define CONFIG_UART_RXPIN 15
#define CONFIG_UART_RXTRIS TRISBbits.TRISB15
#define CONFIG_UART_TXTRIS TRISBbits.TRISB14
#define CONFIG_UART_TXPIN RPOR7bits.RP14R

  AD1PCFGL = 0xFFFF;

  RPINR18bits.U1RXR = CONFIG_UART_RXPIN;
  CONFIG_UART_RXTRIS = 1;
  CONFIG_UART_TXTRIS = 0;
  CONFIG_UART_TXPIN = 3;

  U1MODEbits.STSEL = 0;
  U1MODEbits.PDSEL = 0;
  U1MODEbits.ABAUD = 0;
  U1MODEbits.BRGH = 0;
  U1BRG = BRGVAL;

  U1MODEbits.UARTEN = 1;
  U1STAbits.UTXEN = 1;

#if 0 /* interrupts disabled */
  IFS0bits.U1RXIF = 0;
  IPC2bits.U1RXIP = 3;
  IEC0bits.U1RXIE = 1;
#endif /* interrupts disabled */
}

static void uart_write(uint8_t* s)
{
  unsigned int i;

  for (i = 0; i < COM_FRAME_SIZE; ++i, ++s)
  {
    while (U1STAbits.UTXBF) ;
    U1TXREG = *s;
  }
}

static void uart_read(uint8_t* s)
{
  /* todo: check for errors */

  unsigned int i;

  for (i = 0; i < COM_FRAME_SIZE; ++i, ++s)
  {
    while (U1STAbits.URXDA == 0) ;
    *s = U1RXREG;
  }
}

#define com_setup uart_setup
#define com_write uart_write
#define com_read uart_read

#endif /* CONFIG_USE_UART */


/* flash programming routines */

static inline void erase_page(uint16_t addrhi, uint16_t addrlo)
{
#if 0
  asm("push TBLPAG");
#endif

  asm("mov #0x4042, W2");
  asm("mov W2, NVMCOM");

  /* the 24 bit target addr is (tblpag<7:0> << 16) + Wi */
  /* i specified in the table instruction */
  asm("mov W0, TBLPAG");
  asm("tblwtl W1, [W1]");

  /* disable interrupts during the following insn block */
  asm("disi #5");

  /* write the key */
  asm("mov #0x55, W0");
  asm("mov W0, NVMKEY");
  asm("mov #0xaa, W1");
  asm("mov W1, NVMKEY");

  /* start erase sequence */
  asm("bset NVMCOM, #15");

  /* see ref manual 5-1 example */
#if 1
  asm("nop");
  asm("nop");
#else
  asm("1: btsc NVMCON, #15");
  asm("bra 1b");
#endif

#if 0
  asm("pop TBLPAG");
#endif
}

static void read_mem
(uint16_t hiaddr, uint16_t loaddr, uint16_t fu, uint16_t bar)
{
  /* read a flash page at {hi,lo}addr */

  /* todo */
}

static inline void write_mem(void)
{
  /* write one program memory row */
  asm("mov #0x4001, W0");
  asm("mov W0, NVMCON");

  /* see erase_page */
  asm("disi #5");
  asm("mov #0x55, W0");
  asm("mov W0, NVMKEY");
  asm("mov #0xaa, W1");
  asm("mov W1, NVMKEY");
  asm("bset NVMCOM, #15");
  asm("nop");
  asm("nop");
}

static inline void load_latches
(uint16_t addrhi, uint16_t addrlo, uint16_t wordhi, uint16_t wordlo)
{
  /* w0 contains addrhi */
  /* w1 contains addrlo */
  /* w2 contains wordhi */
  /* w3 contains wordlo */

  asm("mov W0, TBLPAG");
  asm("tblwtl W3, [W1]");
  asm("tblwth W2, [W1]");
}


/* commands */

#define CMD_ID_WRITE_PROGRAM 0
#define CMD_ID_WRITE_CONFIG 1
#define CMD_ID_READ_PROGRAM 2
#define CMD_ID_READ_CONFIG 3
#define CMD_ID_STATUS 4
#define CMD_ID_GO 5

static void read_process_cmd(void)
{
#define ROW_INSN_COUNT 64
#define ROW_BYTE_COUNT (ROW_INSN_COUNT * 3)
#define PAGE_INSN_COUNT 512
#define PAGE_BYTE_COUNT (PAGE_INSN_COUNT * 3)

  /* 8 is added to avoid overflow in com_read */
  static uint8_t page_buf[PAGE_BYTE_COUNT + 8];

  uint8_t cmd_buf[8];
  uint32_t addr;
  uint16_t size;
  uint16_t off;
  uint16_t i;
  uint16_t j;

  com_read(cmd_buf);

  switch (read_uint8(cmd_buf + 0))
  {
  case CMD_ID_WRITE_PROGRAM:
    {
      /* write a program page */

      addr = read_uint32(cmd_buf + 1);
      size = read_uint16(cmd_buf + 5);

      /* todo: send command ack */

      /* read the page before erasing, if not a full page. */
      off = 0;
      if (size != PAGE_BYTE_COUNT)
      {
	off = addr % PAGE_BYTE_COUNT;
	read_page(HI(addr), LO(addr), HI(page_buf), LO(page_buf));
      }

      /* erase page */
      erase_page(HI(addr), LO(addr));

      /* receive the page */
      for (i = 0; i < size; i += COM_FRAME_SIZE)
      {
	com_read(page_buf + off + i);

	/* todo: send data ack */
      }

      /* write a whole page. i incremented by inner loop */
      for (i = 0; i < PAGE_BYTE_COUNT; )
      {
	/* fill the one row program memory buffer 3 bytes at a time */
	for (j = 0; j < (ROW_BYTE_COUNT / 3); i += 3, j += 3, addr += 3)
	{
	  const uint32_t tmp = buf[i];
	  load_latches(HI(addr), LO(addr), HI(tmp), LO(tmp));
	}

	/* write latches to memory */
	write_mem();
      }

      /* todo: send page programming ack */

      break ;
    }

  case CMD_ID_WRITE_CONFIG:
    {
      /* todo */

      break ;
    }

  case CMD_ID_READ_PROGRAM:
    {
      /* todo */

      break ;
    }

  case CMD_ID_READ_CONFIG:
    {
      /* todo */

      break ;
    }

  case CMD_ID_STATUS:
    {
      /* todo: reply status */

      break ;
    }

  case CMD_ID_GO:
    {
      /* jump to the entrypoint */

      call_user_code();

      break ;
    }

  default: break ;
  }
}


/* main */

int main(void)
{
  /* look for soft reset */
  if (is_soft_reset)
  {
    is_soft_reset = 0;
    call_user_code();
  }

  osc_setup();

  com_setup(get_boot_id());

  while (1) read_process_cmd();

  return 0;
}
