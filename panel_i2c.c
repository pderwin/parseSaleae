#include <stdlib.h>
#include "frame.h"
#include "hdr.h"
#include "panel_commands.h"
#include "panel_i2c.h"
#include "parseSaleae.h"

// #define VERBOSE 1

/*
 * This is the address shifted up by one to accomodate the r/w bit.
 */
#define PANEL_I2C_ADDRESS (0x58)

typedef enum
    {
     STATE_IDLE,
     STATE_WAIT_FOR_FLAG_BYTE,
     STATE_WAIT_FOR_CMD,
     STATE_WAIT_FOR_DATA,
    } state_e;
static char *state_strs[] =
    {
     "IDLE",
     "WAIT_FOR_FLAG_BYTE",
     "WAIT_FOR_CMD",
     "WAIT_FOR_DATA"
    };

static uint32_t rw;
static log_file_t *lf = NULL;

static uint32_t data_buffer[128];
static uint32_t data_count;
static uint32_t data_length;
static void process_cmd(uint32_t cmd, time_nsecs_t time_nsecs, uint32_t *data, uint32_t length);

void panel_i2c_connect(void)
{
    lf = log_file_create("panel_i2c.log");
}

void panel_i2c(uint32_t byte, time_nsecs_t time_nsecs, uint32_t is_start)
{
    uint32_t i;

    static uint32_t cmd = 0;
    static uint32_t flag_byte = 0;
    static uint32_t is_read = 0;
    static state_e  state = STATE_IDLE;

    (void) state_strs;

#if (VERBOSE > 0)
    fprintf(lf->fp, "\nPANEL_I2C: byte: %x state: %s data_count: %d \n", byte, state_strs[state], data_count);
#endif

    switch(state) {
    case STATE_IDLE:
	if (is_start && ((byte & 0xfe) == PANEL_I2C_ADDRESS)) {
	    rw = (byte & 1);

	    /*
	     * If lower bit is on, then we're doing a read.  We do not need to
	     * wait for the flag byte.
	     */
	    if (rw) {
		data_count = 0;
		state = STATE_WAIT_FOR_DATA;
	    }
	    else {
		state = STATE_WAIT_FOR_FLAG_BYTE;
	    }
	}
	break;

    case STATE_WAIT_FOR_FLAG_BYTE:
	flag_byte = byte;
	state = STATE_WAIT_FOR_CMD;
	break;

    case STATE_WAIT_FOR_CMD:
	cmd = byte;

	data_length = flag_byte & 0x7f;
	is_read  = !!(flag_byte & 0x80);

	//	fprintf(panel_lf->fp, "\nCMD: %x dl: %x is_read: %d \n", cmd, data_length, is_read);
	/*
	 * When not reading, we wait for the written data.
	 */
	if (! is_read) {
	    data_count = 0;
	    state = STATE_WAIT_FOR_DATA;
	}
	else {
	    state = STATE_IDLE;
	}

	break;

    case STATE_WAIT_FOR_DATA:

	if (data_count >= ARRAY_SIZE(data_buffer)) {

	    fprintf(lf->fp, "%s: data_buffer array overrun\n", __func__);

	    for (i=0; i < ARRAY_SIZE(data_buffer); i++) {
		fprintf(lf->fp, "   %2d %x\n", i, data_buffer[i]);
	    }

	    fprintf(stderr, "%s: data_buffer array overrun\n", __func__);
	    data_count = 0;
	}

	data_buffer[data_count] = byte;
	data_count++;

	if (data_count == data_length) {
	    process_cmd(cmd, time_nsecs, data_buffer, data_count);
	    state = STATE_IDLE;
	}

	break;
    }
}

void panel_i2c_irq (frame_t *frame, int state)
{
    if (state == 0) {
	hdr(lf, frame->time_nsecs, "***PANEL IRQ ASSERT***");
    }
    else {
	hdr(lf, frame->time_nsecs, "***PANEL IRQ DE-ASSERT***");
    }
}

#if 0
static char *cap44x_key_str[][8] =
    {
     {
      "3",
      "2",
      "#",
      "START",
      "0,4",
      "0,5",
      "0,6",
      "0,7"
     },
     {
      "4",
      "*",
      "9",
      "BACK",
      "1,4",
      "1,5",
      "1,6",
      "1,7"
     },
     {
      "6",
      "1",
      "STOP",
      "PAUSE",
      "2,4",
      "2,5",
      "2,6",
      "2,7"
     },
     {
      "7",
      "5",
      "HOME",
      "VOL_UP",
      "3,4",
      "3,5",
      "3,6",
      "3,7"
     },
     {
      "8",
      "0",
      "VOL_DN",
      "CLEAR",
      "4,4",
      "4,5",
      "4,6",
      "4,7"
     },
     {
      "5,0",
      "5,1",
      "5,2",
      "5,3",
      "5,4",
      "5,5",
      "5,6",
      "5,7"
     },
     {
      "6,0",
      "6,1",
      "6,2",
      "6,3",
      "6,4",
      "6,5",
      "6,6",
      "6,7"
     },
     {
      "7,0",
      "7,1",
      "7,2",
      "7,3",
      "7,4",
      "7,5",
      "7,6",
      "7,7"
     }};
#endif

static char *ata24x_key_str[][8] =
    {
     {    // r: 0
      "3",
      "2",
      "#",
      "START",
      "OK",
      "0,5",
      "0,6",
      "0,7"
     },
     {   // r: 1
      "4",
      "*",
      "9",
      "BACK",
      "RIGHT",
      "1,5",
      "1,6",
      "1,7"
     },
     {   // r: 2
      "6",
      "1",
      "STOP",
      "PAUSE",
      "LEFT",
      "2,5",
      "2,6",
      "2,7"
     },
     {  // r: 3
      "7",
      "5",
      "HOME",
      "VOL_UP",
      "UP",
      "3,5",
      "3,6",
      "3,7"
     },
     {  // r: 4
      "8",
      "0",
      "VOL_DN",
      "BKSP",
      "DOWN",
      "4,5",
      "4,6",
      "4,7"
     },
     {  // r: 5
      "5,0",
      "5,1",
      "5,2",
      "5,3",
      "5,4",
      "5,5",
      "5,6",
      "5,7"
     },
     {  // r: 6
      "6,0",
      "6,1",
      "6,2",
      "6,3",
      "6,4",
      "6,5",
      "6,6",
      "6,7"
     },
     {   // r: 7
      "7,0",
      "7,1",
      "7,2",
      "7,3",
      "7,4",
      "7,5",
      "7,6",
      "7,7"
     }
    };



static void process_cmd(uint32_t cmd, time_nsecs_t time_nsecs, uint32_t *data, uint32_t length)
{
    char buf[128];
    uint32_t i;
    static uint32_t cmd_count = 0;

    sprintf(buf, "*** Cmd: %2x (%d) ", cmd, cmd_count);
    hdr(lf, time_nsecs, buf);

    cmd_count++;

    for (i=0; i<length; i++) {
	fprintf(lf->fp, "%02x ", data[i]);
    }


    switch(cmd) {

    case UIBC_PANEL_ID:
	fprintf(lf->fp, "PANEL_ID: 0x%x ", data[0]);
	break;
    case UIBC_KEY_FIFO:
	{
	    uint32_t
		col,
		more,
		row,
		state;

	    fprintf(lf->fp, "KEY_FIFO: 0x%02x ", data[0]);

	    more  = (data[0] >> 7) & 1;
	    state = (data[0] >> 6) & 1;
	    row   = (data[0] >> 3) & 7;
	    col   = (data[0] >> 0) & 7;

	    fprintf(lf->fp, "r: %d c: %d ch: %s %s %s",
		    row,
		    col,
		    ata24x_key_str[row][col],
		    state ? "MK":"BK",
		    more  ? "MORE":"");
	}

	break;

    case UIBC_PANEL_VERSION:
	fprintf(lf->fp, "PANEL_VERSION: 0x%x ", data[0]);
	break;

    case UIBC_FW_VERSION_MAJ:
	fprintf(lf->fp, "FW_VERSION_MAJ: 0x%x ", data[0]);
	break;
    case UIBC_FW_VERSION_MIN:
	fprintf(lf->fp, "FW_VERSION_MIN: 0x%x ", data[0]);
	break;
    case UIBC_SW_ENAB:
	fprintf(lf->fp, "SW_ENAB: 0x%x ", data[0]);
	break;
    case UIBC_SPI_CLK_CONFIG_stub:
	fprintf(lf->fp, "SPI_CLK_CONFIG_stub: %x ", data[0]);
	break;

    case UIBC_SPI_DATA_W:
	for (i=0; i<length; i++) {
	    panel_spi_data(data[i]);
	}
	break;

    case UIBC_SPI_CMD_W:
	fprintf(lf->fp, "SPI_CMD_W: %x ", data[0]);
	panel_spi_cmd(data[0]);
	break;

    case UIBC_BACKLIGHT_DUTY:
	fprintf(lf->fp, "BACKLIGHT_DUTY: %x ", data[0]);
	break;
    case UIBC_IRQ_MASK:
	fprintf(lf->fp, "IRQ_MASK: %x ", data[0]);
	break;
    case UIBC_IRQ_STATUS:
	fprintf(lf->fp, "IRQ_STATUS: %x ", data[0]);
	break;
    case UIBC_DISP_ENB:
	fprintf(lf->fp, "DISP_ENB: %x ", data[0]);
	break;
    case UIBC_DISP_ENB2:
	fprintf(lf->fp, "DISP_ENB2: %x ", data[0]);
	break;
    case UIBC_DISP_ENB3:
	fprintf(lf->fp, "DISP_ENB3: %x ", data[0]);
	break;

    case UIBC_5V_ENB:
	fprintf(lf->fp, "5V_ENB: %x ", data[0]);
	break;

    case UIBC_LED1_CTRL:
	fprintf(lf->fp, "LED1_CTRL: %x ", data[0]);
	break;
    case UIBC_LED1_DUTY:
	fprintf(lf->fp, "LED1_DUTY: %x ", data[0]);
	break;
    case UIBC_LED2_CTRL:
	fprintf(lf->fp, "LED2_CTRL: %x ", data[0]);
	break;
    case UIBC_LED2_DUTY:
	fprintf(lf->fp, "LED2_DUTY: %x ", data[0]);
	break;
    case 0x83:
	fprintf(lf->fp, "UNDOCUMENTED COMMAND 0x83: %x ", data[0]);
	break;

    case UIBC_UC_ARCH_ID:
	fprintf(lf->fp, "UC_ARCH_ID: %x ", data[0]);
	break;
    case UIBC_UC_DEVICE_ID:
	fprintf(lf->fp, "UC_DEVICE_ID: %x ", data[0]);
	break;
    case UIBC_UC_DERIVATIVE_ID:
	fprintf(lf->fp, "UC_DERIVATIVE_ID: %x ", data[0]);
	break;
    case UIBC_UC_REVISION_ID:
	fprintf(lf->fp, "UC_REVISION_ID: %x ", data[0]);
	break;
    case UIBC_UC_UNIQUE_ID:
	fprintf(lf->fp, "UC_UNIQUE_ID: \n");
	for (i=0; i<16; i+= 4) {
	    fprintf(lf->fp, "     %2x %2x %2x %2x\n", data[i+0], data[i+1],data[i+2],data[i+3]);
	}
	break;

    case UIBC_A2D_RD:
	fprintf(lf->fp, "A2D_RD: %d mV", (data[1] << 8) | data[0]);
	break;

    default:
	fprintf(lf->fp, "\nPANEL_I2C: unknown command: %x (commands.h)\n", cmd);
	fprintf(stderr,       "\nPANEL_I2C: unknown command: %x (commands.h) \n", cmd);
    }
}
