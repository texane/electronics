#include <p33Fxxxx.h>

/* osc configuration
 */

/* output frequence eq. 8-1 from dspic33f manual
   Fosc = Fin * M / (N1 * N2) or
   Fosc = Fin * (PLLDIV + 2) / ((PLLPRE + 2) * (PLLPOST + 2))
   notes that:
   PLLDIV is PLLFBD
   eq. 8-1 says (PLLPOST + 1), but subsequent example say + 2
*/

int osc_setup(void)
{
#if 0 /* working configuration */

  PLLFBD = 30; /* M = 32 */
  CLKDIVbits.PLLPRE = 0; /* N1 = 2 */
  CLKDIVbits.PLLPOST = 0; /* N2 = 2 */

  /* Initiate Clock Switch to FRC oscillator with PLL (NOSC=0b001) */
  OSCTUN = 0; /* Tune FRC oscillator, if FRC is used */
  RCONbits.SWDTEN = 0; /* Disable Watch Dog Timer */

  /* Clock switch to incorporate PLL */
  __builtin_write_OSCCONH(0x03); /* Initiate Clock Switch to primary */
  __builtin_write_OSCCONL(0x01); /* Start clock switching */
  while(OSCCONbits.COSC != 3); /* Wait for Clock switch to occur */
  while(OSCCONbits.LOCK != 1); /* Wait for PLL to lock  switch */

#else /* fast rc oscillator, from aversive source code */

  PLLFBD = 41;
  CLKDIVbits.PLLPOST = 0;
  CLKDIVbits.PLLPRE = 0;

  OSCTUN = 0;
  RCONbits.SWDTEN = 0;

  __builtin_write_OSCCONH(0x01);
  __builtin_write_OSCCONL(0x01);
  while (OSCCONbits.COSC != 1) ;
  while (OSCCONbits.LOCK != 1) ;

#endif

  return 0;
}
