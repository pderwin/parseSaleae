#include <stdlib.h>
#include "crypto.h"
#include "hdr.h"
#include "lr1110.h"

#define MY_GROUP     LR1110_GROUP_CRYPTO
#define MY_GROUP_STR "CRYPTO"

enum {
      SET_KEY              = 0x02,
      DERIVE_AND_STORE_KEY = 0x03,
      PROCESS_JOIN_ACCEPT  = 0x04,
      COMPUTE_AES_CMAC     = 0x05,
      AES_ENCRYPT01        = 0x07,
      STORE_TO_FLASH       = 0x0a,
      RESTORE_FROM_FLASH   = 0x0b
};


void lr1110_crypto(parser_t *parser)
{
    uint32_t cmd;
    uint32_t i;
    uint8_t  *mosi;
    uint8_t  *miso;
    lr1110_data_t *data = parser->data;
    FILE     *log_fp = parser->lf->fp;

    mosi = data->mosi;
    miso = data->miso;

    (void) miso;

    if (data->pending_cmd) {
	cmd = data->pending_cmd;
    }
    else{
	cmd = data->mosi[1];
    }

    switch (cmd) {

    case AES_ENCRYPT01:
	checkPacketSize("AES_ENCRYPT01", 0);
	break;

    case COMPUTE_AES_CMAC:
	checkPacketSize("COMPUTE_AES_CMAC", 0);

	fprintf(log_fp, "Bytes: \n");
	for (i=0; i<13; i++) {
	    fprintf(log_fp, "   %2d: %x \n", i, mosi[i]);
	}
	break;

    case DERIVE_AND_STORE_KEY:
	checkPacketSize("DERIVE_AND_STORE_KEY", 20);
	break;

    case PROCESS_JOIN_ACCEPT:
	checkPacketSize("PROCESS_JOIN_ACCEPT", 0);
	break;

    case RESTORE_FROM_FLASH:
	checkPacketSize("RESTORE_FROM_FLASH", 2);
	break;

    case SET_KEY:
	checkPacketSize("SET_KEY", 19);
	break;
    case (SET_KEY | PENDING):
	checkPacketSize("SET_KEY(resp)", 19);

	clear_pending_cmd(SET_KEY);

	break;

    case STORE_TO_FLASH:
	checkPacketSize("STORE_TO_FLASH", 2);
	break;

    default:
	hdr(parser->lf, data->packet_start_time, "UNHANDLED_CRYPTO_CMD");
	fprintf(log_fp, "Unhandled CRYPTO command: %04x length: %d \n", cmd, data->count);
	exit(1);
    }
}
