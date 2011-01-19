#ifndef OSMO_PRIMITIVE_H
#define OSMO_PRIMITIVE_H

#include <stdint.h>

enum osmo_prim_operation {
	PRIM_OP_REQUEST,
	PRIM_OP_RESPONSE,
	PRIM_OP_INDICATION,
	PRIM_OP_CONFIRM,
};

struct osmo_prim_hdr {
	uint16_t sap;
	uint16_t primitive;
	enum osmo_prim_operation operation;
};

#endif
