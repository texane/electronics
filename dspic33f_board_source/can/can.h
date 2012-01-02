#ifndef CAN_H_INCLUDED
# define CAN_H_INCLUDED


#include "../common/inttypes.h"


#define CAN_MODE_NORMAL 0
#define CAN_MODE_DISABLE 1
#define CAN_MODE_LOOPBACK 2
#define CAN_MODE_LISTEN 3
#define CAN_MODE_CONFIG 4
#define CAN_MODE_LISTEN_ALL 7

#define CAN_DATA_SIZE 8

int can_setup(uint16_t, uint8_t);
int can_read(uint8_t*);
int can_read_addr(uint16_t*, uint8_t*);
int can_write(uint8_t*);
uint16_t can_get_errors(void);
void can_flush_input(void);
uint16_t can_get_rx_errors(void);
unsigned int can_is_bus_off(void);


#endif /* ! CAN_H_INCLUDED */
