#include <string.h>
#include "parseSaleae.h"
#include "lr1110.h"

#define MSB_FIRST 1

static void parse_packet (uint32_t count);

typedef struct {
    uint32_t nss;
    uint32_t clk;
    uint32_t mosi;
    uint32_t miso;
    uint32_t busy;
    uint32_t irq;
} sample_t;

static sample_t last_sample;
static sample_t sample;
static uint32_t accumulated_bits;
static uint32_t accumulated_mosi_byte;
static uint32_t accumulated_miso_byte;
static parser_t my_parser;

#define NUMBER_BYTES (256)

static uint32_t accumulated_count;
static uint32_t mosi[NUMBER_BYTES];
static uint32_t miso[NUMBER_BYTES];

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

    /*
     * If we've started a new packet, then clear accumulation.
     */
    if (falling_edge(nss)) {
	clear_packet();

	/*
	 * Check for device being busy
	 */
	if (sample.busy) {
	    fprintf(my_parser.lf->fp, "ERROR: nss dropped while device was busy\n");
	    exit(1);
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
		fprintf(my_parser.lf->fp, ">>>>  LR1110 byte: %02x %02x \n", accumulated_mosi_byte, accumulated_miso_byte);

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
    fprintf(my_parser.lf->fp, "%s: ", str);

    if (accumulated_count != size) {
	fprintf(my_parser.lf->fp, "<Length error.  Expected: %d bytes, got: %d > ", size, accumulated_count);
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

static void set_pending_cmd(uint32_t cmd)
{
    if (pending_cmd) {
	fprintf(my_parser.lf->fp, "<Pending Command Error.  pending: %x cmd: %x > ", pending_cmd, cmd);
	exit(1);
    }

    pending_cmd = (cmd | PENDING);
}

static void parse_packet (uint32_t count)
{
    uint32_t cmd;
    uint32_t freq;
    uint32_t i;
    uint32_t irq;
    uint32_t timeout;
    static uint32_t packet_count = 0;
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

    fprintf(my_parser.lf->fp, "(%d) ", packet_count);
    packet_count++;

    switch(cmd) {

    case CLEAR_ERRORS:
	fprintf(my_parser.lf->fp, "CLEAR_ERRORS: ");
	break;

    case CLEAR_IRQ:
	fprintf(my_parser.lf->fp, "CLEAR_IRQ: ");

	irq = (mosi[2] << 24) | (mosi[3] << 16) | (mosi[4] << 8) | (mosi[5] << 0) ;

	fprintf(my_parser.lf->fp, "irq_to_clear: %08x \n", irq);

	break;

    case GET_PACKET_TYPE:
	checkPacketSize("GET_PACKET_TYPE", 2);
	set_pending_cmd(GET_PACKET_TYPE);
	break;

    case (GET_PACKET_TYPE | PENDING):
	checkPacketSize("GET_PACKET_TYPE (response)", 2);

	fprintf(my_parser.lf->fp, "packet: %x \n", miso[2]);

	clear_pending_cmd(GET_PACKET_TYPE);
	break;

    case GET_STATUS:
	checkPacketSize("GET_STATUS", 6);

	irq = (miso[2] << 24) | (miso[3] << 16) | (miso[4] << 8) | (miso[5] << 0) ;

	fprintf(my_parser.lf->fp, "stat1: %x stat2: %x irq: %08x \n", miso[0], miso[1], irq);

	break;
    case GET_ERRORS:
	fprintf(my_parser.lf->fp, "GET_ERRORS: \n");
	set_pending_cmd(GET_ERRORS);
	break;

    case (GET_ERRORS | PENDING):
	fprintf(my_parser.lf->fp, "GET_ERRORS (response): %x %x %x \n",  miso[0], miso[1], miso[2]);

	clear_pending_cmd(GET_ERRORS);
	break;

    case GET_VERSION:
	fprintf(my_parser.lf->fp, "GET_VERSION: \n");
	set_pending_cmd(GET_VERSION);
	break;

    case (GET_VERSION | PENDING):
	fprintf(my_parser.lf->fp, "GET_VERSION (response): \n");
	fprintf(my_parser.lf->fp, "fw: %4x\n", (miso[2] << 8) | miso[3]);

	clear_pending_cmd(GET_VERSION);
	break;

    case LORA_SYNCH_TIMEOUT:
	fprintf(my_parser.lf->fp, "LORA_SYNCH_TIMEOUT: ");
	fprintf(my_parser.lf->fp, "symbol: %02x ( should be zero )", mosi[2]);
	break;

    case SET_DIO_AS_RF_SWITCH:
	fprintf(my_parser.lf->fp, "SET_DIO_AS_RF_SWITCH: ");
	fprintf(my_parser.lf->fp, "Enable: %02x, ",  mosi[2]);
	fprintf(my_parser.lf->fp, "SbCfg: %02x, ",   mosi[3]);
	fprintf(my_parser.lf->fp, "RxCfg: %02x, ",   mosi[4]);
	fprintf(my_parser.lf->fp, "TxCfg: %02x, ",   mosi[5]);
	fprintf(my_parser.lf->fp, "TxHpCfg: %02x, ", mosi[6]);
	fprintf(my_parser.lf->fp, "GnssCfg: %02x, ", mosi[8]);
	fprintf(my_parser.lf->fp, "WifiCfg: %02x, ", mosi[9]);
	break;

    case SET_DIO_IRQ_PARAMS:
	fprintf(my_parser.lf->fp, "SET_DIO_IRQ_PARAMS: ");
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
	fprintf(my_parser.lf->fp, "SET_PA_CFG: ");
	fprintf(my_parser.lf->fp, "PaSel: %02x, ",  mosi[2]);
	fprintf(my_parser.lf->fp, "RegPaSupply: %02x, ",   mosi[3]);
	fprintf(my_parser.lf->fp, "PaDutyCycle: %02x, ",   mosi[4]);
	fprintf(my_parser.lf->fp, "PaHpSel: %02x",   mosi[5]);
	break;

    case SET_PACKET_PARAM:

	switch(packet_type) {

	case PACKET_TYPE_LORA:
	    checkPacketSize("SET_PACKET_PARAM (LORA)", 8);
	    fprintf(my_parser.lf->fp, "PbLength: %04x ",  (mosi[2] << 8) | mosi[3]);
	    fprintf(my_parser.lf->fp, "HeaderType: %02x ",  mosi[4]);

	    payload_length = mosi[5];

	    fprintf(my_parser.lf->fp, "PayloadLen: %02x ",  mosi[5]);
	    fprintf(my_parser.lf->fp, "CRC: %02x ",  mosi[6]);
	    fprintf(my_parser.lf->fp, "InvertIQ: %02x ",  mosi[7]);
	    break;

	default:
	    fprintf(my_parser.lf->fp, "SET_PACKET_PARAM: unknown packet type: %x ", packet_type);
	    exit(1);
	}
	break;

    case SET_PACKET_TYPE:
	fprintf(my_parser.lf->fp, "SET_PACKET_TYPE: ");

	packet_type = mosi[2];

	fprintf(my_parser.lf->fp, "PacketType: %02x ", packet_type);
	break;

    case SET_REG_MODE:
	fprintf(my_parser.lf->fp, "SET_REG_MODE: ");
	fprintf(my_parser.lf->fp, "RegMode: %02x ",  mosi[2]);
	break;

    case SET_RF_FREQUENCY:
	fprintf(my_parser.lf->fp, "SET_RF_FREQUENCY: ");

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

    case SET_STANDBY:
	fprintf(my_parser.lf->fp, "SET_STANDBY: ");
	fprintf(my_parser.lf->fp, "StbyConfig: %02x ",  mosi[2]);
	break;

    case SET_TX:
	checkPacketSize("SET_TX", 5);

	timeout = (mosi[2] << 16) | (mosi[3] << 8) | mosi[4];

	fprintf(my_parser.lf->fp, "timeout: 0x%x (%d)", timeout, timeout);
	break;

    case SET_TX_PARAMS:
	fprintf(my_parser.lf->fp, "SET_TX_PARAMS: ");
	fprintf(my_parser.lf->fp, "TxPower: %02x ",  mosi[2]);
	fprintf(my_parser.lf->fp, "RampTime: %02x ",  mosi[3]);
	break;

    case STOP_TIMEOUT_ON_PREAMBLE:
	fprintf(my_parser.lf->fp, "STOP_TIMEOUT_ON_PREAMBLE: ");
	fprintf(my_parser.lf->fp, "stop: %02x", mosi[2]);
	break;

    case WRITE_BUFFER_8:
	checkPacketSize("WRITE_BUFFER_8", payload_length + 2);

	for (i=0; i< payload_length; i++) {
	    fprintf(my_parser.lf->fp, "%02x ", mosi[i + 2]);
	}

	break;

    default:
	fprintf(my_parser.lf->fp, "Unhandled command: %04x \n", cmd);
	exit(1);
    }

    fprintf(my_parser.lf->fp, "\n\n");
}

static signal_t my_signals[] =
{
   { "nss",  &sample.nss },
   { "clk",  &sample.clk },
   { "mosi", &sample.mosi},
   { "miso", &sample.miso},
   { "busy", &sample.busy},
   { "irq",  &sample.irq},
   { NULL, NULL}
};

static parser_t my_parser =
    {
     .name          = "lr1110",
     .signals       = my_signals,
     .process_frame = process_frame,
     .log_file      = "lr1110.log",
    };

static void CONSTRUCTOR init (void)
{
    parser_register(&my_parser);
}
