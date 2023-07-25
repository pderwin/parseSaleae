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
      VERIFY_AES_CMAC      = 0x06,
      AES_ENCRYPT01        = 0x07,
      STORE_TO_FLASH       = 0x0a,
      RESTORE_FROM_FLASH   = 0x0b,
      UNKNOWN_0x10         = 0x10,
};

static char *key_id_str[] = {
			     "?? 0",
			     "?? 1",
			     "?? 2",
			     "?? 3",
			     "?? 4",
			     "?? 5",
			     "?? 6",
			     "?? 7",
			     "?? 8",
			     "?? 9",
			     "?? 10",
			     "?? 11",
			     "?? 12",
			     "?? 13",
			     "?? 14",
			     "NwkSEncKey",
			     "?? 16",
			     "?? 17",
			     "?? 18",
			     "?? 19",
			     "?? 20",
			     "?? 21",
			     "?? 22",
			     "?? 23",
			     "?? 24",
			     "?? 25",
			     "?? 26",
			     "?? 27"
};

void lr1110_crypto(parser_t *parser)
{
    uint8_t
	*miso,
	*mosi;
    uint32_t
	cmd,
	key_id;
    lr1110_data_t
	*data = parser->data;
    FILE
	*log_fp = parser->log_file->fp;


    mosi = data->mosi;
    miso = data->miso;

    (void) miso;

    cmd = get_command(data);

    switch (cmd) {

    case AES_ENCRYPT01:
	checkPacketSize("AES_ENCRYPT01", 0);
	break;

    case COMPUTE_AES_CMAC:
	checkPacketSize("COMPUTE_AES_CMAC", 0);

	key_id = mosi[2];

	fprintf(log_fp, "Key: %x (%s)", key_id, key_id_str[key_id] );

	fprintf(log_fp, "Payload: ");
	hex_dump(&mosi[3], data->count - 3);

	set_pending_cmd(COMPUTE_AES_CMAC);
	break;

    case RESPONSE(COMPUTE_AES_CMAC):

	parse_stat1(miso[0]);

	fprintf(log_fp, "CEstatus: %x ", miso[1]);
	fprintf(log_fp, "MIC: %08x", get_32(&miso[2]) );

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

    case RESPONSE(SET_KEY):
	checkPacketSize("SET_KEY(resp)", 19);

	break;

    case STORE_TO_FLASH:
	checkPacketSize("STORE_TO_FLASH", 2);
	break;

    case VERIFY_AES_CMAC:
	checkPacketSize("VERIFY_AES_CMAC", 0);
	break;

    case UNKNOWN_0x10:
	checkPacketSize("UNKNOWN_0x10", 0);
	break;

    default:
	hdr(parser, data->packet_start_time, "UNHANDLED_CRYPTO_CMD");
	fprintf(log_fp, "Unhandled CRYPTO command: %04x length: %d ZZZ: %x \n", cmd, data->count,  UNKNOWN_0x10);
	exit(1);
    }
}
