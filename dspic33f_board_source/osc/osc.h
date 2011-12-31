#ifndef OSC_H_INCLUDED
# define OSC_H_INCLUDED

#if 0 /* external crystal used */
#define OSC_FOSC 25000000
#else /* frc */
#define OSC_FOSC 79227500
#endif

#define OSC_FCY (OSC_FOSC / 2)

void osc_setup(void);

#endif /* OSC_H_INCLUDED */
