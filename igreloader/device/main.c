#include <p33Fxxxx.h>


/* configuration bits */

_FOSCSEL(FNOSC_FRCPLL)
_FOSC(FCKSM_CSECMD & IOL1WAY_OFF & OSCIOFNC_ON & POSCMD_NONE)
_FWDT(FWDTEN_OFF);


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
  /* todo */
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


/* nibbling */

#define LO(__x) (((__x) >> 0) & 0xffff)
#define HI(__x) (((__x) >> 16) & 0xffff)


/* communication layer */

#define COM_FRAME_SIZE 8

#if CONFIG_USE_ECAN

static void ecan_setup(uint8_t id)
{
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
/* dspic33f_bootloader/msstate/bootloader/24h_24f_target/mem.c */

static inline void write_mem(uint16_t val)
{
  asm("mov W0, NVMCON");
  __builtin_write_NVM();

  /* wait for write end */
  asm("1: btsc NVMCON, #15");
  asm("bra 1b");
}

static inline void load_addr(uint16_t nvmadru, uint16_t nvmadr)
{
  asm("mov W0, TBLPAG");
  asm("mov W1, W1");
}

static inline void load_latches
(uint16_t addrhi, uint16_t addrlo, uint16_t wordhi, uint16_t wordlo)
{
  asm("mov W0, TBLPAG");
  asm("tblwtl W3, [W1]");
  asm("tblwth W2, [W1]");
}

static inline uint32_t read_latch(uint16_t addrhi, uint16_t addrlo)
{
  asm("mov W0, TBLPAG");
  asm("tblrdl [W1], W0");
  asm("tblrdh [W1], W1");

}

static inline void erase_page(uint16_t addrhi, uint16_t addrlo, uint16_t val)
{
  asm("push TBLPAG");
  asm("mov W2, NVMCON");

  asm("mov W0, TBLPAG");
  asm("tblwtl W1, [W1]");

  __builtin_write_NVM();

  asm("1: btsc NVMCON, #15");
  asm("bra 1b");
  asm("pop TBLPAG");
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
  static uint8_t buf[PAGE_BYTE_COUNT];

  uint32_t addr;
  uint16_t off;

  com_read(buf);

  switch (read_uint8(buf + 0))
  {
  case CMD_ID_WRITE_PROGRAM:
    {
      addr = read_uint32(buf + 1);

      /* todo: ack */
      /* todo: receive */
      /* todo: ack */
      /* todo: write */

      /* receive the page */
      for (off = 0; off < PAGE_BYTE_COUNT; off += COM_FRAME_SIZE)
	com_read(buf + off);

      /* erase page */
      erase_page(HI(addr), LO(addr), );

      for (off = 0; off < PAGE_BYTE_COUNT; off += ROW_BYTE_COUNT, addr += ROW_BYTE_COUNT)
      {
	/* set latches */
	load_latches(HI(addr), LO(addr), , );

	/* write latches to memory */
	write_mem();
      }

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
