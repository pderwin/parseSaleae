#include <string.h>
#include "frame.h"
#include "hdr.h"
#include "parseSaleae.h"
#include "parser.h"
#include "lr1110.h"

// #define VERBOSE 1

static void parse_packet (uint32_t count);

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

static sample_t     last_sample;
static sample_t     sample;
static uint32_t     accumulated_bits;
static uint32_t     accumulated_mosi_byte;
static uint32_t     accumulated_miso_byte;
static parser_t     my_parser;
static uint32_t     packet_count = 0;
static time_nsecs_t packet_start_time;

#define NUMBER_BYTES (256)

static uint32_t accumulated_count;
static uint8_t  mosi[NUMBER_BYTES];
static uint8_t  miso[NUMBER_BYTES];

static void clear_accumulator (void)
{
    accumulated_bits = 0;

    accumulated_mosi_byte = 0;
    accumulated_miso_byte = 0;
}

static void clear_packet (void)
{
    accumulated_count = 0;

    clear_accumulator();
}

static void process_frame (frame_t *frame)
{
#if 0
    {
	static uint32_t current_spi_clk = 0;
	static uint32_t countdown       = 0;
	static uint32_t first_frame     = 1;

	/*
	 * To keep from thinking we have a rising edge of clock at the beginning of debounce, we need to set our
	 * 'current' to our first entry.
	 */
	if (first_frame) {
	    current_spi_clk = last_sample.clk = sample.clk;
	    first_frame = 0;
	    return;
	}
	/*
	 * The crisp clock is essential to our parsing ability.  Filter it for 250
	 * nSecs.
	 *
	 * When it changes states, set counter and just return.
	 */
	if (sample.clk != current_spi_clk) {

	    if (countdown == 0) {
		countdown = DEGLITCH_TIME / SAMPLE_TIME_NSECS;
		return;
	    }
	    else {
		/*
		 * Here, we already have a countdown value, so if we decrement it and
		 * it becomes 0, then we can continue.  When non-zero, just return.
		 */
		if (--countdown) {
		    return;
		}
	    }

	    /*
	     * If we get here, then we have debounced it.  Remember current state.
	     */
	    current_spi_clk = sample.clk;

	} // sample != current
    }
#endif




    /*
     * If we've started a new packet, then clear accumulation.
     */
    if (falling_edge(nss)) {

	packet_start_time = frame->time_nsecs;

	clear_packet();

	/*
	 * Check for device being busy
	 */
	if (sample.busy) {
	    hdr(my_parser.lf, frame->time_nsecs, "NSS_ERROR");
	    fprintf(my_parser.lf->fp, "ERROR: nss dropped while device was busy\n");
	    //	    exit(1);
	}

    }

    /*
     * If done with a packet, then parse it.
     */
    if (rising_edge(nss)) {
	parse_packet(accumulated_count);
	memcpy(&last_sample, &sample, sizeof(sample_t));
	return;
    }

    if (rising_edge(irq)) {
	hdr(my_parser.lf, frame->time_nsecs, "*** IRQ rise ***");
	fprintf(my_parser.lf->fp, "\n");
    }

    if (falling_edge(irq)) {
	hdr(my_parser.lf, frame->time_nsecs, "*** IRQ fall ***");
	fprintf(my_parser.lf->fp, "\n");
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
	// static uint64_t last_time_nsecs = 0;
	//	fprintf(my_parser.lf->fp, "%lld   SCLK rise  delta: %lld\n", frame->time_nsecs, frame->time_nsecs - last_time_nsecs);

	// last_time_nsecs = frame->time_nsecs;

	    if (sample.nss == 0) {

	    //	    printf("SCB1.CLK rising edge\n");

	    accumulated_mosi_byte |= (sample.mosi << (7 - accumulated_bits));
	    accumulated_miso_byte |= (sample.miso << (7 - accumulated_bits));

	    accumulated_bits++;

	    if (accumulated_bits == 8) {
#ifdef VERBOSE
		fprintf(my_parser.lf->fp, ">>>>  LR1110 byte: %02x %02x \n", accumulated_mosi_byte, accumulated_miso_byte);
#endif

		miso[accumulated_count] = accumulated_miso_byte;
		mosi[accumulated_count] = accumulated_mosi_byte;

		accumulated_count++;

		clear_accumulator();
	    }
	}
    }

    memcpy(&last_sample, &sample, sizeof(sample_t));
}

static void checkPacketSize(char *str, uint32_t size)
{
    hdr_with_lineno(my_parser.lf, packet_start_time, str, packet_count);

    if (size) {
	if (accumulated_count != size) {
	    fprintf(my_parser.lf->fp, "<Length error.  Expected: %d bytes, got: %d > ", size, accumulated_count);
	    exit(1);
	}
    }
}

static uint32_t pending_cmd;

static void clear_pending_cmd(uint32_t cmd)
{
    if (!pending_cmd) {
	fprintf(my_parser.lf->fp, "<Clear Pending Command Error.  cmd: %x > ", cmd);
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

static void set_pending_cmd(uint32_t cmd)
{
    if (pending_cmd) {
	fprintf(my_parser.lf->fp, "<Pending Command Error.  pending: %x cmd: %x > ", pending_cmd, cmd);
	exit(1);
    }

    pending_cmd = (cmd | PENDING);
}

static void parse_irq(uint8_t *miso)
{
   uint32_t irq;

   irq = (miso[0] << 24) | (miso[1] << 16) | (miso[2] << 8) | miso[3];

   fprintf(my_parser.lf->fp, "irq: %08x (", irq);

#define DO_BIT(mask) \
   if (irq & mask) { \
       fprintf(my_parser.lf->fp, "%s ", # mask); \
       irq &= ~mask; \
   }

   DO_BIT(IRQ_CMD_ERROR);
   DO_BIT(IRQ_ERROR);
   DO_BIT(IRQ_TIMEOUT);
   DO_BIT(IRQ_TX_DONE);

   fprintf(my_parser.lf->fp, ") ");

   if (irq) {
       fprintf(my_parser.lf->fp, "unparsed irq: %08x\n", irq);
       exit(1);
   }
}

static void parse_stat1(uint8_t stat1)
{
    char *cmd_str[] = {
		       "CMD_FAIL",
		       "CMD_PERR",
		       "CMD_OK",
		       "CMD_DAT"
    };

    uint32_t cmd_status;

    fprintf(my_parser.lf->fp, "stat1: %02x [ ", stat1);

    cmd_status = (stat1 >> 1) & 3;

    fprintf(my_parser.lf->fp, "%s ", cmd_str[cmd_status]);

    if (stat1 & 1) {
	fprintf(my_parser.lf->fp, "IRQ");
    }

    fprintf(my_parser.lf->fp, "] ");

}

static void parse_stat2(uint8_t stat2)
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

    fprintf(my_parser.lf->fp, "stat2: %02x ", stat2);

    reset_status = (stat2 >> 4) & 0xf;

    switch(reset_status) {

    case 0:
	reset_status_str = "Cleared";
	break;
    case 3:
	reset_status_str = "SYS_RESET";
	break;

    default:
	fprintf(my_parser.lf->fp, "unknown reset status: %x\n", reset_status);
	exit(1);
    }

    fprintf(my_parser.lf->fp, "reset_status: %x (%s) ", reset_status, reset_status_str);

    chip_mode = (stat2 >> 1) & 0x7;

    fprintf(my_parser.lf->fp, "chip mode: %x (%s) ", chip_mode, chip_mode_str[chip_mode] );

    if (stat2 & 1) {
	fprintf(my_parser.lf->fp, "FLASH ");
    }
}



static void parse_packet (uint32_t count)
{
    uint32_t cmd;
    uint32_t delay;
    uint32_t errors;
    uint32_t freq;
    uint32_t i;
    uint32_t irq;
    uint32_t timeout;
    static uint32_t payload_length  = 0;
    static packet_type_e packet_type = PACKET_TYPE_NONE;

    if (count < 2) {
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

	fprintf(my_parser.lf->fp, "params: %08x", mosi[2]);
	break;

    case CLEAR_ERRORS:
	checkPacketSize("CLEAR_ERRORS", 2);
	break;

    case CLEAR_IRQ:
	checkPacketSize("CLEAR_IRQ", 6);

	irq = (mosi[2] << 24) | (mosi[3] << 16) | (mosi[4] << 8) | (mosi[5] << 0) ;

	fprintf(my_parser.lf->fp, "irq_to_clear: %08x", irq);

	break;

    case CONFIG_LF_CLOCK:
	checkPacketSize("CONFIG_LF_CLOCK", 3);

	fprintf(my_parser.lf->fp, "wait xtal ready: %d clock: %d ", (mosi[2] >> 2) & 1, mosi[2] & 3);
	break;

    case CRYPTO_COMPUTE_AES_CMAC:
	checkPacketSize("CRYPTO_COMPUTE_AES_CMAC", 0);

	fprintf(my_parser.lf->fp, "Bytes: \n");
	for (i=0; i<13; i++) {
	    fprintf(my_parser.lf->fp, "   %2d: %x \n", i, mosi[i]);
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

	parse_stat1(miso[0]);

	for (i=0; i<8; i++) {
	    fprintf(my_parser.lf->fp, "%02x ", miso[i+1]);
	}

	clear_pending_cmd(GET_DEV_EUI);
	break;

    case GET_JOIN_EUI:
	checkPacketSize("GET_JOIN_EUI", 2);
	set_pending_cmd(GET_JOIN_EUI);
	break;

    case ( GET_JOIN_EUI | PENDING ):
	checkPacketSize("GET_JOIN_EUI (resp)", 9);

	parse_stat1(miso[0]);

	for (i=0; i<8; i++) {
	    fprintf(my_parser.lf->fp, "%02x ", miso[i+1]);
	}

	clear_pending_cmd(GET_DEV_EUI);
	break;

    case GET_PACKET_TYPE:
	checkPacketSize("GET_PACKET_TYPE", 2);
	set_pending_cmd(GET_PACKET_TYPE);
	break;

    case (GET_PACKET_TYPE | PENDING):
	checkPacketSize("GET_PACKET_TYPE (resp)", 2);

	fprintf(my_parser.lf->fp, "type: %x", miso[1]);

	clear_pending_cmd(GET_PACKET_TYPE);
	break;

    case GET_ERRORS:
	checkPacketSize("GET_ERRORS", 2);
	set_pending_cmd(GET_ERRORS);
	break;

    case (GET_ERRORS | PENDING):
	checkPacketSize("GET_ERRORS (resp)", 3);

	errors = (miso[1] << 8) | miso[2];
	fprintf(my_parser.lf->fp, "stat1: %x errors: %x",  miso[0], errors);

	clear_pending_cmd(GET_ERRORS);
	break;

    case GET_RANDOM:
	checkPacketSize("GET_RANDOM", 2);
	set_pending_cmd(GET_RANDOM);
	break;

    case (GET_RANDOM | PENDING):
	checkPacketSize("GET_RANDOM (resp)", 4);

	fprintf(my_parser.lf->fp, "random: %x", get_32(&miso[0]) );
	clear_pending_cmd(GET_RANDOM);
	break;

    case GET_STATS:
	checkPacketSize("GET_STATS", 2);
	set_pending_cmd(GET_STATS);
	break;

    case (GET_STATS | PENDING):
	checkPacketSize("GET_STATS (resp)", 9);

	parse_stat1(miso[0]);

	fprintf(my_parser.lf->fp, "nbPackets: %d ", get_16(&miso[1]) );
	fprintf(my_parser.lf->fp, "crc error: %d ", get_16(&miso[3]) );
	fprintf(my_parser.lf->fp, "hdr error: %d ", get_16(&miso[5]) );
	fprintf(my_parser.lf->fp, "false sync: %d", get_16(&miso[7]) );

	clear_pending_cmd(GET_STATS);
	break;

    case GET_STATUS:
	checkPacketSize("GET_STATUS", 6);

	irq = (miso[2] << 24) | (miso[3] << 16) | (miso[4] << 8) | (miso[5] << 0);

	parse_stat1(miso[0]);
	parse_stat2(miso[1]);
	parse_irq(&miso[2]);
	break;

    case GET_VBAT:
	checkPacketSize("GET_VBAT", 2);
	set_pending_cmd(GET_VBAT);
	break;

    case ( GET_VBAT | PENDING ):
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


	    fprintf(my_parser.lf->fp, "vBat: %5.2f ", vBat);

	    clear_pending_cmd(GET_VBAT);
	}
	break;

    case GET_VERSION:
	checkPacketSize("GET_VERSION", 2);
	set_pending_cmd(GET_VERSION);
	break;

    case (GET_VERSION | PENDING):
	checkPacketSize("GET_VERSION (resp)", 5);

	parse_stat1(miso[0]);
	fprintf(my_parser.lf->fp, "hw: %x ", miso[1]);
	fprintf(my_parser.lf->fp, "usecase: %x ", miso[2]);
	fprintf(my_parser.lf->fp, "fw: %d.%d", miso[3], miso[4]);

	clear_pending_cmd(GET_VERSION);
	break;

    case LORA_SYNCH_TIMEOUT:
	checkPacketSize("LORA_SYNCH_TIMEOUT", 3);
	fprintf(my_parser.lf->fp, "symbol: %02x ( should be zero )", mosi[2]);
	break;

    case READ_DEVICE_PIN:
	/*
	 * The packet sent should be only 2 bytes, but 13 were sent by Semtech. Disable length
	 * check for now.
	 */
	checkPacketSize("READ_DEVICE_PIN (FRIG)", 0);
	set_pending_cmd(READ_DEVICE_PIN);
	break;

    case (READ_DEVICE_PIN | PENDING):
	checkPacketSize("READ_DEVICE_PIN (resp)", 0);

	parse_stat1(miso[0]);
	fprintf(my_parser.lf->fp, "pins: %08x", get_32(&mosi[1]));

	clear_pending_cmd(READ_DEVICE_PIN);
	break;

    case REBOOT:
	checkPacketSize("REBOOT", 3);
	fprintf(my_parser.lf->fp, "stay in bootloader: %d", mosi[2]);
	break;

    case SEMTECH_UNKNOWN_0229:
	checkPacketSize("SEMTECH_UNKNOWN_0229", 13);
	fprintf(my_parser.lf->fp, "Bytes: ");
	for (i=0; i<13; i++) {
	    fprintf(my_parser.lf->fp, "%02x ", mosi[i]);
	}

	break;

    case SEMTECH_UNKNOWN_022b:
	checkPacketSize("SEMTECH_UNKNOWN_022b", 3);
	fprintf(my_parser.lf->fp, "Bytes: \n");
	for (i=0; i<3; i++) {
	    fprintf(my_parser.lf->fp, "   %2d: %x \n", i, mosi[i]);
	}

	break;

    case SET_DIO_AS_RF_SWITCH:
	checkPacketSize("SET_DIO_AS_RF_SWITCH", 10);
	fprintf(my_parser.lf->fp, "Enable: %02x, ",  mosi[2]);
	fprintf(my_parser.lf->fp, "SbCfg: %02x, ",   mosi[3]);
	fprintf(my_parser.lf->fp, "RxCfg: %02x, ",   mosi[4]);
	fprintf(my_parser.lf->fp, "TxCfg: %02x, ",   mosi[5]);
	fprintf(my_parser.lf->fp, "TxHpCfg: %02x, ", mosi[6]);
	fprintf(my_parser.lf->fp, "GnssCfg: %02x, ", mosi[8]);
	fprintf(my_parser.lf->fp, "WifiCfg: %02x", mosi[9]);
	break;

    case SET_DIO_IRQ_PARAMS:
	checkPacketSize("SET_DIO_IRQ_PARAMS", 10);

	fprintf(my_parser.lf->fp, "IRQ1: %02x%02x%02x%02x, ", mosi[2], mosi[3], mosi[4], mosi[5]);
	fprintf(my_parser.lf->fp, "IRQ2: %02x%02x%02x%02x",   mosi[6], mosi[7], mosi[8], mosi[9]);
	break;

    case SET_LORA_PUBLIC_NETWORK:
	checkPacketSize("SET_LORA_PUBLIC_NETWORK", 3);
	fprintf(my_parser.lf->fp, "network: %02x", mosi[2]);
	break;

    case SET_MODULATION_PARAM:

	checkPacketSize("SET_MODULATION_PARAM", 6);

	fprintf(my_parser.lf->fp, "SF: %02x ", mosi[2]);
	fprintf(my_parser.lf->fp, "BWL: %02x ", mosi[3]);
	fprintf(my_parser.lf->fp, "CR: %02x ", mosi[4]);
	fprintf(my_parser.lf->fp, "lowDROpt: %02x", mosi[5]);
	break;

    case SET_PA_CFG:
	checkPacketSize("SET_PA_CFG", 6);

	fprintf(my_parser.lf->fp, "PaSel: %02x, ",  mosi[2]);
	fprintf(my_parser.lf->fp, "RegPaSupply: %02x, ",   mosi[3]);
	fprintf(my_parser.lf->fp, "PaDutyCycle: %02x, ",   mosi[4]);
	fprintf(my_parser.lf->fp, "PaHpSel: %02x",   mosi[5]);
	break;

    case SET_PACKET_PARAM:

	switch(packet_type) {

	default:
	    fprintf(my_parser.lf->fp, "SET_PACKET_PARAM: unknown packet type: %x.  Assume LORA\n", packet_type);

	    /* fall-through */

	case PACKET_TYPE_LORA:
	    checkPacketSize("SET_PACKET_PARAM (LORA)", 8);
	    fprintf(my_parser.lf->fp, "PbLength: %04x ",  (mosi[2] << 8) | mosi[3]);
	    fprintf(my_parser.lf->fp, "HeaderType: %02x ",  mosi[4]);

	    payload_length = mosi[5];

	    fprintf(my_parser.lf->fp, "PayloadLen: %02x ",  mosi[5]);
	    fprintf(my_parser.lf->fp, "CRC: %02x ",  mosi[6]);
	    fprintf(my_parser.lf->fp, "InvertIQ: %02x ",  mosi[7]);
	    break;
	}
	break;

    case SET_PACKET_TYPE:
	checkPacketSize("SET_PACKET_TYPE", 3);

	packet_type = mosi[2];

	fprintf(my_parser.lf->fp, "PacketType: %02x ", packet_type);
	break;

    case SET_REG_MODE:
	checkPacketSize("SET_REG_MODE", 3);

	fprintf(my_parser.lf->fp, "RegMode: %02x ",  mosi[2]);
	break;

    case SET_RF_FREQUENCY:
	checkPacketSize("SET_RF_FREQUENCY", 6);

	freq = (mosi[2] << 24) | (mosi[3] << 16) | (mosi[4] << 8) | (mosi[5] << 0);
	fprintf(my_parser.lf->fp, "Frequency: %04x (%d)",  freq, freq);
	break;

    case SET_RX:
	checkPacketSize("SET_RX", 5);

	timeout = (mosi[2] << 16) | (mosi[3] << 8) | mosi[4];

	fprintf(my_parser.lf->fp, "timeout: 0x%x (%d)", timeout, timeout);
	break;

    case SET_RX_BOOSTED:
	checkPacketSize("SET_RX_BOOSTED", 3);
	fprintf(my_parser.lf->fp, "boosted: %02x", mosi[2]);
	break;

    case SET_SLEEP:
	checkPacketSize("SET_SLEEP", 7);
	fprintf(my_parser.lf->fp, "sleepConfig: %0x ", mosi[2]);
	fprintf(my_parser.lf->fp, "time: %x", get_32(&mosi[3]) );
	break;

    case SET_STANDBY:
	checkPacketSize("SET_STANDBY", 3);

	fprintf(my_parser.lf->fp, "StbyConfig: %02x ",  mosi[2]);
	break;

    case SET_TCXO_MODE:
	checkPacketSize("SET_TCXO_MODE", 6);

	delay = get_24(&mosi[3]);

	fprintf(my_parser.lf->fp, "tune: %02x delay: %x ",  mosi[2], delay);
	break;

    case SET_TX:
	checkPacketSize("SET_TX", 5);

	timeout = (mosi[2] << 16) | (mosi[3] << 8) | mosi[4];

	fprintf(my_parser.lf->fp, "timeout: 0x%x (%d)", timeout, timeout);
	break;

    case SET_TXCW:
	checkPacketSize("SET_TXCW", 2);
	break;

    case SET_TX_PARAMS:
	checkPacketSize("SET_TX_PARAMS", 4);
	fprintf(my_parser.lf->fp, "TxPower: %02x ",  mosi[2]);
	fprintf(my_parser.lf->fp, "RampTime: %02x ",  mosi[3]);
	break;

    case STOP_TIMEOUT_ON_PREAMBLE:
	checkPacketSize("STOP_TIMEOUT_ON_PREAMBLE", 3);
	fprintf(my_parser.lf->fp, "stop: %02x", mosi[2]);
	break;

    case WRITE_BUFFER_8:
	checkPacketSize("WRITE_BUFFER_8", payload_length + 2);

	for (i=0; i< payload_length; i++) {
	    fprintf(my_parser.lf->fp, "%02x ", mosi[i + 2]);
	}

	break;

    case WRITE_REG_MEM_MASK32:
	checkPacketSize("WRITE_REG_MEM_MASK32", 14);

	fprintf(my_parser.lf->fp, "addr: %x ", get_32(&mosi[2]));
	fprintf(my_parser.lf->fp, "mask: %x ", get_32(&mosi[6]));
	fprintf(my_parser.lf->fp, "data: %x ", get_32(&mosi[10]));
	break;

    default:
	fprintf(my_parser.lf->fp, "Unhandled command: %04x length: %d \n", cmd, payload_length);
	//	exit(1);
    }

    fprintf(my_parser.lf->fp, "\n");
}

static signal_t my_signals[] =
    {
     { "nss",  &sample.nss, .deglitch_nsecs = 50 },
     { "clk",  &sample.clk, .deglitch_nsecs = 50 },
     { "mosi", &sample.mosi},
     { "miso", &sample.miso},
     { "busy", &sample.busy},
     { "irq",  &sample.irq},
     { NULL, NULL}
    };

static void post_connect (void)
{
    //    parser_redirect_output("uart", my_parser.lf);
}

static parser_t my_parser =
    {
     .name          = "lr1110",
     .signals       = my_signals,
     .process_frame = process_frame,
     .log_file      = "lr1110.log",
     .post_connect  = post_connect,

     /*
      * To be able to filter glitches on the clock, we want to get events at
      * least every 50 nSecs
      */
     .sample_time_nsecs = SAMPLE_TIME_NSECS,
    };

static void CONSTRUCTOR init (void)
{
    parser_register(&my_parser);
}
