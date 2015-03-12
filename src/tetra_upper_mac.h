#ifndef TETRA_UPPER_MAC_H
#define TETRA_UPPER_MAC_H

#include "tetra_prim.h"

int upper_mac_prim_recv(struct osmo_prim_hdr *op, void *priv);

/*  
 * ETSI EN 300 392-9 V1.5.1 (2012-04) page 15
 * Notification indicator information element
 * Table 3: Notification indicator information element contents
 */
static const char* const  notification_indicator_strings[]={
	"LE broadcast" /* 0 */ ,
	"LE acknowledgement" /* 1 */ ,
	"LE paging" /* 2 */ ,
	"AL operation" /* 3 */ ,
	"Call barred by SS-BIC" /* 4 */ ,
	"Call barred by SS-BOC" /* 5 */ ,
	"Call is forwarded (diverting)" /* 6 */ ,
	"Forwarding (diversion) activated" /* 7 */ ,
	"Identity presentation restricted" /* 8 */ ,
	"Presentation restriction overridden" /* 9 */ ,
	"Call waiting invoked" /* 10 */ ,
	"Call put on hold (remote hold)" /* 11 */ ,
	"Call on hold retrieved (remote retrieval)" /* 12 */ ,
	"Include call" /* 13 */ ,
	"Multiparty call" /* 14 */ ,
	"LSC invoked" /* 15 */ ,
	"Call rejected due to SS-AS" /* 16 */ ,
	"SS-AS not invoked/supported" /* 17 */ ,
	"Called user alerted" /* 18 */ ,
	"Called user connected" /* 19 */ ,
	"Call proceeding" /* 20 */ ,
	"SS-CFU invoked" /* 21 */ ,
	"SS-CFB invoked" /* 22 */ ,
	"SS-CFNRy invoked" /* 23 */ ,
	"SS-CFNRc invoked" /* 24 */ ,
	"AL-call or speech item" /* 25 */ ,
	"Notice of imminent call disconnection" /* 26 */ ,
	"Limited group coverage" /* 27 */ 
		/* values 28-63 are reserved */
};


#endif
