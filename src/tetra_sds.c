#include <stdint.h>
/* Tetra SDS functions --sq5bpf */
#include "tetra_sds.h"

const char oth_reserved[]="Other Reserved";

int decode_pdu(char *dec,unsigned char *enc,int len)
{
	int outlen=0;
	int bits=0;
	unsigned char carry=0;
	while(len) {
		*dec=carry|(*enc<<(bits))&0x7f;
		carry=*enc>>(7-bits);
		bits++;
		dec++;
		outlen++;
		enc++;

		if (bits==7) {
			*dec=carry;
			dec++;
			bits=0;
			outlen++;
		}
	}
	*dec=0;
}




char *get_sds_type(uint8_t type) {
	struct sds_type* a= (struct sds_type *)&sds_types;	
	while (a->description)
	{
		if (a->type==type) {
			return(a->description);

		}
		a++;
	}
	return((char *)&oth_reserved);

}

