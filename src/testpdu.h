
#include <stdint.h>

extern uint8_t pdu_sync[8];		/* 60 bits */
extern uint8_t pdu_sysinfo[16];	/* 124 bits */
extern uint8_t pdu_acc_ass[2];
extern uint8_t pdu_schf[268];

void testpdu_init();

