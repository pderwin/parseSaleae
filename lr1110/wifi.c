#include <stdint.h>
#include <stdlib.h>
#include "lr1110.h"
#include "hdr.h"
#include "system.h"

#define MY_GROUP     LR1110_GROUP_WIFI
#define MY_GROUP_STR "WIFI"

enum {
      SCAN_TIME_LIMIT              = 0x01,
      COUNTRY_CODE                 = 0x02,
      GET_NB_RESULTS               = 0x05,
      READ_RESULTS                 = 0x06,
      RESET_CUMUL_TIMING           = 0x07,
      READ_CUMUL_TIMING            = 0x08,
      CONFIGURE_TIMESTAMP_AP_PHONE = 0x0b
};

void lr1110_wifi(parser_t *parser)
{
    uint8_t
	*mosi,
	*miso;
    uint32_t
	cmd,
	entry_idx,
	entry_size,
	i;
    DECLARE_LOG_FP;
    lr1110_data_t
	*data = parser->data;
    static uint32_t
	format     = 0,
	nb_results = 0;

    mosi = data->mosi_array;
    miso = data->miso_array;

    (void) mosi;

    cmd = get_command(data);

    switch (cmd) {

    case COUNTRY_CODE:
	checkPacketSize("COUNTRY_CODE", 9);
	break;

    case GET_NB_RESULTS:
	checkPacketSize("GET_NB_RESULTS", 2);
	set_pending_cmd( GET_NB_RESULTS);
	break;

    case RESPONSE(GET_NB_RESULTS):
	checkPacketSize("GET_NB_RESULTS (resp)", 2);

	parse_stat1(miso[0]);
	fprintf(log_fp, "nb_results: %d ",     miso[1]);
	break;

    case READ_CUMUL_TIMING:
	checkPacketSize("READ_CUMUL_TIMING", 2);
	set_pending_cmd(READ_CUMUL_TIMING);
	break;

    case RESPONSE(READ_CUMUL_TIMING):
	checkPacketSize("READ_CUMUL_TIMING(resp)", 17);

	parse_stat1(miso[0]);

	fprintf(log_fp, "preamble detection: %d ", get_32(&miso[ 5]));
	fprintf(log_fp, "capture: %d ",            get_32(&miso[ 9]));
	fprintf(log_fp, "demodulation: %d ",       get_32(&miso[13]));
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
	    entry_size = 22; // 22 bytes per entry
	    break;
	default:
	    fprintf(log_fp, "ERROR: invalid format: %x", format);
	    exit(1);
	}

	checkPacketSize("READ_RESULTS", (entry_size * nb_results) + 1);

	parse_stat1(miso[0]);

	fprintf(log_fp, "\n\t\t\tType | Channel | RSSI |FrameCtl |     MAC      | PhiOffset |    TimeStamp     | PeriodBeacon \n");

	for (i=0; i < nb_results; i++) {

	    char
		rssi;
	    uint32_t
		j;

	    entry_idx = (i * entry_size) + 1;

	    fprintf(log_fp, "\t\t\t");
	    fprintf(log_fp, " %02x  |", miso[entry_idx + 0] ); // wifi type
	    fprintf(log_fp, "   %02x    | ",       miso[entry_idx + 1] );  // channel info

	    rssi = (char) miso[entry_idx + 2];
	    fprintf(log_fp, "%4d | ",   rssi );  // RSSI
	    fprintf(log_fp, "   %02x   | ",       miso[entry_idx + 3] );  // FrameCtl
	    fprintf(log_fp, "%02x%02x%02x%02x%02x%02x | ",
		    miso[entry_idx + 4],  // MAC
		    miso[entry_idx + 5],
		    miso[entry_idx + 6],
		    miso[entry_idx + 7],
		    miso[entry_idx + 8],
		    miso[entry_idx + 9] );
	    fprintf(log_fp, "   %04x   | ", get_16(&miso[entry_idx + 10]));  // PhiOffset

	    for (j=0; j<8; j++) {
		fprintf(log_fp, "%02x", miso[entry_idx + 12 + j]);  // TimeStamp
	    }
	    fprintf(log_fp, " | ");

	    fprintf(log_fp, "    %04x", get_16(&miso[entry_idx + 20]));  // PeriodBeacon

	    fprintf(log_fp, "\n");

	}

	break;

    case SCAN_TIME_LIMIT:
	checkPacketSize("SCAN_TIME_LIMIT", 11);

	fprintf(log_fp, "signal type: %x ",              miso[2]);
	fprintf(log_fp, "chan_mask: %x ",       get_16( &miso[3]));
	fprintf(log_fp, "acq_mode: %x ",                 miso[5]);
	fprintf(log_fp, "nb_max: %x ",                   miso[6]);
	fprintf(log_fp, "timeout_per_chan: %x ", get_16(&miso[7]));
	fprintf(log_fp, "timeout_per_scan: %x", get_16(&miso[9]));
	break;

    case CONFIGURE_TIMESTAMP_AP_PHONE:
	checkPacketSize("CONFIGURE_TIMESTAMP_AP_PHONE", 6);
	break;

    case RESET_CUMUL_TIMING:
	checkPacketSize("RESET_CUMUL_TIMING", 2);
	break;

    default:
	hdr(parser, data->packet_start_time, "UNHANDLED_WIFI_CMD");
	fprintf(log_fp, "Unhandled WIFI command: %04x length: %d \n", cmd, data->count);
	exit(1);
    }
}
