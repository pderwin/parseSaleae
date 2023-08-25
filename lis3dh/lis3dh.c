#include <string.h>
#include "frame.h"
#include "hdr.h"
#include "i2c_parser.h"
#include "parseSaleae.h"
#include "parser.h"
#include "lis3dh.h"

/*
 * packet sample frequency to support filtering of i2c_clk signal.
 */
#define SAMPLE_TIME_NSECS (10)

static lis3dh_data_t
   lis3dh_data;

static signal_t
    scl = { "lis3dh_scl", .deglitch_nsecs = 50 },
    sda = { "lis3dh_sda", .deglitch_nsecs = 50 },
    irq = { "lis3dh_irq" };

static void reg_shadow_print(FILE *fp);
static void reg_shadow_set(uint32_t reg, uint32_t val);

/*-------------------------------------------------------------------------
 *
 * name:        parse_bits
 *
 * description: print names of bit fields which are enabled.
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
static void parse_bits (parser_t *parser, uint32_t val, char **strs)
{
    uint32_t
	i;
    DECLARE_LOG_FP;

    for ( i= 0; i< 8; i++) {

	if (val & 0x80) {
	    fprintf(log_fp, "%s ", strs[i]);
	}

	val = (val << 1);
    }
}


static void parse_ctrl1(parser_t *parser, uint32_t val)
{
    static char *odr_str[] = {
			      "power-down mode",     // 0
			      "1 Hz",                // 1
			      "10 Hz",               // 2
			      "25 Hz",               // 3
			      "50 Hz",               // 4
			      "100 Hz",              // 5
			      "200 Hz",              // 6
			      "400 Hz",              // 7
			      "(low power) 1.6 kHz", // 8
			      "(normal) 1.344 kHz",  // 9
			      "???",                 // 0xa
			      "",                    // 0xb
			      "",                    // 0xc
			      "",                    // 0xd
			      "",                    // 0xe
			      "",                    // 0xf


    };

    uint32_t
	odr;

    DECLARE_LOG_FP;

    odr = (val >> 4) & 0xf;

    fprintf(log_fp, "ODR: %x (%s) ",  odr, odr_str[odr]);
    fprintf(log_fp, "LPen: %x ", (val >> 3) &  1);
    fprintf(log_fp, "Zen: %x ",  (val >> 2) &  1);
    fprintf(log_fp, "Yen: %x ",  (val >> 1) &  1);
    fprintf(log_fp, "Xen: %x ",  (val >> 0) &  1);
    fprintf(log_fp, "Xen: %x",   (val >> 0) &  1);
}


static void parse_ctrl2(parser_t *parser, uint32_t val)
{
    uint32_t
	fds,
	hpcf,
	hpclick,
	hpm;

    DECLARE_LOG_FP;

    hpm = (val >> 6) & 0x3;
    hpcf = (val >> 4) & 0x1;
    fds = (val >> 3) & 0x1;
    hpclick = (val >> 2) & 0x1;

    fprintf(log_fp, "hpm: %x ", hpm);
    fprintf(log_fp, "hpcf: %x ", hpcf);
    fprintf(log_fp, "fds: %x ", fds);
    fprintf(log_fp, "hpclick: %x ", hpclick);
    if ( val & (1 << 1)) { fprintf(log_fp, "HP_IA2 "); }
    if ( val & (1 << 0)) { fprintf(log_fp, "HP_IA1 "); }
}

static void parse_ctrl3(parser_t *parser, uint32_t val)
{
    DECLARE_LOG_FP;

    if ( val & (1 << 7)) { fprintf(log_fp, "I1_CLICK "); }
    if ( val & (1 << 6)) { fprintf(log_fp, "I1_IA1 "); }
    if ( val & (1 << 5)) { fprintf(log_fp, "I1_IA2 "); }
    if ( val & (1 << 4)) { fprintf(log_fp, "I1_ZYXDA "); }
    if ( val & (1 << 3)) { fprintf(log_fp, "I1_321AD "); }
    if ( val & (1 << 2)) { fprintf(log_fp, "I1_WTM "); }
    if ( val & (1 << 1)) { fprintf(log_fp, "I1_OVERRUN "); }
}

static void parse_ctrl4(parser_t *parser, uint32_t val)
{
    static char *full_scale_strs[] = {
				      "2g",
				      "4g",
				      "8g",
				      "16g"
    };

    uint32_t
	full_scale_sel,
	self_test;

    DECLARE_LOG_FP;

    if (val & (1 << 7)) { fprintf(log_fp, "BDU"); }
    if (val & (1 << 6)) { fprintf(log_fp, "BLE"); }

    full_scale_sel = (val >> 4) & 3;

    fprintf(log_fp, "full_scale: %d (%s) ", full_scale_sel, full_scale_strs[full_scale_sel]);

    if (val & (1 << 3)) { fprintf(log_fp, "HR"); }

    self_test = (val >> 1) & 3;

    if (self_test) {
	fprintf(log_fp, "Selft-test: %d ", self_test);
    }

    if (val & (1 << 0)) { fprintf(log_fp, "SIM"); }

}

static void parse_ctrl5(parser_t *parser, uint32_t val)
{
    DECLARE_LOG_FP;

    if (val & (1 << 7)) { fprintf(log_fp, "BOOT "); }
    if (val & (1 << 6)) { fprintf(log_fp, "FIFO_EN "); }
    if (val & (1 << 3)) { fprintf(log_fp, "LIR_INT1 "); }
    if (val & (1 << 2)) { fprintf(log_fp, "D4D_INT1 "); }
    if (val & (1 << 1)) { fprintf(log_fp, "LIR_INT2 "); }
    if (val & (1 << 0)) { fprintf(log_fp, "D4D_INT2 "); }

}

static void parse_ctrl6(parser_t *parser, uint32_t val)
{
    DECLARE_LOG_FP;

    if (val & (1 << 7)) { fprintf(log_fp, "I2_CLICK "); }
    if (val & (1 << 6)) { fprintf(log_fp, "I2_IA1 "); }
    if (val & (1 << 5)) { fprintf(log_fp, "I2_IA2 "); }
    if (val & (1 << 4)) { fprintf(log_fp, "I2_BOOT "); }
    if (val & (1 << 3)) { fprintf(log_fp, "I2_ACT "); }
    if (val & (1 << 1)) { fprintf(log_fp, "INT_POLARITY "); }  // 0 = active high, 1 = active low
}

static void parse_reference(parser_t *parser, uint32_t val)
{
    DECLARE_LOG_FP;


    (void) log_fp;
    (void) val;
}

static void parse_int1_cfg(parser_t *parser, uint32_t val)
{
    static char *bits[] = {
			   "AIO",
			   "6D",
			   "ZHIE",
			   "ZLIE",
			   "YHIE",
			   "YLIE",
			   "XHIE",
			   "XLIE"
    };

    parse_bits(parser, val, bits);
}

static void parse_int1_src(parser_t *parser, uint32_t val)
{
    static char *bits[] = {
			   "",
			   "IA",
			   "ZH",
			   "ZL",
			   "YH",
			   "YL",
			   "XH",
			   "XL"
    };

    parse_bits(parser, val, bits);
}


static void parse_int2_cfg(parser_t *parser, uint32_t val)
{
    static char *int2_bits[] = {
				"AIO",
				"6D",
				"ZHIE",
				"ZLIE",
				"YHIE",
				"YLIE",
				"XHIE",
				"XLIE"
    };

    parse_bits(parser, val, int2_bits);
}

static void parse_int2_src(parser_t *parser, uint32_t val)
{
    static char *bits[] = {
			 "",
			 "IA",
			 "ZH",
			 "ZL",
			 "YH",
			 "YL",
			 "ZH",
			 "ZL"
    };

    parse_bits(parser, val, bits);
}

static void parse_int1_duration(parser_t *parser, uint32_t val)
{
    DECLARE_LOG_FP;

    (void) log_fp;
    (void) val;
}

static void parse_int2_duration(parser_t *parser, uint32_t val)
{
    DECLARE_LOG_FP;

    (void) log_fp;
    (void) val;
}

static void parse_int1_ths(parser_t *parser, uint32_t val)
{
    DECLARE_LOG_FP;

    (void) log_fp;
    (void) val;
}

static void parse_int2_ths(parser_t *parser, uint32_t val)
{
    DECLARE_LOG_FP;

    (void) log_fp;
    (void) val;
}

static void parse_click_cfg(parser_t *parser, uint32_t val)
{
    DECLARE_LOG_FP;

    (void) log_fp;
    (void) val;
}



static void parse_out_x_h(parser_t *parser, uint32_t val)
{
    DECLARE_LOG_FP;

    fprintf(log_fp, "OUT_X_H: %x -- ", val);
}

static void parse_out_x_l(parser_t *parser, uint32_t val)
{
    DECLARE_LOG_FP;

    fprintf(log_fp, "OUT_X_L: %x -- ", val);
}

static void parse_out_y_h(parser_t *parser, uint32_t val)
{
    DECLARE_LOG_FP;

    fprintf(log_fp, "OUT_Y_H: %x -- ", val);
}

static void parse_out_y_l(parser_t *parser, uint32_t val)
{
    DECLARE_LOG_FP;

    fprintf(log_fp, "OUT_Y_L: %x -- ", val);
}


static void parse_out_z_h(parser_t *parser, uint32_t val)
{
    DECLARE_LOG_FP;

    fprintf(log_fp, "OUT_Z_H: %x -- ", val);
}

static void parse_out_z_l(parser_t *parser, uint32_t val)
{
    DECLARE_LOG_FP;

    fprintf(log_fp, "OUT_Z_L: %x -- ", val);
}

static void parse_status_reg(parser_t *parser, uint32_t val)
{
    DECLARE_LOG_FP;

    fprintf(log_fp, "ZYXOR: %x ", (val >> 7) & 1);
    fprintf(log_fp, "ZOR: %x ",   (val >> 6) & 1);
    fprintf(log_fp, "YOR: %x ",   (val >> 5) & 1);
    fprintf(log_fp, "XOR: %x ",   (val >> 4) & 1);

    fprintf(log_fp, "ZYXDA: %x ", (val >> 3) & 1);
    fprintf(log_fp, "ZDA: %x ",   (val >> 2) & 1);
    fprintf(log_fp, "YDA: %x ",   (val >> 1) & 1);
    fprintf(log_fp, "XDA: %x ",   (val >> 0) & 1);
}


static void lis3dh_rd(parser_t *parser, time_nsecs_t time_nsecs, uint8_t *packet, uint32_t packet_count)
{
    char
	buf[32];
    uint32_t
	reg,
	val;
    lis3dh_data_t
	*data = parser->data;
    DECLARE_LOG_FP;

    reg = (data->reg & 0x7f);

    while(packet_count) {

	sprintf(buf, "[RD] %02x (%s)", reg, reg_strs[reg]);
	hdr(parser, time_nsecs, buf);

	val = *packet;

	fprintf(log_fp, "%02x | ", val);

	switch(reg) {

	case CTRL_REG1:
	    parse_ctrl1(parser, val);
	    break;

	case CTRL_REG2:
	    parse_ctrl2(parser, val);
	    break;
	case CTRL_REG3:
	    parse_ctrl3(parser, val);
	    break;
	case CTRL_REG4:
	    parse_ctrl4(parser, val);
	    break;
	case CTRL_REG5:
	    parse_ctrl5(parser, val);
	    break;
	case CTRL_REG6:
	    parse_ctrl6(parser, val);
	    break;
	case REFERENCE:
	    parse_reference(parser, val);
	    break;
	case INT1_CFG:
	    parse_int1_cfg(parser, val);
	    break;
	case INT1_SRC:
	    parse_int1_src(parser, val);
	    break;
	case INT2_CFG:
	    parse_int2_cfg(parser, val);
	    break;
	case INT2_SRC:
	    parse_int2_src(parser, val);
	    break;
	case INT2_DURATION:
	    parse_int2_duration(parser, val);
	    break;
	case INT2_THS:
	    parse_int2_ths(parser, val);
	    break;
	case CLICK_CFG:
	    parse_click_cfg(parser, val);
	    break;
	case STATUS_REG:
	    parse_status_reg(parser, val);
	    break;

	case OUT_X_L:
	    parse_out_x_l(parser, val);
	    break;
	case OUT_X_H:
	    parse_out_x_h(parser, val);
	    break;

	case OUT_Y_L:
	    parse_out_y_l(parser, val);
	    break;
	case OUT_Y_H:
	    parse_out_y_h(parser, val);
	    break;

	case OUT_Z_L:
	    parse_out_z_l(parser, val);
	    break;
	case OUT_Z_H:
	    parse_out_z_h(parser, val);
	    break;

	case WHO_AM_I:
	    fprintf(log_fp, "WHO_AM_I: %x", val);
	    break;


	default:
	    fprintf(stderr, "%s: Unknown register: %x\n", __func__, reg);
	    fprintf(log_fp, "%s: Unknown register: %x\n", __func__, reg);
	}

	reg++;
	packet_count--;
	packet++;

	fprintf(log_fp, "\n");
    }
}

static void lis3dh_wr(parser_t *parser, time_nsecs_t time_nsecs, uint8_t *packet, uint32_t packet_count)
{
    char
	buf[32];
    uint32_t
	reg,
	val;

    DECLARE_LOG_FP;

    lis3dh_data_t *data = parser->data;

    /*
     * The register number is the next byte
     */
    data->reg = *packet;
    packet++;
    packet_count--;

    /*
     * turn off the auto-incrment bit
     */
    reg = data->reg & 0x7f;

    /*
     * May be just writing the address with no data to setup for a read operation.
     */
    if (packet_count == 0) {
	sprintf(buf, "[WR] %02x (%s)", reg, reg_strs[reg]);
	hdr(parser, time_nsecs, buf);
	fprintf(log_fp, "\n");
	return;
    }

    while(packet_count) {

	sprintf(buf, "[WR] %02x (%s)", reg, reg_strs[reg]);
	hdr(parser, time_nsecs, buf);

	val = *packet;

	reg_shadow_set(reg, val);

	fprintf(log_fp, "%02x | ", val);

	switch(reg) {

	case CTRL_REG1:
	    parse_ctrl1(parser, val);
	    break;

	case CTRL_REG2:
	    parse_ctrl2(parser, val);
	    break;
	case CTRL_REG3:
	    parse_ctrl3(parser, val);
	    break;
	case CTRL_REG4:
	    parse_ctrl4(parser, val);
	    break;
	case CTRL_REG5:
	    parse_ctrl5(parser, val);
	    break;
	case CTRL_REG6:
	    parse_ctrl6(parser, val);
	    break;
	case REFERENCE:
	    parse_reference(parser, val);
	    break;
	case INT1_CFG:
	    parse_int1_cfg(parser, val);
	    break;
	case INT1_SRC:
	    parse_int1_src(parser, val);
	    break;
	case INT1_DURATION:
	    parse_int1_duration(parser, val);
	    break;
	case INT1_THS:
	    parse_int1_ths(parser, val);
	    break;
	case INT2_CFG:
	    parse_int2_cfg(parser, val);
	    break;
	case INT2_SRC:
	    parse_int2_src(parser, val);
	    break;
	case INT2_DURATION:
	    parse_int2_duration(parser, val);
	    break;
	case INT2_THS:
	    parse_int2_ths(parser, val);
	    break;
	case CLICK_CFG:
	    parse_click_cfg(parser, val);
	    break;
	case STATUS_REG:
	    fprintf(log_fp, "STATUS_REG\n");
	    break;
	case WHO_AM_I:
	    fprintf(log_fp, "WHO_AM_I\n");
	    break;

	default:
	    fprintf(stderr, "%s: Unknown register: %x\n", __func__, reg);
	    fprintf(log_fp, "%s: Unknown register: %x\n", __func__, reg);
	}

	reg++;
	packet++;
	packet_count--;

	fprintf(log_fp, "\n");
    }
}

typedef struct {
    uint8_t val;
    uint32_t written:1;
} reg_shadow_t;

reg_shadow_t reg_shadows[ NUMBER_REGS ];

static void reg_shadow_set(uint32_t reg, uint32_t val)
{
    reg_shadow_t *rp;

    rp = &reg_shadows[reg];

    rp->val     = val;
    rp->written = 1;
}

static void reg_shadow_print (FILE *fp)
{
    uint32_t i;
    reg_shadow_t *rp;

    fprintf(fp, "REGS: \n");

    rp = reg_shadows;

    for (i=0; i<NUMBER_REGS; i++,rp++) {
	if (rp->written) {
	    fprintf(fp, "   %-10.10s | %02x\n", reg_strs[i], rp->val);
	}
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
static void parse_packet (parser_t *parser, time_nsecs_t time_nsecs, uint8_t *packet, uint32_t packet_count)
{
    uint32_t
	address,
	rw;

#if 0
    {
	uint32_t
	    i;
	DECLARE_LOG_FP;

	fprintf(log_fp, ">>> ");

	for (i=0; i<packet_count; i++) {
	    fprintf(log_fp, "%02x ", packet[i]);
	}

	fprintf(log_fp, "\n");
    }
#endif

    /*
     * Get the address byte
     */
    address = packet[0] >> 1;
    rw      = (packet[0] & 1);

    if (address == 0x19) {
	if (rw) {
	    lis3dh_rd(parser, time_nsecs, &packet[1], packet_count - 1);
	}
	else {
	    lis3dh_wr(parser, time_nsecs, &packet[1], packet_count - 1);
	}
    }
}

/*-------------------------------------------------------------------------
 *
 * name:        process_frame
 *
 * description: only look at the INT2 input.  The SCL and SDA are processed
 *              by the I2C parser.
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
void process_frame(parser_t *parser, frame_t *frame)
{
    lis3dh_data_t
	*data = parser->data;
    DECLARE_LOG_FP;

    if (falling_edge(data->irq)) {
	hdr(parser, frame->time_nsecs, "*** IRQ FALL ***");
	fprintf(log_fp,"\n");
    }

    if (rising_edge(data->irq)) {
	hdr(parser,  frame->time_nsecs, "*** IRQ RISE ***");
	fprintf(log_fp,"\n");

	reg_shadow_print(log_fp);
    }
}

/*-------------------------------------------------------------------------
 *
 * name:        connect
 *
 * description: if we get here, then we have found the correct signals.
 *              We now create an I2C parser to handle our data.
 *
 * input:
 *
 * output:      1 = enable this parser
 *
 *-------------------------------------------------------------------------*/
static uint32_t connect (parser_t *parser)
{
    lis3dh_data_t
	*data = parser->data;

    i2c_parser_create(parser, &scl, &sda, parse_packet);

    data->irq = &irq;

    return 1;
}

static signal_t *signals[] = {
			      &scl,
			      &sda,
			      &irq,
			      NULL };

static void reg_shadow_print_null (void)
{
    reg_shadow_print(stderr);
}

static parser_t parser =
    {
     .name          = "lis3dh",
     .signals       = signals,
     //     .process_frame = process_frame,
     .log_file_name = "lis3dh.log",

     .data          = &lis3dh_data,

     .connect       = connect,
     .process_frame = process_frame,

     /*
      * To be able to filter glitches on the clock, we want to get events at
      * least every 50 nSecs
      */
     .sample_time_nsecs = SAMPLE_TIME_NSECS,
    };

static void CONSTRUCTOR init (void)
{
    parser_register(&parser);

    atexit(reg_shadow_print_null);
}
