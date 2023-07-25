#include <stdint.h>
#include <stdlib.h>
#include "lr1110.h"
#include "hdr.h"
#include "system.h"

#define MY_GROUP LR1110_GROUP_SYSTEM
#define MY_GROUP_STR "SYSTEM"

enum {
      GET_STATUS           = 0x00,
      GET_VERSION          = 0x01,
      UNKNOWN_0108         = 0x08,
      WRITE_BUFFER_8       = 0x09,
      READ_BUFFER_8        = 0x0a,
      CLEAR_RX_BUFFER      = 0x0b,
      WRITE_REG_MEM_MASK32 = 0x0c,
      GET_ERRORS           = 0x0d,
      CLEAR_ERRORS         = 0x0e,
      CALIBRATE            = 0x0f,
      SET_REG_MODE         = 0x10,
      SET_DIO_AS_RF_SWITCH = 0x12,
      SET_DIO_IRQ_PARAMS   = 0x13,
      CLEAR_IRQ            = 0x14,
      CONFIG_LF_CLOCK      = 0x16,
      SET_TCXO_MODE        = 0x17,
      REBOOT               = 0x18,
      GET_VBAT             = 0x19,
      GET_TEMP             = 0x1a,
      SET_SLEEP            = 0x1b,
      SET_STANDBY          = 0x1c,
      SET_FS               = 0x1d,
      GET_RANDOM           = 0x20,
      GET_DEV_EUI          = 0x25,
      GET_JOIN_EUI         = 0x26,
      READ_DEVICE_PIN      = 0x27,
};

static void parse_errors(FILE *log_fp, uint16_t errors);
static void parse_irq   (parser_t *parser, uint8_t *miso);

extern uint32_t pld_len_in_bytes;

void lr1110_system(parser_t *parser)
{
    uint32_t  cmd;
    uint32_t delay;
    uint32_t
	i;
    int32_t
	time_secs;
    static int32_t
	last_time_secs = 0;
    uint8_t  *mosi;
    uint8_t  *miso;
    FILE     *log_fp = parser->log_file->fp;
    lr1110_data_t *data = parser->data;

    mosi = data->mosi;
    miso = data->miso;

    cmd = get_command(data);

    switch (cmd) {

    case CALIBRATE:
	checkPacketSize("CALIBRATE", 3);

	fprintf(log_fp, "params: %08x", mosi[2]);
	break;


    case CLEAR_ERRORS:
	checkPacketSize("CLEAR_ERRORS", 2);
	break;

    case CLEAR_IRQ:
	checkPacketSize("CLEAR_IRQ", 6);

	parse_irq(parser, &mosi[2]);

	break;

    case CLEAR_RX_BUFFER:
	checkPacketSize("CLEAR_RX_BUFFER", 2);
	break;

    case CONFIG_LF_CLOCK:
	checkPacketSize("CONFIG_LF_CLOCK", 3);

	fprintf(log_fp, "wait xtal ready: %d clock: %d ", (mosi[2] >> 2) & 1, mosi[2] & 3);
	break;

    case GET_DEV_EUI:
	checkPacketSize("GET_DEV_EUI", 2);
	set_pending_cmd(GET_DEV_EUI);
	break;

    case RESPONSE(GET_DEV_EUI):

	checkPacketSize("GET_DEV_EUI (resp)", 9);

	parse_stat1(miso[0]);

	for (i=0; i<8; i++) {
	    fprintf(log_fp, "%02x ", miso[i+1]);
	}

	break;


    case GET_ERRORS:
	checkPacketSize("GET_ERRORS", 2);
	set_pending_cmd(GET_ERRORS);
	break;

    case RESPONSE(GET_ERRORS):
	checkPacketSize("GET_ERRORS (resp)", 3);

	parse_stat1(miso[0]);
	parse_errors(log_fp, get_16(&miso[1]));

	break;

    case GET_JOIN_EUI:
	checkPacketSize("GET_JOIN_EUI", 2);
	set_pending_cmd(GET_JOIN_EUI);
	break;

    case RESPONSE(GET_JOIN_EUI):
	checkPacketSize("GET_JOIN_EUI (resp)", 9);

	parse_stat1(miso[0]);

	for (i=0; i<8; i++) {
	    fprintf(log_fp, "%02x ", miso[i+1]);
	}

	break;

    case GET_RANDOM:
	checkPacketSize("GET_RANDOM", 2);
	set_pending_cmd(GET_RANDOM);
	break;

    case RESPONSE(GET_RANDOM):
	checkPacketSize("GET_RANDOM (resp)", 5);

	fprintf(log_fp, "random: %x", get_32(&miso[1]) );
	break;

    case SET_FS:
	checkPacketSize("SET_FS", 2);
	break;

    case SET_STANDBY:
	checkPacketSize("SET_STANDBY", 3);

	fprintf(log_fp, "StbyConfig: %02x ",  mosi[2]);
	break;


    case GET_STATUS:
	checkPacketSize("GET_STATUS", 6);

	parse_stat1(miso[0]);
	parse_stat2(miso[1]);

	parse_irq(parser, &miso[2]);
	break;

    case GET_TEMP:
	checkPacketSize("GET_TEMP", 2);
	//	set_pending_cmd(GET_VBAT);
	break;

    case GET_VBAT:
	checkPacketSize("GET_VBAT", 2);
	set_pending_cmd(GET_VBAT);
	break;
    case RESPONSE(GET_VBAT):
	{

	    float vBat;

	    checkPacketSize("GET_VBAT (resp)", 2);

	    parse_stat1(miso[0]);

	    /*
	     * Using equation from spec:
	     */
	    vBat = (miso[1] * 5.0) / 255;

	    vBat -= 1;
	    vBat *= 1.35;


	    fprintf(log_fp, "vBat: %5.2f ", vBat);
	}
	break;

    case GET_VERSION:
	checkPacketSize("GET_VERSION", 2);
	set_pending_cmd(GET_VERSION);
	break;

    case RESPONSE(GET_VERSION):

	checkPacketSize("GET_VERSION (resp)", 5);

	parse_stat1(miso[0]);

	fprintf(log_fp, "hw: %x ", miso[1]);
	fprintf(log_fp, "usecase: %x ", miso[2]);
	fprintf(log_fp, "fw: %d.%d", miso[3], miso[4]);

	break;


   case READ_BUFFER_8:
	checkPacketSize("READ_BUFFER_8", 4);

	/*
	 * Should read one more byte than the length due to stat1
	 */
	if (mosi[3] != pld_len_in_bytes) {
	    fprintf(log_fp, "ERROR: read length not equal to length from RX_BUFFER_STATUS command: m: %d p: %d ", mosi[3], pld_len_in_bytes);
	    //	    exit(1);
	}

	fprintf(log_fp, "offset: %x length: %d ", mosi[2], mosi[3] );
	set_pending_cmd(READ_BUFFER_8);
	break;

    case RESPONSE(READ_BUFFER_8):
	/*
	 * A stat1 is returned as the first byte of data, so add 1.
	 */
	checkPacketSize("READ_BUFFER_8 (resp)", pld_len_in_bytes + 1);

	parse_stat1(miso[0]);

	fprintf(log_fp, "bytes: ");

	for (i=0; i < (pld_len_in_bytes + 1); i++) {
	    fprintf(log_fp, "%02x ", miso[i + 1]);
	}

	break;

    case READ_DEVICE_PIN:
	/*
	 * The packet sent should be only 2 bytes, but 13 were sent by Semtech. Disable length
	 * check for now.
	 */
	checkPacketSize("READ_DEVICE_PIN", 0);
	set_pending_cmd(READ_DEVICE_PIN);
	break;

    case RESPONSE(READ_DEVICE_PIN):
	checkPacketSize("READ_DEVICE_PIN (resp)", 0);

	parse_stat1(miso[0]);
	fprintf(log_fp, "pins: %08x", get_32(&mosi[1]));

	break;

    case REBOOT:
	checkPacketSize("REBOOT", 3);
	fprintf(log_fp, "stay in bootloader: %d", mosi[2]);
	break;

    case SET_DIO_AS_RF_SWITCH:
	checkPacketSize("SET_DIO_AS_RF_SWITCH", 10);
	fprintf(log_fp, "Enable: %02x, ",  mosi[2]);
	fprintf(log_fp, "SbCfg: %02x, ",   mosi[3]);
	fprintf(log_fp, "RxCfg: %02x, ",   mosi[4]);
	fprintf(log_fp, "TxCfg: %02x, ",   mosi[5]);
	fprintf(log_fp, "TxHpCfg: %02x, ", mosi[6]);
	fprintf(log_fp, "TxHCfg: %02x, ", mosi[6]);
	fprintf(log_fp, "GnssCfg: %02x, ", mosi[8]);
	fprintf(log_fp, "WifiCfg: %02x", mosi[9]);
	break;

    case SET_DIO_IRQ_PARAMS:
	checkPacketSize("SET_DIO_IRQ_PARAMS", 10);

	fprintf(log_fp, "IRQ1: ");
	parse_irq(parser, &mosi[2]);

	fprintf(log_fp, "IRQ2: ");
	parse_irq(parser, &mosi[6]);
	break;

    case SET_REG_MODE:
	checkPacketSize("SET_REG_MODE", 3);

	fprintf(log_fp, "RegMode: %02x ",  mosi[2]);
	break;

    case SET_SLEEP:
	checkPacketSize("SET_SLEEP", 7);
	fprintf(log_fp, "sleepConfig: %0x ", mosi[2]);
	fprintf(log_fp, "time: %x", get_32(&mosi[3]) );

	data->last_command_was_sleep = 1;

	break;

    case SET_TCXO_MODE:
	checkPacketSize("SET_TCXO_MODE", 6);

	delay = get_24(&mosi[3]);

	fprintf(log_fp, "tune: %02x delay: %x ",  mosi[2], delay);
	break;


    case UNKNOWN_0108:
	checkPacketSize("UNKNOWN_0108", 7);

	time_secs = get_32(&mosi[3]);

	fprintf(log_fp, "time_secs: %u (0x%x) delta: %u ", time_secs, time_secs, time_secs - last_time_secs);
	last_time_secs = time_secs;

	break;

    case WRITE_REG_MEM_MASK32:
	checkPacketSize("WRITE_REG_MEM_MASK32", 14);

	fprintf(log_fp, "addr: %x ", get_32(&mosi[2]));
	fprintf(log_fp, "mask: %x ", get_32(&mosi[6]));
	fprintf(log_fp, "data: %x ", get_32(&mosi[10]));
	break;

    case WRITE_BUFFER_8:
	checkPacketSize("WRITE_BUFFER_8", 0);

	for (i=0; i < (data->count - 2); i++) {
	    fprintf(log_fp, "%02x ", mosi[i + 2]);
	}

	break;


    default:
	hdr(parser, data->packet_start_time, "UNHANDLED_SYSTEM_CMD");
	fprintf(log_fp, "Unhandled SYSTEM command: %04x length: %d \n", cmd, data->count);
	exit(1);
    }
}

static void parse_errors(FILE *log_fp, uint16_t errors)
{
    static char *error_str[] = {
			      "LF_RC_CALIB_ERR",             //  0
			      "HF_RC_CAALIB_ERR",            //  1
			      "ADC_CALIB_ERR",           //  2
			      "PLL_CALIB_ERR",           //  3
			      "IMG_CALIB_ERR", //  4
			      "HF_XOSC_START_ERR",    //  5
			      "LF_XOSC_START_ERR",        //  6
			      "PLL_LOCK_ERR",               //  7
			      "RX_ADC_OFFSET_ERR",          //  8
			      "??? 9",      //  9
			      "??? 10",           // 10
			      "??? 11",        // 11
			      "??? 12",            // 12
			      "??? 13",            // 13
			      "??? 14",            // 14
			      "??? 15",            // 15
    };
    uint32_t
	i,
	mask;

    fprintf(log_fp, "errors: %08x ( ", errors);

    for (i=0; (i < 16) && errors; i++) {

	mask = (1 << i);

	if (errors & mask) {
	    fprintf(log_fp, "%s ", error_str[i]);
	    errors &= ~mask;
	}
    }

    fprintf(log_fp, ") ");
}


#define IRQ_TX_DONE           (2)
#define IRQ_PREAMBLE_DETECTED (4)

static void parse_irq(parser_t *parser, uint8_t *miso)
{
    static char *irq_str[] = {
			      "??? 0",             //  0
			      "??? 1",             //  1
			      "TX_DONE",           //  2
			      "RX_DONE",           //  3
			      "PREAMBLE_DETECTED", //  4
			      "SYNCWORD_VALID",    //  5
			      "HEADER_ERR",        //  6
			      "ERR",               //  7
			      "CAD_DONE",          //  8
			      "CAD_DETECTED",      //  9
			      "TIMEOUT",           // 10
			      "LRFHSS_HOP",        // 11
			      "??? 12",            // 12
			      "??? 13",            // 13
			      "??? 14",            // 14
			      "??? 15",            // 15
			      "??? 16",            // 16
			      "??? 17",            // 17
			      "??? 18",            // 18
			      "GNSS_DONE",         // 19
			      "WIFI_DONE",         // 20
			      "LBD",               // 21
			      "CMD_ERROR",         // 22
			      "ERROR",             // 23
			      "FSK_LEN_ERROR",     // 24
			      "FSK_ADDR_ERROR",    // 25
			      "??? 26",            // 26
			      "??? 27",            // 27
			      "??? 28",            // 28
			      "??? 29",            // 29
			      "??? 30",            // 30
			      "??? 31"             // 31
    };
    uint32_t
	i,
	irq,
	mask;
    lr1110_data_t
	*data = parser->data;
    DECLARE_LOG_FP;

    irq = (miso[0] << 24) | (miso[1] << 16) | (miso[2] << 8) | miso[3];

    fprintf(log_fp, "irq: %08x ( ", irq);

    /*
     * If we have a TX_DONE interrupt, track the timestamp of the interrupt going active.
     */
    if ( irq & (1 << IRQ_TX_DONE)) {
	data->tx_done_irq_rise_time = data->irq_rise_time;
    }

    if ( irq & (1 << IRQ_PREAMBLE_DETECTED)) {
	fprintf(log_fp, "Preamble Detected after TX Done: ");
	print_time_nsecs(log_fp, data->irq_rise_time - data->tx_done_irq_rise_time);
    }

    for (i=0; (i < 32) && irq; i++) {

	mask = (1 << i);

	if (irq & mask) {
	    fprintf(log_fp, "%s ", irq_str[i]);
	    irq &= ~mask;
	}
    }

    fprintf(log_fp, ") ");
}
