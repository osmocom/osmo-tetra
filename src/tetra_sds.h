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


/*************   LIP   ****************/
/* LIP direction  of travel */
enum tetra_lip_dirtravelid {
TETRA_LIP_DIRTRAVEL_N = 0 ,
TETRA_LIP_DIRTRAVEL_NNE = 1 ,
TETRA_LIP_DIRTRAVEL_NE = 2 ,
TETRA_LIP_DIRTRAVEL_ENE = 3 ,
TETRA_LIP_DIRTRAVEL_E = 4 ,
TETRA_LIP_DIRTRAVEL_ESE = 5 ,
TETRA_LIP_DIRTRAVEL_SE = 6 ,
TETRA_LIP_DIRTRAVEL_SSE = 7 ,
TETRA_LIP_DIRTRAVEL_S = 8 ,
TETRA_LIP_DIRTRAVEL_SSW = 9 ,
TETRA_LIP_DIRTRAVEL_SW = 10 ,
TETRA_LIP_DIRTRAVEL_WSW = 11 ,
TETRA_LIP_DIRTRAVEL_W = 12 ,
TETRA_LIP_DIRTRAVEL_WNW = 13 ,
TETRA_LIP_DIRTRAVEL_NW = 14 ,
TETRA_LIP_DIRTRAVEL_NNW = 15 
};
struct lip_dirtravel_type {
	uint8_t type;
	char * description;
};
static const struct lip_dirtravel_type lip_dirtravel_types[]=
{
	{ TETRA_LIP_DIRTRAVEL_N , "N" },
	{ TETRA_LIP_DIRTRAVEL_NNE , "NNE" },
	{ TETRA_LIP_DIRTRAVEL_NE , "NE" },
	{ TETRA_LIP_DIRTRAVEL_ENE , "ENE" },
	{ TETRA_LIP_DIRTRAVEL_E , "E" },
	{ TETRA_LIP_DIRTRAVEL_ESE , "ESE" },
	{ TETRA_LIP_DIRTRAVEL_SE , "SE" },
	{ TETRA_LIP_DIRTRAVEL_SSE , "SSE" },
	{ TETRA_LIP_DIRTRAVEL_S , "S" },
	{ TETRA_LIP_DIRTRAVEL_SSW , "SSW" },
	{ TETRA_LIP_DIRTRAVEL_SW , "SW" },
	{ TETRA_LIP_DIRTRAVEL_WSW , "WSW" },
	{ TETRA_LIP_DIRTRAVEL_W , "W" },
	{ TETRA_LIP_DIRTRAVEL_WNW , "WNW" },
	{ TETRA_LIP_DIRTRAVEL_NW , "NW" },
	{ TETRA_LIP_DIRTRAVEL_NNW , "NNW" },
	{ 0x0,0 }
};

static const char* const  lip_position_errors[]={ "<2m", "<20m", "<200m", "<2km", "<20km", "=<200km", ">200km", "unknown" };
int decode_lip(char *out, int outlen,uint8_t *bits,int datalen);

/**************   Location System   **************/
int decode_locsystem(char *out, int outlen,uint8_t *bits,int datalen);

#endif
