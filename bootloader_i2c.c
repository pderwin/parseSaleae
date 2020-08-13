#include <stdlib.h>
#include "hdr.h"
#include "parseSaleae.h"
#include "panel_commands.h"

// #define VERBOSE 1

#define SOP (0x01)
#define EOP (0x17)

/*
 * This is the address shifted up by one to accomodate the r/w bit.
 */
#define BOOTLOADER_I2C_ADDRESS (0xe0)

typedef enum
    {
     STATE_IDLE,
     STATE_SOP,
     STATE_CMD,
     STATE_PAYLOAD_SIZE_LSB,
     STATE_PAYLOAD_SIZE_MSB,
     STATE_DATA,
     STATE_CHKSUM_LSB,
     STATE_CHKSUM_MSB,
     STATE_EOP,
    } state_e;

static char *state_strs[] =
    {
     "IDLE",
     "SOP",
     "CMD",
     "PAYLOAD_LSB",
     "PAYLOAD_MSB",
     "DATA",
     "CHKSUM_LSB",
     "CHKSUM_MSB",
     "EOP",
    };

static uint32_t rw;
static log_file_t *lf = NULL;

// static uint32_t data_buffer[128];
static uint32_t data_count;
// static uint32_t data_length;
// static void process_cmd(uint32_t cmd, uint32_t *data, uint32_t length);

static void open_panel_log(void)
{
    if (lf) {
	return;
    }

    lf = log_file_create("bootloader_i2c.log");
}

void bootloader_i2c(uint32_t byte, uint32_t is_start, time_nsecs_t time_nsecs, uint32_t event_count)
{
    //    uint32_t i;

    static uint32_t cmd = 0;
    // static uint32_t flag_byte = 0;
    //    static uint32_t is_read = 0;
    static state_e  state = STATE_IDLE;
    static uint16_t payload_size = 0;
    static uint16_t chksum = 0;

    (void) state_strs;

    open_panel_log();

#if (VERBOSE > 0)
    hdr(log_fp, time_nsecs, "BOOTLOADER_I2C");
    fprintf(log_fp, "BOOTLOADER_I2C: byte: %x state: %s (ec: %d) \n", byte, state_strs[state], event_count);
#endif

    switch(state) {

    case STATE_IDLE:
	if (is_start && ((byte & 0xfe) == BOOTLOADER_I2C_ADDRESS)) {
	    rw = (byte & 1);

#if (VERBOSE > 0)
	    fprintf(log_fp, "Address %x ", byte);
#endif
	    /*
	     * Wait for SOP back
	     */
	    state = STATE_SOP;
	}
	break;

    case STATE_SOP:
	if (byte != SOP) {
	    fprintf(lf->fp, "ERROR: did not get SOP byte: %x  (ec: %d) \n", byte, event_count);
	    state = STATE_IDLE;
	}
	else {
	    state = STATE_CMD;
	}
	break;

    case STATE_CMD:
	cmd = byte;

	hdr(lf, time_nsecs, "BOOTLOADER_I2C");

	fprintf(lf->fp, "Cmd: %x | ", cmd);
	state = STATE_PAYLOAD_SIZE_LSB;
	break;

    case STATE_PAYLOAD_SIZE_LSB:
	payload_size = byte;
	state = STATE_PAYLOAD_SIZE_MSB;
	break;

    case STATE_PAYLOAD_SIZE_MSB:
	payload_size |= (byte << 8);

	data_count = 0;
#if (VERBOSE > 0)
	fprintf(log_fp, "Payload Size: %d \n", payload_size);
#endif
	state = STATE_DATA;
	break;

    case STATE_DATA:

	fprintf(lf->fp, "%02x ", byte);

	data_count++;
	if (data_count == payload_size) {
	    state = STATE_CHKSUM_LSB;
	}
	break;

    case STATE_CHKSUM_LSB:
	chksum = byte;
	state =STATE_CHKSUM_MSB;
	break;

    case STATE_CHKSUM_MSB:
	chksum |= (byte << 8);
	state =STATE_EOP;
	break;
    case STATE_EOP:
	if (byte != EOP) {
	    fprintf(lf->fp, "Invalid EOP: %x ", byte);
	}
	state = STATE_IDLE;
	break;
    default:
	printf("Unhandled case\n");
	exit(1);
    }
}

#if 0
static void process_cmd(uint32_t cmd, uint32_t *data, uint32_t length)
{
    uint32_t i;
    static uint32_t cmd_count = 0;

    fprintf(log_fp, "*** Cmd: %2x (%d) ", cmd, cmd_count);
    cmd_count++;

    for (i=0; i<length; i++) {
	fprintf(log_fp, "%02x ", data[i]);
    }


    switch(cmd) {

    case UIBC_PANEL_ID:
	fprintf(log_fp, "PANEL_ID: %x ", data[0]);
	break;
    case UIBC_KEY_FIFO:
	fprintf(log_fp, "KEY_FIFO: %x ", data[0]);
	break;

    case UIBC_PANEL_VERSION:
	fprintf(log_fp, "PANEL_VERSION: %x ", data[0]);
	break;

    case UIBC_FW_VERSION_MAJ:
	fprintf(log_fp, "FW_VERSION_MAJ: %x ", data[0]);
	break;
    case UIBC_FW_VERSION_MIN:
	fprintf(log_fp, "FW_VERSION_MIN: %x ", data[0]);
	break;
    case UIBC_SW_ENAB:
	fprintf(log_fp, "SW_ENAB: %x ", data[0]);
	break;
    case UIBC_SPI_CLK_CONFIG_stub:
	fprintf(log_fp, "SPI_CLK_CONFIG_stub: %x ", data[0]);
	break;

    case UIBC_SPI_DATA_W:
	for (i=0; i<length; i++) {
	    panel_spi_data(data[i]);
	}
	break;

    case UIBC_SPI_CMD_W:
	fprintf(log_fp, "SPI_CMD_W: %x ", data[0]);
	panel_spi_cmd(data[0]);
	break;

    case UIBC_BACKLIGHT_DUTY:
	fprintf(log_fp, "BACKLIGHT_DUTY: %x ", data[0]);
	break;
    case UIBC_IRQ_MASK:
	fprintf(log_fp, "IRQ_MASK: %x ", data[0]);
	break;
    case UIBC_IRQ_STATUS:
	fprintf(log_fp, "IRQ_STATUS: %x ", data[0]);
	break;
    case UIBC_DISP_ENB:
	fprintf(log_fp, "DISP_ENB: %x ", data[0]);
	break;
    case UIBC_DISP_ENB2:
	fprintf(log_fp, "DISP_ENB2: %x ", data[0]);
	break;
    case UIBC_DISP_ENB3:
	fprintf(log_fp, "DISP_ENB3: %x ", data[0]);
	break;

    case UIBC_5V_ENB:
	fprintf(log_fp, "5V_ENB: %x ", data[0]);
	break;

    case UIBC_LED1_CTRL:
	fprintf(log_fp, "LED1_CTRL: %x ", data[0]);
	break;
    case UIBC_LED1_DUTY:
	fprintf(log_fp, "LED1_DUTY: %x ", data[0]);
	break;
    case UIBC_LED2_CTRL:
	fprintf(log_fp, "LED2_CTRL: %x ", data[0]);
	break;
    case UIBC_LED2_DUTY:
	fprintf(log_fp, "LED2_DUTY: %x ", data[0]);
	break;
    case 0x83:
	fprintf(log_fp, "UNDOCUMENTED COMMAND 0x83: %x ", data[0]);
	break;

    case UIBC_UC_ARCH_ID:
	fprintf(log_fp, "UC_ARCH_ID: %x ", data[0]);
	break;
    case UIBC_UC_DEVICE_ID:
	fprintf(log_fp, "UC_DEVICE_ID: %x ", data[0]);
	break;
    case UIBC_UC_DERIVATIVE_ID:
	fprintf(log_fp, "UC_DERIVATIVE_ID: %x ", data[0]);
	break;
    case UIBC_UC_REVISION_ID:
	fprintf(log_fp, "UC_REVISION_ID: %x ", data[0]);
	break;
    case UIBC_UC_UNIQUE_ID:
	fprintf(log_fp, "UC_UNIQUE_ID: \n");
	for (i=0; i<16; i+= 4) {
	    fprintf(log_fp, "     %2x %2x %2x %2x\n", data[i+0], data[i+1],data[i+2],data[i+3]);
	}
	break;

    default:
	fprintf(log_fp, "\nPANEL_I2C: unknown command: %x (commands.h)\n", cmd);
	fprintf(stderr,       "\nPANEL_I2C: unknown command: %x (commands.h) \n", cmd);
    }

    fprintf(log_fp, "\n");
}

#endif
