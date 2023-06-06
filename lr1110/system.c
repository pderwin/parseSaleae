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
      GET_RANDOM           = 0x20,
      GET_DEV_EUI          = 0x25,
      GET_JOIN_EUI         = 0x26,
      READ_DEVICE_PIN      = 0x27,
};

typedef enum {
	      IRQ_TX_DONE           = (1 <<  2),
	      IRQ_RX_DONE           = (1 <<  3),
	      IRQ_PREAMBLE_DETECTED = (1 <<  4),
	      IRQ_SYNCWORD_VALID    = (1 <<  5),
	      IRQ_HEADER_ERR        = (1 <<  6),
	      IRQ_ERR               = (1 <<  7),
	      IRQ_CAD_DONE          = (1 <<  8),
	      IRQ_CAD_DETECTED      = (1 <<  9),
	      IRQ_TIMEOUT           = (1 << 10),
	      IRQ_LRFHSS_HOP        = (1 << 11),
	      IRQ_GNSS_DONE         = (1 << 19),
	      IRQ_WIFI_DONE         = (1 << 20),
	      IRQ_LBD               = (1 << 21),
	      IRQ_CMD_ERROR         = (1 << 22),
	      IRQ_ERROR             = (1 << 23),
	      IRQ_FSK_LEN_ERROR     = (1 << 24),
	      IRQ_FSK_ADDR_ERROR    = (1 << 25),
} irq_e;



static void parse_irq(FILE *log_fp, uint8_t *miso);


void lr1110_system(parser_t *parser)
{
    uint32_t  cmd;
    uint32_t delay;
    uint32_t errors;
    uint32_t i;
    uint8_t  *mosi;
    uint8_t  *miso;
    static uint32_t payload_length  = 0;
    FILE     *log_fp = parser->lf->fp;
    lr1110_data_t *data = parser->data;

    mosi = data->mosi;
    miso = data->miso;

    cmd = get_command(data);

    switch (cmd) {

    case CALIBRATE:
	checkPacketSize("CALIBRATE", 3);

	fprintf(parser->lf->fp, "params: %08x", mosi[2]);
	break;


    case CLEAR_ERRORS:
	checkPacketSize("CLEAR_ERRORS", 2);
	break;

    case CLEAR_IRQ:
	checkPacketSize("CLEAR_IRQ", 6);

	parse_irq(log_fp, &mosi[2]);

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

	errors = (miso[1] << 8) | miso[2];
	fprintf(log_fp, "stat1: %x errors: %x",  miso[0], errors);

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
	checkPacketSize("GET_RANDOM (resp)", 4);

	fprintf(log_fp, "random: %x", get_32(&miso[0]) );
	break;

    case SET_STANDBY:
	checkPacketSize("SET_STANDBY", 3);

	fprintf(log_fp, "StbyConfig: %02x ",  mosi[2]);
	break;


    case GET_STATUS:
	checkPacketSize("GET_STATUS", 6);

	parse_stat1(miso[0]);
	parse_stat2(miso[1]);

	parse_irq(log_fp, &miso[2]);
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

	payload_length = mosi[3];

	fprintf(log_fp, "offset: %x length: %d ", mosi[2], mosi[3] );
	set_pending_cmd(READ_BUFFER_8);
	break;

    case RESPONSE(READ_BUFFER_8):
	/*
	 * A stat1 is returned as the first byte of data, so add 1.
	 */
	checkPacketSize("SYS_READ_BUFFER_8 (resp)", payload_length + 1);

	parse_stat1(miso[0]);

	fprintf(log_fp, "bytes: ");

	for (i=0; i < payload_length; i++) {
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
	fprintf(log_fp, "GnssCfg: %02x, ", mosi[8]);
	fprintf(log_fp, "WifiCfg: %02x", mosi[9]);
	break;

    case SET_DIO_IRQ_PARAMS:
	checkPacketSize("SET_DIO_IRQ_PARAMS", 10);

	fprintf(log_fp, "IRQ1: %02x%02x%02x%02x, ", mosi[2], mosi[3], mosi[4], mosi[5]);
	fprintf(log_fp, "IRQ2: %02x%02x%02x%02x",   mosi[6], mosi[7], mosi[8], mosi[9]);
	break;

    case SET_REG_MODE:
	checkPacketSize("SET_REG_MODE", 3);

	fprintf(log_fp, "RegMode: %02x ",  mosi[2]);
	break;

    case SET_SLEEP:
	checkPacketSize("SET_SLEEP", 7);
	fprintf(log_fp, "sleepConfig: %0x ", mosi[2]);
	fprintf(log_fp, "time: %x", get_32(&mosi[3]) );
	break;

    case SET_TCXO_MODE:
	checkPacketSize("SET_TCXO_MODE", 6);

	delay = get_24(&mosi[3]);

	fprintf(log_fp, "tune: %02x delay: %x ",  mosi[2], delay);
	break;


    case UNKNOWN_0108:
	checkPacketSize("UNKNOWN_0108", 7);
	hex_dump(&mosi[2], 5);
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
	hdr(parser->lf, data->packet_start_time, "UNHANDLED_SYSTEM_CMD");
	fprintf(log_fp, "Unhandled SYSTEM command: %04x length: %d \n", cmd, data->count);
	exit(1);
    }
}

static void parse_irq(FILE *log_fp, uint8_t *miso)
{
    uint32_t bit_num;
    uint32_t irq;

    irq = (miso[0] << 24) | (miso[1] << 16) | (miso[2] << 8) | miso[3];

    fprintf(log_fp, "irq: %08x (", irq);

#define DO_BIT(mask) \
    if (irq & mask) {		       \
	fprintf(log_fp, "%s ", # mask);		\
	irq &= ~mask;				\
    }

    DO_BIT(IRQ_CMD_ERROR);
    DO_BIT(IRQ_ERROR);
    DO_BIT(IRQ_TIMEOUT);
    DO_BIT(IRQ_TX_DONE);
    DO_BIT(IRQ_RX_DONE);
    DO_BIT(IRQ_PREAMBLE_DETECTED);
    DO_BIT(IRQ_SYNCWORD_VALID);
    DO_BIT(IRQ_GNSS_DONE);
    DO_BIT(IRQ_HEADER_ERR);
    DO_BIT(IRQ_ERR);
    DO_BIT(IRQ_CAD_DONE);
    DO_BIT(IRQ_CAD_DETECTED);
    DO_BIT(IRQ_LRFHSS_HOP);
    DO_BIT(IRQ_WIFI_DONE);

    fprintf(log_fp, ") ");

    if (irq) {
	fprintf(log_fp, "\n\nERROR: unparsed irq: %08x.  ", irq);

	for (bit_num = 0; bit_num < 32; bit_num++) {
	    if (irq & (1 << bit_num)) {
		fprintf(log_fp, "bit number: %d \n", bit_num);
		exit(1);
	    }

	}
    }
}
