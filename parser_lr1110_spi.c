#include <string.h>
#include "frame.h"
#include "hdr.h"
#include "parseSaleae.h"
#include "parser.h"
#include "lr1110.h"

// #define VERBOSE 1

/*
 * packet sample frequency to support filtering of i2c_clk signal.
 */
#define SAMPLE_TIME_NSECS (10)

/*
 * Set the time for de-glitch
 */
#define DEGLITCH_TIME (5 * SAMPLE_TIME_NSECS)

typedef struct {
    uint32_t nss;
    uint32_t clk;
    uint32_t mosi;
    uint32_t miso;
    uint32_t busy;
    uint32_t irq;
} sample_t;


typedef struct lr1110_data_s {
    sample_t last_sample;
    sample_t sample;

    uint32_t accumulated_bits;
    uint32_t accumulated_miso_byte;
    uint32_t accumulated_mosi_byte;

    uint32_t accumulated_count;

} lr1110_data_t;


static lr1110_data_t
    lr1110_semtech_data,
    lr1110_n1_data;

#undef  falling_edge
#define falling_edge(__elem) (data->sample.__elem == 0) && (data->last_sample.__elem == 1)
#undef  rising_edge
#define rising_edge(__elem)  (data->sample.__elem == 1) && (data->last_sample.__elem == 0)


static uint32_t     packet_count = 0;
static time_nsecs_t packet_start_time;

#define NUMBER_BYTES (256)

static uint8_t  mosi[NUMBER_BYTES];
static uint8_t  miso[NUMBER_BYTES];

static void parse_packet (parser_t *parser);

static void clear_accumulator (lr1110_data_t *data)
{
    data->accumulated_bits = 0;

    data->accumulated_mosi_byte = 0;
    data->accumulated_miso_byte = 0;
}

static void clear_packet (lr1110_data_t *data)
{
    data->accumulated_count = 0;

    clear_accumulator(data);
}

static void process_frame (parser_t *parser, frame_t *frame)
{
    lr1110_data_t *data;

    data = parser->data;

    /*
     * If we've started a new packet, then clear accumulation.
     */
    if (falling_edge(nss)) {

	packet_start_time = frame->time_nsecs;

	clear_packet(data);

	/*
	 * Check for device being busy
	 */
	if (data->sample.busy) {
	    hdr(parser->lf, frame->time_nsecs, "NSS_ERROR");
	    fprintf(parser->lf->fp, "ERROR: nss dropped while device was busy\n");
	    //	    exit(1);
	}

    }

    /*
     * If done with a packet, then parse it.
     */
    if (rising_edge(nss)) {
	parse_packet(parser);
	memcpy(&data->last_sample, &data->sample, sizeof(sample_t));
	return;
    }

    if (rising_edge(irq)) {
	hdr(parser->lf, frame->time_nsecs, "*** IRQ rise ***");
	fprintf(parser->lf->fp, "\n");
    }

    if (falling_edge(irq)) {
	hdr(parser->lf, frame->time_nsecs, "*** IRQ fall ***");
	fprintf(parser->lf->fp, "\n");
    }
#if 0
    if (falling_edge(busy)) {
	hdr(my_parser.lf, frame->time_nsecs, "*** BUSY fall ***");
	fprintf(my_parser.lf->fp, "\n");
    }
#endif

    /*
     * Get bits
     */
    if (rising_edge(clk)) {

	if (data->sample.nss == 0) {

	    data->accumulated_mosi_byte |= (data->sample.mosi << (7 - data->accumulated_bits));
	    data->accumulated_miso_byte |= (data->sample.miso << (7 - data->accumulated_bits));

	    data->accumulated_bits++;

	    if (data->accumulated_bits == 8) {
#ifdef VERBOSE
		fprintf(my_parser.lf->fp, ">>>>  LR1110 byte: %02x %02x \n", accumulated_mosi_byte, accumulated_miso_byte);
#endif

		miso[data->accumulated_count] = data->accumulated_miso_byte;
		mosi[data->accumulated_count] = data->accumulated_mosi_byte;

		data->accumulated_count++;

		clear_accumulator(data);
	    }
	}
    }

    memcpy(&data->last_sample, &data->sample, sizeof(sample_t));
}

#define checkPacketSize(__str, __size) _checkPacketSize(parser, __str, __size)

static void _checkPacketSize(parser_t *parser, char *str, uint32_t size)
{
    lr1110_data_t *data = parser->data;

    hdr_with_lineno(parser->lf, packet_start_time, str, packet_count);

    if (size) {
	if (data->accumulated_count != size) {
	    fprintf(parser->lf->fp, "<Length error.  Expected: %d bytes, got: %d > ", size, data->accumulated_count);
	    exit(1);
	}
    }
}

static uint32_t pending_cmd;


#define clear_pending_cmd(__cmd) _clear_pending_cmd(log_fp, __cmd)

static void _clear_pending_cmd(FILE *log_fp, uint32_t cmd)
{
    if (!pending_cmd) {
	fprintf(log_fp, "<Clear Pending Command Error.  cmd: %x > ", cmd);
	exit(1);
    }

    pending_cmd = 0;
}

static uint32_t get_16(uint8_t *mosi)
{
    uint32_t val;

    val = (mosi[0] << 8) | mosi[1];

    return val;
}

static uint32_t get_24(uint8_t *mosi)
{
    uint32_t val;

    val = (mosi[0] << 16) | (mosi[1] << 8) | mosi[2];

    return val;
}

static uint32_t get_32(uint8_t *mosi)
{
    uint32_t val;

    val = (mosi[0] << 24) | (mosi[1] << 16) | (mosi[2] << 8) | mosi[3];

    return val;
}

#define set_pending_cmd(__cmd) _set_pending_cmd(log_fp, __cmd)

static void _set_pending_cmd(FILE *log_fp, uint32_t cmd)
{
    if (pending_cmd) {
	fprintf(log_fp, "<Pending Command Error.  pending: %x cmd: %x > ", pending_cmd, cmd);
	exit(1);
    }

    pending_cmd = (cmd | PENDING);
}

static void parse_irq(FILE *log_fp, uint8_t *miso)
{
   uint32_t irq;

   irq = (miso[0] << 24) | (miso[1] << 16) | (miso[2] << 8) | miso[3];

   fprintf(log_fp, "irq: %08x (", irq);

#define DO_BIT(mask) \
   if (irq & mask) { \
       fprintf(log_fp, "%s ", # mask); \
       irq &= ~mask; \
   }

   DO_BIT(IRQ_CMD_ERROR);
   DO_BIT(IRQ_ERROR);
   DO_BIT(IRQ_TIMEOUT);
   DO_BIT(IRQ_TX_DONE);
   DO_BIT(IRQ_RX_DONE);
   DO_BIT(IRQ_PREAMBLE_DETECTED);
   DO_BIT(IRQ_SYNCWORD_VALID);

   fprintf(log_fp, ") ");

   if (irq) {
       fprintf(log_fp, "unparsed irq: %08x\n", irq);
       exit(1);
   }
}

static void parse_stat1(FILE *log_fp, uint8_t stat1)
{
    char *cmd_str[] = {
		       "CMD_FAIL",
		       "CMD_PERR",
		       "CMD_OK",
		       "CMD_DAT"
    };

    uint32_t cmd_status;

    fprintf(log_fp, "stat1: %02x [ ", stat1);

    cmd_status = (stat1 >> 1) & 3;

    fprintf(log_fp, "%s ", cmd_str[cmd_status]);

    if (stat1 & 1) {
	fprintf(log_fp, "IRQ");
    }

    fprintf(log_fp, "] ");

}

#define parse_stat2(__stat2) _parse_stat2(log_fp, __stat2)

static void _parse_stat2(FILE *log_fp, uint8_t stat2)
{
    char     *reset_status_str = "???";
    uint32_t reset_status;
    uint32_t chip_mode;
    static char *chip_mode_str[] = {
				    "Sleep",            // 0
				    "Stby w/ RC Osc",   // 1
				    "Stby w/ Xtal Osc", // 2
				    "FS",               // 3
				    "RX",               // 4
				    "TX",               // 5
				    "WiFi or GNSS",     // 6
				    "???"               // 7
    };

    fprintf(log_fp, "stat2: %02x ", stat2);

    reset_status = (stat2 >> 4) & 0xf;

    switch(reset_status) {

    case 0:
	reset_status_str = "Cleared";
	break;
    case 3:
	reset_status_str = "SYS_RESET";
	break;

    default:
	fprintf(log_fp, "unknown reset status: %x\n", reset_status);
	exit(1);
    }

    fprintf(log_fp, "reset_status: %x (%s) ", reset_status, reset_status_str);

    chip_mode = (stat2 >> 1) & 0x7;

    fprintf(log_fp, "chip mode: %x (%s) ", chip_mode, chip_mode_str[chip_mode] );

    if (stat2 & 1) {
	fprintf(log_fp, "FLASH ");
    }
}



/*-------------------------------------------------------------------------
 *
 * name:        parse_packet
 *
 * description:
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
static void parse_packet (parser_t *parser)
{
    uint32_t cmd;
    uint32_t delay;
    uint32_t errors;
    uint32_t freq;
    uint32_t i;
    //    uint32_t irq;
    uint32_t timeout;
    static uint32_t payload_length  = 0;
    static packet_type_e packet_type = PACKET_TYPE_NONE;
    FILE *log_fp;
    lr1110_data_t *data = parser->data;

    log_fp = parser->lf->fp;

    if (data->accumulated_count < 2) {
	return;
    }

    if (pending_cmd) {
	cmd = pending_cmd;
    }
    else {
	cmd = (mosi[0] << 8) | mosi[1];
    }

    packet_count++;

    switch(cmd) {

    case 0:
	checkPacketSize("---- 00's ----", 0);
	break;

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

    case CONFIG_LF_CLOCK:
	checkPacketSize("CONFIG_LF_CLOCK", 3);

	fprintf(log_fp, "wait xtal ready: %d clock: %d ", (mosi[2] >> 2) & 1, mosi[2] & 3);
	break;

    case CRYPTO_COMPUTE_AES_CMAC:
	checkPacketSize("CRYPTO_COMPUTE_AES_CMAC", 0);

	fprintf(log_fp, "Bytes: \n");
	for (i=0; i<13; i++) {
	    fprintf(log_fp, "   %2d: %x \n", i, mosi[i]);
	}
	break;

    case CRYPTO_DERIVE_AND_STORE_KEY:
	checkPacketSize("CRYPTO_DERIVE_AND_STORE_KEY", 20);
	break;

    case CRYPTO_RESTORE_FROM_FLASH:
	checkPacketSize("CRYPTO_RESTORE_FROM_FLASH", 2);
	break;

    case CRYPTO_SET_KEY:
	checkPacketSize("CRYPTO_SET_KEY", 19);
	break;
    case (CRYPTO_SET_KEY | PENDING):
	checkPacketSize("CRYPTO_SET_KEY", 19);

	clear_pending_cmd(CRYPTO_SET_KEY);

	break;

    case CRYPTO_STORE_TO_FLASH:
	checkPacketSize("CRYPTO_STORE_TO_FLASH", 2);
	break;

    case GET_DEV_EUI:
	checkPacketSize("GET_DEV_EUI", 2);
	set_pending_cmd(GET_DEV_EUI);
	break;

    case ( GET_DEV_EUI | PENDING ):
	checkPacketSize("GET_DEV_EUI (resp)", 9);

	parse_stat1(log_fp, miso[0]);

	for (i=0; i<8; i++) {
	    fprintf(log_fp, "%02x ", miso[i+1]);
	}

	clear_pending_cmd(GET_DEV_EUI);
	break;

    case GET_ERRORS:
	checkPacketSize("GET_ERRORS", 2);
	set_pending_cmd(GET_ERRORS);
	break;

    case (GET_ERRORS | PENDING):
	checkPacketSize("GET_ERRORS (resp)", 3);

	errors = (miso[1] << 8) | miso[2];
	fprintf(log_fp, "stat1: %x errors: %x",  miso[0], errors);

	clear_pending_cmd(GET_ERRORS);
	break;

    case GET_JOIN_EUI:
	checkPacketSize("GET_JOIN_EUI", 2);
	set_pending_cmd(GET_JOIN_EUI);
	break;

    case ( GET_JOIN_EUI | PENDING ):
	checkPacketSize("GET_JOIN_EUI (resp)", 9);

	parse_stat1(log_fp, miso[0]);

	for (i=0; i<8; i++) {
	    fprintf(log_fp, "%02x ", miso[i+1]);
	}

	clear_pending_cmd(GET_DEV_EUI);
	break;

    case GET_PACKET_STATUS:
	checkPacketSize("GET_PACKET_STATUS", 2);
	set_pending_cmd(GET_PACKET_STATUS);
	break;

    case (GET_PACKET_STATUS | PENDING):
	checkPacketSize("GET_PACKET_STATUS (resp)", 4);

	parse_stat1(log_fp, miso[0]);

	fprintf(log_fp, "rssipkt: %x",         miso[1]);
	fprintf(log_fp, "snrpkt: %x",          miso[2]);
	fprintf(log_fp, "signal_rssi_pkt: %x", miso[3]);

	clear_pending_cmd(GET_PACKET_TYPE);
	break;

    case GET_PACKET_TYPE:
	checkPacketSize("GET_PACKET_TYPE", 2);
	set_pending_cmd(GET_PACKET_TYPE);
	break;

    case (GET_PACKET_TYPE | PENDING):
	checkPacketSize("GET_PACKET_TYPE (resp)", 2);

	fprintf(log_fp, "type: %x", miso[1]);

	clear_pending_cmd(GET_PACKET_TYPE);
	break;

    case GET_RANDOM:
	checkPacketSize("GET_RANDOM", 2);
	set_pending_cmd(GET_RANDOM);
	break;

    case (GET_RANDOM | PENDING):
	checkPacketSize("GET_RANDOM (resp)", 4);

	fprintf(log_fp, "random: %x", get_32(&miso[0]) );
	clear_pending_cmd(GET_RANDOM);
	break;

    case GET_RX_BUFFER_STATUS:
	checkPacketSize("GET_RX_BUFFER_STATUS", 2);
	set_pending_cmd(GET_RX_BUFFER_STATUS);
	break;

    case ( GET_RX_BUFFER_STATUS | PENDING):
	checkPacketSize("GET_RX_BUFFER_STATUS (resp)", 3);

	parse_stat1(log_fp, miso[0]);

	fprintf(log_fp, "payload length: %d ", miso[1] );
	fprintf(log_fp, "RX buffer start: %x ", miso[2] );

	clear_pending_cmd(GET_RX_BUFFER_STATUS);
	break;

    case GET_STATS:
	checkPacketSize("GET_STATS", 2);
	set_pending_cmd(GET_STATS);
	break;

    case (GET_STATS | PENDING):
	checkPacketSize("GET_STATS (resp)", 9);

	parse_stat1(log_fp, miso[0]);

	fprintf(log_fp, "nbPackets: %d ", get_16(&miso[1]) );
	fprintf(log_fp, "crc error: %d ", get_16(&miso[3]) );
	fprintf(log_fp, "hdr error: %d ", get_16(&miso[5]) );
	fprintf(log_fp, "false sync: %d", get_16(&miso[7]) );

	clear_pending_cmd(GET_STATS);
	break;

    case GET_STATUS:
	checkPacketSize("GET_STATUS", 6);

	parse_stat1(log_fp, miso[0]);
	parse_stat2(miso[1]);
	parse_irq(log_fp, &miso[2]);
	break;

    case GET_VBAT:
	checkPacketSize("GET_VBAT", 2);
	set_pending_cmd(GET_VBAT);
	break;

    case ( GET_VBAT | PENDING ):
	{

	    float vBat;

	    checkPacketSize("GET_VBAT (resp)", 2);

	    parse_stat1(log_fp, miso[0]);

	    /*
	     * Using equation from spec:
	     */
	    vBat = (miso[1] * 5.0) / 255;

	    vBat -= 1;
	    vBat *= 1.35;


	    fprintf(log_fp, "vBat: %5.2f ", vBat);

	    clear_pending_cmd(GET_VBAT);
	}
	break;

    case GET_VERSION:
	checkPacketSize("GET_VERSION", 2);
	set_pending_cmd(GET_VERSION);
	break;

    case (GET_VERSION | PENDING):
	checkPacketSize("GET_VERSION (resp)", 5);

	parse_stat1(log_fp, miso[0]);

	fprintf(log_fp, "hw: %x ", miso[1]);
	fprintf(log_fp, "usecase: %x ", miso[2]);
	fprintf(log_fp, "fw: %d.%d", miso[3], miso[4]);

	clear_pending_cmd(GET_VERSION);
	break;

    case LORA_SYNCH_TIMEOUT:
	checkPacketSize("LORA_SYNCH_TIMEOUT", 3);
	fprintf(log_fp, "symbol: %02x ( should be zero )", mosi[2]);
	break;

   case READ_BUFFER_8:
	checkPacketSize("READ_BUFFER_8", 4);

	payload_length = mosi[3];

	fprintf(log_fp, "offset: %x length: %x ", mosi[2], mosi[3] );
	set_pending_cmd(READ_BUFFER_8);
	break;

    case ( READ_BUFFER_8 | PENDING):
	/*
	 * A stat1 is returned as the first byte of data, so add 1.
	 */
	checkPacketSize("READ_BUFFER_8 (resp)", payload_length + 1);

	fprintf(log_fp, "bytes: ");

	for (i=0; i < (payload_length + 1); i++) {
	    fprintf(log_fp, "%02x ", miso[i]);
	}

	clear_pending_cmd(READ_BUFFER_8);
	break;

    case READ_DEVICE_PIN:
	/*
	 * The packet sent should be only 2 bytes, but 13 were sent by Semtech. Disable length
	 * check for now.
	 */
	checkPacketSize("READ_DEVICE_PIN", 0);
	set_pending_cmd(READ_DEVICE_PIN);
	break;

    case (READ_DEVICE_PIN | PENDING):
	checkPacketSize("READ_DEVICE_PIN (resp)", 0);

	parse_stat1(log_fp, miso[0]);
	fprintf(log_fp, "pins: %08x", get_32(&mosi[1]));

	clear_pending_cmd(READ_DEVICE_PIN);
	break;

    case REBOOT:
	checkPacketSize("REBOOT", 3);
	fprintf(log_fp, "stay in bootloader: %d", mosi[2]);
	break;

    case SEMTECH_UNKNOWN_0229:

	checkPacketSize("SEMTECH_UNKNOWN_0229", 13);

	fprintf(log_fp, "Bytes: ");

	for (i=0; i<13; i++) {
	    fprintf(log_fp, "%02x ", mosi[i]);
	}

	break;

    case SEMTECH_UNKNOWN_022b:
	checkPacketSize("SEMTECH_UNKNOWN_022b", 3);
	fprintf(log_fp, "Bytes: \n");
	for (i=0; i<3; i++) {
	    fprintf(log_fp, "   %2d: %x \n", i, mosi[i]);
	}

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

    case SET_LORA_PUBLIC_NETWORK:
	checkPacketSize("SET_LORA_PUBLIC_NETWORK", 3);
	fprintf(log_fp, "network: %02x", mosi[2]);
	break;

    case SET_MODULATION_PARAM:

	checkPacketSize("SET_MODULATION_PARAM", 6);

	fprintf(log_fp, "SF: %02x ", mosi[2]);
	fprintf(log_fp, "BWL: %02x ", mosi[3]);
	fprintf(log_fp, "CR: %02x ", mosi[4]);
	fprintf(log_fp, "lowDROpt: %02x", mosi[5]);
	break;

    case SET_PA_CFG:
	checkPacketSize("SET_PA_CFG", 6);

	fprintf(log_fp, "PaSel: %02x, ",  mosi[2]);
	fprintf(log_fp, "RegPaSupply: %02x, ",   mosi[3]);
	fprintf(log_fp, "PaDutyCycle: %02x, ",   mosi[4]);
	fprintf(log_fp, "PaHpSel: %02x",   mosi[5]);
	break;

    case SET_PACKET_PARAM:

	switch(packet_type) {

	default:
	    fprintf(log_fp, "SET_PACKET_PARAM: unknown packet type: %x.  Assume LORA\n", packet_type);

	    /* fall-through */

	case PACKET_TYPE_LORA:
	    checkPacketSize("SET_PACKET_PARAM (LORA)", 8);
	    fprintf(log_fp, "PbLength: %04x ",  (mosi[2] << 8) | mosi[3]);
	    fprintf(log_fp, "HeaderType: %02x ",  mosi[4]);

	    payload_length = mosi[5];

	    fprintf(log_fp, "PayloadLen: %02x ",  mosi[5]);
	    fprintf(log_fp, "CRC: %02x ",  mosi[6]);
	    fprintf(log_fp, "InvertIQ: %02x ",  mosi[7]);
	    break;
	}
	break;

    case SET_PACKET_TYPE:
	checkPacketSize("SET_PACKET_TYPE", 3);

	packet_type = mosi[2];

	fprintf(log_fp, "PacketType: %02x ", packet_type);
	break;

    case SET_REG_MODE:
	checkPacketSize("SET_REG_MODE", 3);

	fprintf(log_fp, "RegMode: %02x ",  mosi[2]);
	break;

    case SET_RF_FREQUENCY:
	checkPacketSize("SET_RF_FREQUENCY", 6);

	freq = (mosi[2] << 24) | (mosi[3] << 16) | (mosi[4] << 8) | (mosi[5] << 0);
	fprintf(log_fp, "Frequency: %04x (%d)",  freq, freq);
	break;

    case SET_RX:
	checkPacketSize("SET_RX", 5);

	timeout = (mosi[2] << 16) | (mosi[3] << 8) | mosi[4];

	fprintf(log_fp, "timeout: 0x%x (%d)", timeout, timeout);
	break;

    case SET_RX_BOOSTED:
	checkPacketSize("SET_RX_BOOSTED", 3);
	fprintf(log_fp, "boosted: %02x", mosi[2]);
	break;

    case SET_SLEEP:
	checkPacketSize("SET_SLEEP", 7);
	fprintf(log_fp, "sleepConfig: %0x ", mosi[2]);
	fprintf(log_fp, "time: %x", get_32(&mosi[3]) );
	break;

    case SET_STANDBY:
	checkPacketSize("SET_STANDBY", 3);

	fprintf(log_fp, "StbyConfig: %02x ",  mosi[2]);
	break;

    case SET_TCXO_MODE:
	checkPacketSize("SET_TCXO_MODE", 6);

	delay = get_24(&mosi[3]);

	fprintf(log_fp, "tune: %02x delay: %x ",  mosi[2], delay);
	break;

    case SET_TX:
	checkPacketSize("SET_TX", 5);

	timeout = (mosi[2] << 16) | (mosi[3] << 8) | mosi[4];

	fprintf(log_fp, "timeout: 0x%x (%d)", timeout, timeout);
	break;

    case SET_TXCW:
	checkPacketSize("SET_TXCW", 2);
	break;

    case SET_TX_PARAMS:
	checkPacketSize("SET_TX_PARAMS", 4);
	fprintf(log_fp, "TxPower: %02x ",  mosi[2]);
	fprintf(log_fp, "RampTime: %02x ",  mosi[3]);
	break;

    case STOP_TIMEOUT_ON_PREAMBLE:
	checkPacketSize("STOP_TIMEOUT_ON_PREAMBLE", 3);
	fprintf(log_fp, "stop: %02x", mosi[2]);
	break;

    case WRITE_BUFFER_8:
	checkPacketSize("WRITE_BUFFER_8", payload_length + 2);

	for (i=0; i< payload_length; i++) {
	    fprintf(log_fp, "%02x ", mosi[i + 2]);
	}

	break;

    case WRITE_REG_MEM_MASK32:
	checkPacketSize("WRITE_REG_MEM_MASK32", 14);

	fprintf(log_fp, "addr: %x ", get_32(&mosi[2]));
	fprintf(log_fp, "mask: %x ", get_32(&mosi[6]));
	fprintf(log_fp, "data: %x ", get_32(&mosi[10]));
	break;

    default:
	hdr(parser->lf, packet_start_time, "UNHANDLED_CMD");
	fprintf(log_fp, "Unhandled command: %04x length: %d \n", cmd, data->accumulated_count);
	//	exit(1);
    }

    fprintf(log_fp, "\n");
}


static signal_t signals_semtech[] =
    {
     { "nss",  &lr1110_semtech_data.sample.nss, .deglitch_nsecs = 50 },
     { "clk",  &lr1110_semtech_data.sample.clk, .deglitch_nsecs = 50 },
     { "mosi", &lr1110_semtech_data.sample.mosi},
     { "miso", &lr1110_semtech_data.sample.miso},
     { "busy", &lr1110_semtech_data.sample.busy},
     { "irq",  &lr1110_semtech_data.sample.irq},
     { NULL, NULL}
    };

static signal_t signals_n1[] =
    {
     { "n1_nss",  &lr1110_n1_data.sample.nss, .deglitch_nsecs = 50 },
     { "n1_clk",  &lr1110_n1_data.sample.clk, .deglitch_nsecs = 50 },
     { "n1_mosi", &lr1110_n1_data.sample.mosi},
     { "n1_miso", &lr1110_n1_data.sample.miso},
     { "n1_busy", &lr1110_n1_data.sample.busy},
     { "n1_irq",  &lr1110_n1_data.sample.irq},
     { NULL, NULL}
    };


static parser_t my_parser =
    {
     .name          = "lr1110_semtech",
     .signals       = signals_semtech,
     .process_frame = process_frame,
     .log_file      = "lr1110_semtech.log",

     .data          = &lr1110_semtech_data,

     /*
      * To be able to filter glitches on the clock, we want to get events at
      * least every 50 nSecs
      */
     .sample_time_nsecs = SAMPLE_TIME_NSECS,
    };

static parser_t n1_parser =
    {
     .name          = "lr1110_n1",
     .signals       = signals_n1,
     .process_frame = process_frame,
     .log_file      = "lr1110_n1.log",

     .data          = &lr1110_n1_data,

     /*
      * To be able to filter glitches on the clock, we want to get events at
      * least every 50 nSecs
      */
     .sample_time_nsecs = SAMPLE_TIME_NSECS,

    };

static void CONSTRUCTOR init (void)
{
    parser_register(&my_parser);
    parser_register(&n1_parser);
}
