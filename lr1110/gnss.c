#include <stdlib.h>
#include "gnss.h"
#include "hdr.h"
#include "lr1110.h"

#define MY_GROUP     LR1110_GROUP_GNSS
#define MY_GROUP_STR "GNSS"

enum {
      SET_CONSTELLATION        = 0x00,
      READ_CONSTELLATION       = 0x01,
      READ_ALMANAC_UPDATE      = 0x03,
      SET_FREQ_SEARCH_SPACE    = 0x04,
      READ_FREQ_SEARCH_SPACE   = 0x05,
      SET_MODE                 = 0x08,
      SCAN_AUTONOMOUS          = 0x09,
      SCAN_ASSISTED            = 0x0a,
      GET_RESULT_SIZE          = 0x0c,
      READ_RESULTS             = 0x0d,
      ALMANAC_FULL_UPDATE      = 0x0e,
      UNKNOWN_0x0F             = 0x0f,
      SET_ASSISTANCE_POS  = 0x10,
      READ_ASSISTANCE_POS = 0x11,
      PUSH_SOLVER_MSG          = 0x14,
      GET_CONTEXT_STATUS       = 0x16,
      GET_NB_SV_DETECTED       = 0x17,
      GET_SV_DETECTED          = 0x18,
      GET_CONSUMPTION          = 0x19,
      GET_SV_VISIBLE           = 0x1f,
      GET_SV_VISIBLE_DOPPLER   = 0x20,

      // not documented this way      GNSS_SCAN_AUTONOMOUS        = 0x0430,
};

#define LR11XX_GNSS_SCALING_LATITUDE  90
#define LR11XX_GNSS_SCALING_LONGITUDE 180

static void  almanac_update(FILE *fp, uint8_t *cp);
static float to_latitude(uint8_t *lat_str);
static float to_longitude(uint8_t *long_str);

static uint8_t satellites_detected;


void lr1110_gnss(parser_t *parser)
{
    uint8_t
	*cp,
	*miso,
	*mosi;
    int16_t
	latitude,
	longitude;
    int32_t
	count;
    uint32_t
	cmd,
	i,
	scan_time;
    float
	latitude_f,
	longitude_f;
    static uint32_t
	payload_length  = 0;
    FILE
	*log_fp = parser->log_file->fp;
    lr1110_data_t
	*data = parser->data;

    mosi = data->mosi_array;
    miso = data->miso_array;

    cmd = get_command(data);

    switch (cmd) {

    case ALMANAC_FULL_UPDATE:
	checkPacketSize("ALMANAC_FULL_UPDATE", 0);
	almanac_update(log_fp, &mosi[2]);
	break;

    case GET_NB_SV_DETECTED:
	checkPacketSize("GET_NB_SV_DETECTED", 2);
	set_pending_cmd( GET_NB_SV_DETECTED);
	break;

    case ( GET_NB_SV_DETECTED | PENDING):
	checkPacketSize("GET_NB_SV_DETECTED (resp)", 2);

	parse_stat1(miso[0]);

	satellites_detected = miso[1];

	fprintf(log_fp, "sv_detected: %x", satellites_detected);

	break;

    case GET_SV_DETECTED:
	checkPacketSize("GET_SV_DETECTED", 2);
	set_pending_cmd( GET_SV_DETECTED);
	break;

    case (GET_SV_DETECTED | PENDING):
	checkPacketSize("GET_SV_DETECTED (resp)", 0);

	parse_stat1(miso[0]);


	/*
	 * Subtract one off for the stat1 byte
	 */
	count = (data->count - 1);
	cp = &miso[1];

	fprintf(log_fp, "\n");

	while(count >= 4) {
	    fprintf(log_fp, "%20ssvid: %02x ", "", cp[0]);
	    fprintf(log_fp, "snr: %x ",  cp[1]);
	    fprintf(log_fp, "doppler: %04x\n", (cp[2] << 8) | cp[3]);

	    cp += 4;
	    count -= 4;
	}

	break;

    case GET_CONSUMPTION:
	checkPacketSize("GET_CONSUMPTION", 2);
	set_pending_cmd(GET_CONSUMPTION);
	break;

    case ( GET_CONSUMPTION | PENDING):
	checkPacketSize("GET_CONSUMPTION (resp)", 9);

	parse_stat1(miso[0]);

	fprintf(log_fp, "CPU time: %08x ", get_32(&miso[1]));
	fprintf(log_fp, "Radio time: %08x ", get_32(&miso[5]));

	break;

    case GET_CONTEXT_STATUS:
	checkPacketSize("GET_CONTEXT_STATUS", 2);
	set_pending_cmd(GET_CONTEXT_STATUS);
	break;

    case ( GET_CONTEXT_STATUS | PENDING):
	{
	    uint32_t freqss_0;
	    uint32_t freqss_1;

	    checkPacketSize("GET_CONTEXT_STATUS (resp)", 10);

	    parse_stat1(miso[0]);
	    fprintf(log_fp, "dest_id: %x ", miso[1]);
	    fprintf(log_fp, "opcode: %x ", miso[2]);
	    fprintf(log_fp, "Fw_ver: %x ", miso[3]);
	    fprintf(log_fp, "almanac_crc: %x ", get_32(&miso[4]));
	    fprintf(log_fp, "error_code: %x ", (miso[8] >> 4) & 0xf);
	    fprintf(log_fp, "almanac_update_bitmask: %x ", (miso[8] >> 3) & 0x7);

	    freqss_1 = ((miso[8] >> 0) & 1);
	    freqss_0 = ((miso[9] >> 7) & 1);

	    fprintf(log_fp, "freq_search_space: %x ", (freqss_1 << 1) | freqss_0 );
	}

	break;

    case PUSH_SOLVER_MSG:
	checkPacketSize("PUSH_SOLVER_MSG", 0);
	hex_dump(&mosi[2], data->count - 2);
	break;

    case SET_CONSTELLATION:
	checkPacketSize("SET_CONSTELLATION", 3);

	fprintf(log_fp, "to_use: %x ", mosi[2]);
	break;


    case SCAN_AUTONOMOUS:
	checkPacketSize("SCAN_AUTONOMOUS", 9);

	fprintf(log_fp, "time: %x ",        get_32(&mosi[2]));
	fprintf(log_fp, "effort: %x ",      mosi[6]);
	fprintf(log_fp, "result_mask: %x ", mosi[7]);
	fprintf(log_fp, "nbsvmax: %x ",     mosi[8]);
	break;

    case SET_MODE:
	checkPacketSize("SET_MODE", 3);

	fprintf(log_fp, "Mode: %s", mosi[2] == 0 ? "Single" : "Double");
	break;

    case GET_RESULT_SIZE:
	checkPacketSize("GET_RESULT_SIZE", 2);
	set_pending_cmd(GET_RESULT_SIZE);
	break;

    case ( GET_RESULT_SIZE | PENDING):
	checkPacketSize("GET_RESULT_SIZE (rsp)", 3);

	parse_stat1(miso[0]);

	payload_length = get_16(&miso[1]);

	fprintf(log_fp, "result_size: %d ", payload_length );

	break;

    case READ_ALMANAC_UPDATE:
	checkPacketSize("READ_ALMANAC_UPDATE", 2);
	set_pending_cmd(READ_ALMANAC_UPDATE);
	break;

    case RESPONSE(READ_ALMANAC_UPDATE):

	checkPacketSize("READ_ALMANAC_UPDATE(resp)", 2);

	parse_stat1(miso[0]);

	fprintf(log_fp, "constallation_bitmask: %x ", miso[1] );

	break;

    case READ_ASSISTANCE_POS:
	checkPacketSize("READ_ASSISTANCE_POS", 2);
	set_pending_cmd(READ_ASSISTANCE_POS);
	break;

    case RESPONSE(READ_ASSISTANCE_POS):
	checkPacketSize("READ_ASSISTANCE_POS (resp)", 5);

	parse_stat1(miso[0]);

	latitude  = get_16(&miso[1]);
	longitude = get_16(&miso[3]);

	latitude_f  = (latitude  * LR11XX_GNSS_SCALING_LATITUDE  ) / 2048;
	longitude_f = (longitude * LR11XX_GNSS_SCALING_LONGITUDE ) / 2048;

	fprintf(log_fp, "Location: %.5f, %.5f ", latitude_f, longitude_f );

	break;

    case READ_CONSTELLATION:
	checkPacketSize("READ_CONSTELLATION", 2);
	set_pending_cmd(READ_CONSTELLATION);
	break;

    case RESPONSE(READ_CONSTELLATION):

	checkPacketSize("READ_CONSTELLATION(resp)", 2);

	parse_stat1(miso[0]);

	fprintf(log_fp, "constallation_bitmask: %x ", miso[1] );

	break;

    case READ_RESULTS:
	checkPacketSize("READ_RESULTS", 2);
	set_pending_cmd(READ_RESULTS);
	break;

    case RESPONSE(READ_RESULTS):
	checkPacketSize("READ_RESULTS (rsp)", payload_length + 1);

	parse_stat1(miso[0]);

	for (i=0; i< payload_length + 1; i++) {
	    fprintf(log_fp, "%02x ", miso[i] );
	}

	break;

    case SCAN_ASSISTED:
	checkPacketSize("SCAN_ASSISTED", 9);

	scan_time = get_32(&mosi[2]);

	fprintf(log_fp, "time: %d (0x%x) ", scan_time, scan_time );
	fprintf(log_fp, "effort: %d ", mosi[6]);
	fprintf(log_fp, "result_mask: %d ", mosi[7]);
	fprintf(log_fp, "nb_sv_max: %d ", mosi[8]);
	break;

    case SET_ASSISTANCE_POS:
	checkPacketSize("SET_ASSISTANCE_POS", 6);

	fprintf(log_fp, "lat: %f ", to_latitude(&mosi[2]));
	fprintf(log_fp, "long: %f", to_longitude(&mosi[4]));
	break;

    case SET_FREQ_SEARCH_SPACE:
	checkPacketSize("SET_FREQ_SEARCH_SPACE", 0);
	break;

    case READ_FREQ_SEARCH_SPACE:
	checkPacketSize("READ_FREQ_SEARCH_SPACE", 0);
	break;

    case UNKNOWN_0x0F:
	checkPacketSize("UNKNOWN_0x0F", 2);
	break;

    case GET_SV_VISIBLE:
	checkPacketSize("GET_SV_VISIBLE", 11);

	fprintf(log_fp, "Time: %08x ",        get_32(&mosi[2]));

	latitude_f  = to_latitude(&mosi[6]);
	longitude_f = to_longitude(&mosi[8]);

	fprintf(log_fp, "Latitude: %.5f ",    latitude_f);
	fprintf(log_fp, "Longitude: %.5f ",   longitude_f);
	fprintf(log_fp, "Constellation: %d ", mosi[10]);

	set_pending_cmd(GET_SV_VISIBLE);
	break;

    case RESPONSE(GET_SV_VISIBLE):
	checkPacketSize("GET_SV_VISIBLE(resp)", 2);

	parse_stat1(miso[0]);
	fprintf(log_fp, "visible: %d ",  miso[1]);
	break;

    case GET_SV_VISIBLE_DOPPLER:
	checkPacketSize("GET_SV_VISIBLE_DOPPLER", 2);
	break;

    default:
	hdr(parser, data->packet_start_time, "UNHANDLED_GNSS_CMD");
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
	hex_dump(&cp[1], 16);

	fprintf(log_fp, "| CA: %04x", (cp[16] << 8) | cp[17]);
	fprintf(log_fp, "| MOD: %02x", cp[18]);
	fprintf(log_fp, "| CONST: %02x", cp[19]);
    }

}

static float to_latitude(uint8_t *lat_str)
{
    int16_t
	latitude;
    float
	latitude_f;

    latitude = get_16(lat_str);

    latitude_f  = (latitude  * LR11XX_GNSS_SCALING_LATITUDE  ) / 2048;

    return latitude_f;
}

static float to_longitude(uint8_t *long_str)
{
    int16_t
	longitude;
    float
	longitude_f;

    longitude = get_16(long_str);

    longitude_f = (longitude * LR11XX_GNSS_SCALING_LONGITUDE ) / 2048;

    return longitude_f;
}
