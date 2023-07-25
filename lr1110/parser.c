#include <string.h>
#include "frame.h"
#include "hdr.h"
#include "parseSaleae.h"
#include "parser.h"
#include "lr1110.h"
#include "lr1110/crypto.h"
#include "lr1110/gnss.h"
#include "lr1110/radio.h"
#include "lr1110/system.h"
#include "lr1110/wifi.h"

#define HEX_DUMP_PACKETS 1

/*
 * packet sample frequency to support filtering of i2c_clk signal.
 */
#define SAMPLE_TIME_NSECS (10)

/*
 * Set the time for de-glitch
 */
#define DEGLITCH_TIME (5 * SAMPLE_TIME_NSECS)

static lr1110_data_t
    lr1110_semtech_data,
    lr1110_n1_data;

#undef  falling_edge
#define falling_edge(__elem) (data->sample.__elem == 0) && (data->last_sample.__elem == 1)
#undef  rising_edge
#define rising_edge(__elem)  (data->sample.__elem == 1) && (data->last_sample.__elem == 0)


static uint32_t packet_count = 0;

static void parse_packet (parser_t *parser);

static void clear_accumulator (lr1110_data_t *data)
{
    data->accumulated_bits = 0;

    data->accumulated_mosi_byte = 0;
    data->accumulated_miso_byte = 0;
}

static void clear_packet (lr1110_data_t *data)
{
    data->count = 0;

    clear_accumulator(data);
}

void _hex_dump(FILE *log_fp, uint8_t *cp, uint32_t count)
{
    uint32_t i;

    for (i=0; i<count; i++) {
	fprintf(log_fp, "%02x ", cp[i]);
    }
}

static void process_frame (parser_t *parser, frame_t *frame)
{
    lr1110_data_t *data;
    DECLARE_LOG_FP;

    data = parser->data;

    /*
     * If we've started a new packet, then clear accumulation.
     */
    if (falling_edge(nss)) {

	data->packet_start_time = frame->time_nsecs;

	clear_packet(data);

	/*
	 * Check for device being busy
	 *
	 * This currently fails when we are exiting sleep
	 */
	if (data->sample.busy && !data->last_command_was_sleep) {
	    hdr(parser, frame->time_nsecs, "NSS_ERROR");
	    fprintf(log_fp, "ERROR: nss dropped while device was busy\n");
	    //	    exit(1);
	}
    }

    /*
     * If done with a packet, then parse it.
     */
    if (rising_edge(nss)) {

	/*
	 * Will be turned back on if we truly parse a sleep command.
	 */
	data->last_command_was_sleep = 0;

	parse_packet(parser);
	memcpy(&data->last_sample, &data->sample, sizeof(sample_t));
	return;
    }

    if (rising_edge(irq)) {
	hdr(parser, frame->time_nsecs, "*** IRQ rise ***");
	fprintf(log_fp, "\n");

	data->irq_rise_time = frame->time_nsecs;
    }

    if (falling_edge(irq)) {
	hdr(parser, frame->time_nsecs, "*** IRQ fall ***");
	fprintf(log_fp, "\n");
    }
#if 0
    if (falling_edge(busy)) {
	hdr(parser, frame->time_nsecs, "*** BUSY fall ***");
	fprintf(log_fp, "\n");
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
		fprintf(parser->lf->fp, ">>>>  LR1110 byte: %02x %02x \n",
			data->accumulated_mosi_byte,
			data->accumulated_miso_byte);
#endif

		if (data->count == NUMBER_BYTES) {
		    fprintf(log_fp, "ERROR: miso/mosi buffer overrun\n");
		    exit(1);
		}

		data->miso[data->count] = data->accumulated_miso_byte;
		data->mosi[data->count] = data->accumulated_mosi_byte;

		data->count++;

		clear_accumulator(data);
	    }
	}
    }

    memcpy(&data->last_sample, &data->sample, sizeof(sample_t));
}

void _checkPacketSize(parser_t *parser, char *group_str, char *str, uint32_t size, uint32_t cmd)
{
    lr1110_data_t *data = parser->data;
    DECLARE_LOG_FP;

    hdr_with_lineno(parser, data->packet_start_time, group_str, str, packet_count);

    if (size) {
	if (data->count != size) {
	    fprintf(log_fp, "<Length error.  Expected: %d bytes, got: %d > ", size, data->count);
	    //	    exit(1);
	}
    }

    fprintf(log_fp, "%04x | ", cmd & 0xffff);
}

uint32_t get_command (lr1110_data_t *data)
{
    uint32_t cmd;

    if (data->pending_cmd) {
	cmd = data->pending_cmd;

	data->pending_cmd   = 0;
	data->pending_group = 0;
    }
    else{
	cmd = data->mosi[1];
    }

    return cmd;
}

uint32_t get_16(uint8_t *mosi)
{
    uint32_t val;

    val = (mosi[0] << 8) | mosi[1];

    return val;
}

uint32_t get_24(uint8_t *mosi)
{
    uint32_t val;

    val = (mosi[0] << 16) | (mosi[1] << 8) | mosi[2];

    return val;
}

uint32_t get_32(uint8_t *mosi)
{
    uint32_t val;

    val = (mosi[0] << 24) | (mosi[1] << 16) | (mosi[2] << 8) | mosi[3];

    return val;
}

void _set_pending_cmd(parser_t *parser, uint32_t group, uint32_t cmd)
{
    lr1110_data_t *data = parser->data;
    FILE *log_fp = parser->log_file->fp;

    if (data->pending_cmd) {
	fprintf(log_fp, "<Pending Command Error.  pending: %x cmd: %x > ", data->pending_cmd, cmd);
	exit(1);
    }

    data->pending_group = group;
    data->pending_cmd   = (cmd | PENDING);
}

void _parse_stat1(parser_t *parser, uint8_t stat1)
{
    FILE *log_fp = parser->log_file->fp;

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

void _parse_stat2(parser_t *parser, uint8_t stat2)
{
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
    static char *reset_status_str[] = {
				    "Cleared",            // 0
				    "power on/ brown-out",   // 1
				    "external nReset", // 2
				    "System Reset",               // 3
				    "watchdog reset",               // 4
				    "NSS toggle",               // 5
				    "RTC restart",     // 6
				    "???"               // 7
    };
    FILE *log_fp = parser->log_file->fp;

    fprintf(log_fp, "stat2: %02x ", stat2);

    reset_status = (stat2 >> 4) & 0x7;

    fprintf(log_fp, "reset_status: %x (%s) ", reset_status, reset_status_str[reset_status] );

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
    uint32_t group;
    FILE *log_fp;
    lr1110_data_t *data = parser->data;

    log_fp = parser->log_file->fp;

    if (data->count < 2) {
	return;
    }

    if (data->pending_group) {
	group = data->pending_group;
    }
    else {
	group = data->mosi[0];
    }

    if (show_time_stamps()) {
	packet_count++;
    }

#if (HEX_DUMP_PACKETS > 0)
    fprintf(log_fp, "\nMOSI: ");
    hex_dump(data->mosi, data->count);

    fprintf(log_fp, "\nMISO: ");
    hex_dump(data->miso, data->count);

    fprintf(log_fp, "\n");
#endif

    switch(group) {

    case LR1110_GROUP_ZERO:
	{
	    uint32_t cmd = 0;
#define MY_GROUP     0
#define MY_GROUP_STR "ZERO"
	    checkPacketSize("---- 00's ----", 0);
	}
	break;

    case LR1110_GROUP_CRYPTO:
	lr1110_crypto(parser);
	break;
    case LR1110_GROUP_GNSS:
	lr1110_gnss(parser);
	break;

    case LR1110_GROUP_RADIO:
	lr1110_radio(parser);
	break;

    case LR1110_GROUP_SYSTEM:
	lr1110_system(parser);
	break;

    case LR1110_GROUP_WIFI:
	lr1110_wifi(parser);
	break;

    default:
	fprintf(log_fp, "%s: unknown command group: %x \n", __func__, group);
	fprintf(stderr, "%s: unknown command group: %x \n", __func__, group);
	//	exit(1);
    }

fprintf(log_fp, "\n");
}

static signal_t signals_semtech[] =
    {
     { "nss",  &lr1110_semtech_data.sample.nss, .deglitch_nsecs = 50 },
     { "clk",  &lr1110_semtech_data.sample.clk, .deglitch_nsecs = 50 },
     { "mosi", &lr1110_semtech_data.sample.miso },
     { "miso", &lr1110_semtech_data.sample.mosi },
     { "busy", &lr1110_semtech_data.sample.busy },
     { "irq",  &lr1110_semtech_data.sample.irq, .deglitch_nsecs = 150 },
     { NULL, NULL}
    };

static signal_t signals_n1[] =
    {
     { "n1_nss",   &lr1110_n1_data.sample.nss, .deglitch_nsecs = 50 },
     { "n1_clk",   &lr1110_n1_data.sample.clk, .deglitch_nsecs = 50 },
     { "n1_mosi",  &lr1110_n1_data.sample.mosi },
     { "n1_miso",  &lr1110_n1_data.sample.miso },
     { "n1_busy",  &lr1110_n1_data.sample.busy },
     { "n1_irq",   &lr1110_n1_data.sample.irq, .deglitch_nsecs = 150 },
     { NULL, NULL}
    };


static parser_t my_parser =
    {
     .name          = "lr1110_semtech",
     .signals       = signals_semtech,
     .process_frame = process_frame,
     .log_file_name = "lr1110_semtech.log",

     .data          = &lr1110_semtech_data,

     /*
      * To be able to filter glitches on the clock, we want to get events at
      * least every 50 nSecs
      */
     .sample_time_nsecs = SAMPLE_TIME_NSECS,
    };

static void grab_uart_output (parser_t *parser)
{
    parser_redirect_output("uart", parser->log_file);
}

static parser_t n1_parser =
    {
     .name          = "lr1110_n1",
     .signals       = signals_n1,
     .process_frame = process_frame,
     .log_file_name = "lr1110_n1.log",

     .data          = &lr1110_n1_data,

     /*
      * To be able to filter glitches on the clock, we want to get events at
      * least every 50 nSecs
      */
     .sample_time_nsecs = SAMPLE_TIME_NSECS,

     .post_connect = grab_uart_output,

    };

static void CONSTRUCTOR init (void)
{
    parser_register(&my_parser);
    parser_register(&n1_parser);
}
