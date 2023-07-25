#include <stdint.h>
#include <stdlib.h>
#include "lr1110.h"
#include "hdr.h"
#include "system.h"

#define MY_GROUP     LR1110_GROUP_WIFI
#define MY_GROUP_STR "WIFI"

enum {
      SCAN_TIME_LIMIT              = 0x01,
      GET_NB_RESULTS               = 0x05,
      READ_RESULTS                 = 0x06,
      READ_CUMUL_TIME              = 0x08,
      CONFIGURE_TIMESTAMP_AP_PHONE = 0x0b
};

void lr1110_wifi(parser_t *parser)
{
    uint8_t
	*mosi,
	*miso;
    uint32_t
	cmd,
	result_size;
    DECLARE_LOG_FP;
    lr1110_data_t
	*data = parser->data;
    static uint32_t
	format     = 0,
	nb_results = 0;

    mosi = data->mosi;
    miso = data->miso;

    (void) mosi;
    (void) miso;

    fprintf(log_fp, "WIFI PC: %x\n", data->pending_cmd);
    fprintf(log_fp, "%s %d pc: %p  \n", __func__, __LINE__, &data->pending_cmd);

    cmd = get_command(data);

    switch (cmd) {

    case GET_NB_RESULTS:
	checkPacketSize("GET_NB_RESULTS", 2);
	break;

    case READ_CUMUL_TIME:
	checkPacketSize("READ_CUMUL_TIME", 2);
	break;

    case READ_RESULTS:
	checkPacketSize("READ_RESULTS", 5);

	nb_results = mosi[3];
	format     = mosi[4];

	fprintf(log_fp, "index: %x ",     mosi[2]);
	fprintf(log_fp, "nbResults: %d ", nb_results);
	fprintf(log_fp, "format: %d ",    format);

	set_pending_cmd(READ_RESULTS);

	break;

    case RESPONSE(READ_RESULTS):

	switch(format) {
	case 1:
	    result_size = 22; // 22 bytes per entry
	    break;
	default:
	    fprintf(log_fp, "ERROR: invalid format: %x", format);
	    exit(1);
	}

	result_size *= nb_results;

	checkPacketSize("READ_RESULTS", result_size + 1);

	parse_stat1(miso[0]);

	hex_dump(&miso[1], result_size);
	break;

    case SCAN_TIME_LIMIT:
	checkPacketSize("SCAN_TIME_LIMIT", 11);
	break;

    case CONFIGURE_TIMESTAMP_AP_PHONE:
	checkPacketSize("CONFIGURE_TIMESTAMP_AP_PHONE", 6);
	break;


    default:
	hdr(parser, data->packet_start_time, "UNHANDLED_WIFI_CMD");
	fprintf(log_fp, "Unhandled WIFI command: %04x length: %d \n", cmd, data->count);
	exit(1);
    }
}
