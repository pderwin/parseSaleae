#include <stdlib.h>
#include "gnss.h"
#include "hdr.h"
#include "lr1110.h"

#define MY_GROUP     LR1110_GROUP_GNSS
#define MY_GROUP_STR "GNSS"

enum {
      GNSS_SET_CONSTELLATION      = 0x00,
      GNSS_SET_MODE               = 0x08,
      GNSS_SCAN_AUTONOMOUS        = 0x09,
      GNSS_SCAN_ASSISTED          = 0x0a,
      GNSS_GET_RESULT_SIZE        = 0x0c,
      GNSS_READ_RESULTS           = 0x0d,
      GNSS_ALMANAC_FULL_UPDATE    = 0x0e,
      GNSS_UNKNOWN_040F           = 0x0f,
      GNSS_GET_CONTEXT_STATUS     = 0x16,

      // not documented this way      GNSS_SCAN_AUTONOMOUS        = 0x0430,
};

static void almanac_update(FILE *fp, uint8_t *cp);


void lr1110_gnss(parser_t *parser)
{
    uint32_t  cmd;
    uint32_t i;
    uint8_t  *mosi;
    uint8_t  *miso;
    static uint32_t payload_length  = 0;
    FILE     *log_fp = parser->lf->fp;
    lr1110_data_t *data = parser->data;

    mosi = data->mosi;
    miso = data->miso;

    (void) mosi;

    if (data->pending_cmd) {
	cmd = data->pending_cmd;
    }
    else{
	cmd = data->mosi[1];
    }

    switch (cmd) {

    case GNSS_ALMANAC_FULL_UPDATE:
	checkPacketSize("GNSS_ALMANAC_FULL_UPDATE", 22);
	almanac_update(log_fp, &mosi[2]);
	break;

    case GNSS_GET_CONTEXT_STATUS:
	checkPacketSize("GNSS_GET_CONTEXT_STATUS", 2);
	break;

    case GNSS_SET_CONSTELLATION:
	checkPacketSize("GNSS_SET_CONSTELLATION", 3);

	fprintf(log_fp, "to_use: %x ", mosi[2]);
	break;


    case GNSS_SCAN_AUTONOMOUS:
	checkPacketSize("GNSS_SCAN_AUTONOMOUS", 9);

	fprintf(log_fp, "time: %x ",        get_32(&mosi[2]));
	fprintf(log_fp, "effort: %x ",      mosi[6]);
	fprintf(log_fp, "result_mask: %x ", mosi[7]);
	fprintf(log_fp, "nbsvmax: %x ",     mosi[8]);
	break;

    case GNSS_SET_MODE:
	checkPacketSize("GNSS_SET_MODE", 3);

	fprintf(log_fp, "Mode: %s", mosi[2] == 0 ? "Single" : "Double");
	break;

    case GNSS_UNKNOWN_040F:
	checkPacketSize("GNSS_UNKNOWN_040F", 2);
	break;


    case GNSS_GET_RESULT_SIZE:
	checkPacketSize("GNSS_GET_RESULT_SIZE", 2);
	set_pending_cmd(GNSS_GET_RESULT_SIZE);
	break;

    case ( GNSS_GET_RESULT_SIZE | PENDING):
	checkPacketSize("GNSS_GET_RESULT_SIZE (rsp)", 3);

	parse_stat1(miso[0]);

	payload_length = get_16(&miso[1]);

	fprintf(log_fp, "result_size: %d ", payload_length );

	clear_pending_cmd(GNSS_GET_RESULT_SIZE);
	break;

    case GNSS_READ_RESULTS:
	checkPacketSize("GNSS_READ_RESULTS", 2);
	set_pending_cmd(GNSS_READ_RESULTS);
	break;

    case ( GNSS_READ_RESULTS | PENDING):
	checkPacketSize("GNSS_READ_RESULTS (rsp)", payload_length + 1);

	parse_stat1(miso[0]);

	for (i=0; i< payload_length + 1; i++) {
	    fprintf(log_fp, "%02x ", miso[i] );
	}

	clear_pending_cmd(GNSS_READ_RESULTS);
	break;

    default:
	hdr(parser->lf, data->packet_start_time, "UNHANDLED_GNSS_CMD");
	fprintf(log_fp, "Unhandled GNSS command: %04x length: %d \n", cmd, data->count);
	exit(1);
    }
}


static void almanac_update(FILE *log_fp, uint8_t *cp)
{
    uint8_t SVid;

    SVid = cp[0];

    if (SVid == 0x80) {
	fprintf(log_fp, "Header: ");
	fprintf(log_fp, "Date: %04x " , ((cp[1] << 8) | cp[2]) );
	fprintf(log_fp, "CRC:  %04x" , ((cp[3] << 24) | (cp[4] << 16) | (cp[5] << 8) | cp[6]) );
    }
    else {
	fprintf(log_fp, "SV: %02x | ", SVid);
	hex_dump(log_fp, &cp[1], 16);

	fprintf(log_fp, "| CA: %04x", (cp[16] << 8) | cp[17]);
	fprintf(log_fp, "| MOD: %02x", cp[18]);
	fprintf(log_fp, "| CONST: %02x", cp[19]);
    }

}
