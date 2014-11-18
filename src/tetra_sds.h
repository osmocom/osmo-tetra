/* tetra sds stuff --sq5bpf */
#include <stdint.h>

#ifndef HAVE_TETRA_SDS_H
#define HAVE_TETRA_SDS_H

/* Table 29.21: Protocol identifier information element contents */
enum tetra_sds_protoid {
	TETRA_SDS_PROTO_RESERVED	  = 0x00,
	TETRA_SDS_PROTO_OTAK       		= 0x01,
	TETRA_SDS_PROTO_SIMPLE_TXTMSG            = 0x02,
	TETRA_SDS_PROTO_SIMPLE_LOC            = 0x03,
	TETRA_SDS_PROTO_WAP            = 0x04,
	TETRA_SDS_PROTO_WCMP            = 0x05,
	TETRA_SDS_PROTO_MDMO            = 0x06,
	TETRA_SDS_PROTO_PINAUTH            = 0x07,
	TETRA_SDS_PROTO_EEENCR            = 0x08,
	TETRA_SDS_PROTO_SIMPLE_ITXTMSG            = 0x09,
	TETRA_SDS_PROTO_LIP            = 0x0a,
	TETRA_SDS_PROTO_NAP            = 0x0b,
	TETRA_SDS_PROTO_CONCAT_SDS            = 0x0c,
	TETRA_SDS_PROTO_DOTAM            = 0x0d,
	TETRA_SDS_PROTO_TXTMSG            = 0x82,
	TETRA_SDS_PROTO_LOCSYSTEM            = 0x83,
	TETRA_SDS_PROTO_WAP2            = 0x84,
	TETRA_SDS_PROTO_WCMP2            = 0x85,
	TETRA_SDS_PROTO_MDMO2            = 0x86,
	TETRA_SDS_PROTO_EEENCR2            = 0x88,
	TETRA_SDS_PROTO_ITXTMSG            = 0x89,
	TETRA_SDS_PROTO_MSMUSERHEAD            = 0x8a,
	TETRA_SDS_PROTO_CONCAT_SDS2            = 0x8c
};


struct sds_type {
	uint8_t type;
	char * description;
};
static const struct sds_type sds_types[]=
{
	{ TETRA_SDS_PROTO_RESERVED , "Reserved" },
	{ TETRA_SDS_PROTO_OTAK , "OTAK" },
	{ TETRA_SDS_PROTO_SIMPLE_TXTMSG , "Simple Text Messaging" },
	{ TETRA_SDS_PROTO_SIMPLE_LOC , "Simple location system" },
	{ TETRA_SDS_PROTO_WAP , "Wireless Datagram Protocol WAP" },
	{ TETRA_SDS_PROTO_WCMP , "Wireless Control Message Protocol WCMP" },
	{ TETRA_SDS_PROTO_MDMO , "M-DMO" },
	{ TETRA_SDS_PROTO_PINAUTH , "PIN authentication" },
	{ TETRA_SDS_PROTO_EEENCR , "End-to-end encrypted message" },
	{ TETRA_SDS_PROTO_SIMPLE_ITXTMSG , "Simple immediate text messaging" },
	{ TETRA_SDS_PROTO_LIP , "Location information protocol" },
	{ TETRA_SDS_PROTO_NAP , "Net Assist Protocol (NAP)" },
	{ TETRA_SDS_PROTO_CONCAT_SDS , "Concatenated SDS message" },
	{ TETRA_SDS_PROTO_DOTAM , "DOTAM, refer to TS 100 392-18-3" },
	{ TETRA_SDS_PROTO_TXTMSG , "Text Messaging" },
	{ TETRA_SDS_PROTO_LOCSYSTEM , "Location system" },
	{ TETRA_SDS_PROTO_WAP2 , "Wireless Datagram Protocol WAP" },
	{ TETRA_SDS_PROTO_WCMP2 , "Wireless Control Message Protocol WCMP" },
	{ TETRA_SDS_PROTO_MDMO2 , "M-DMO" },
	{ TETRA_SDS_PROTO_EEENCR2 , "End-to-end encrypted message" },
	{ TETRA_SDS_PROTO_ITXTMSG , "Immediate text messaging" },
	{ TETRA_SDS_PROTO_MSMUSERHEAD , "Message with User Data Header" },
	{ TETRA_SDS_PROTO_CONCAT_SDS2 , "Concatenated SDS message" },
	{ 0x0,0 }
};

char *get_sds_type(uint8_t type);
int decode_pdu(char *dec,unsigned char *enc,int len);

#endif
