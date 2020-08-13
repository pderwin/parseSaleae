#include <stdlib.h>
#include "parseSaleae.h"
#include "panel_spi_commands.h"

typedef enum
    {
     STATE_WAIT_FOR_CMD,
     STATE_WAIT_FOR_BOOSTER,
     STATE_WAIT_FOR_EV,
    } state_e;

typedef enum
    {
     LAST_TYPE_CMD,
     LAST_TYPE_DATA
    } last_type_t;

static uint32_t    data_count;
static last_type_t last_type;
static FILE        *spi_log_fp;

static void open_spi_log(void)
{
    if (spi_log_fp) {
	return;
    }

    if ((spi_log_fp = fopen("panel_spi.log", "w")) == NULL) {
	printf("Error opening panel log file\n");
	exit(1);
    }
}

void panel_spi_cmd(uint32_t byte)
{
    static state_e  state = STATE_WAIT_FOR_CMD;

    open_spi_log();

    if (last_type == LAST_TYPE_DATA) {
	fprintf(spi_log_fp, "\n");
	last_type = LAST_TYPE_CMD;
    }

    //    fprintf(spi_log_fp, "\nPANEL: byte: %x state: %d \n", byte, state);

    fprintf(spi_log_fp, "%02x: ", byte);

    switch(state) {

    case STATE_WAIT_FOR_CMD:

	/*
	 * Check for set page address command.
	 */
	if ((byte & 0xf0) == SPI_CMD_SET_PAGE_ADDRESS) {
	    fprintf(spi_log_fp, "SET_PAGE_ADDRESS: %d \n", byte & 0xf);
	    return;
	}

	/*
	 * Check for set column address high
	 */
	if ((byte & 0xf0) == SPI_CMD_SET_COL_ADDRESS_MSB) {
	    fprintf(spi_log_fp, "SET_COL_ADDRESS_MSB: %d \n", byte & 0xf);
	    return;
	}

	/*
	 * Check for set column address low
	 */
	if ((byte & 0xf0) == SPI_CMD_SET_COL_ADDRESS_LSB) {
	    fprintf(spi_log_fp, "SET_COL_ADDRESS_LSB: %d \n", byte & 0xf);
	    return;
	}

	/*
	 * Check for write data command.
	 */
	if ((byte & 0xf8) == SPI_CMD_POWER_CONTROL) {
	    fprintf(spi_log_fp, "POWER_CONNTROL: %x \n", byte & 0x7);
	    return;
	}

	/*
	 * Check for display start line.
	 */
	if ((byte & 0xc0) == SPI_CMD_DISPLAY_START_LINE) {
	    fprintf(spi_log_fp, "DISPLAY_START_LINE: %x \n", byte & 0x3f);
	    return;
	}

	/*
	 * Check for bias select
	 */
	if ((byte & 0xfe) == SPI_CMD_BIAS_SELECT) {
	    fprintf(spi_log_fp, "BIAS_SELECT: %s \n", (byte & 1) ? "1/7":"1/9");
	    return;
	}

	/*
	 * Check for booster
	 */
	if (byte == SPI_CMD_SET_BOOSTER) {
	    state = STATE_WAIT_FOR_BOOSTER;
	    return;
	}

	/*
	 * Check for inverse display
	 */
	if ((byte & 0xfe) == SPI_CMD_INVERSE_DISPLAY) {
	    fprintf(spi_log_fp, "INVERSE DISPLAY: %s \n", (byte & 1) ? "Inverse":"Normal");
	    return;
	}

	/*
	 * Check for seg direction
	 */
	if ((byte & 0xfe) == SPI_CMD_SEG_DIRECTION) {
	    fprintf(spi_log_fp, "SEG DIRECTION: %s \n", (byte & 1) ? "Reverse":"Normal");
	    return;
	}

	/*
	 * Check for com direction
	 */
	if ((byte & 0xf0) == SPI_CMD_COM_DIRECTION) {
	    fprintf(spi_log_fp, "COM DIRECTION: %s \n", (byte & 8) ? "Reverse":"Normal");
	    return;
	}

	/*
	 * Check for regulation ratio
	 */
	if ((byte & 0xf8) == SPI_CMD_REGULATION_RATIO) {
	    fprintf(spi_log_fp, "REGULATION RATIO: %d \n", (byte & 7));
	    return;
	}

	/*
	 * Check for regulation ratio
	 */
	if ((byte & 0xff) == SPI_CMD_EV) {
	    state = STATE_WAIT_FOR_EV;
	    return;
	}

	/*
	 * Check for regulation ratio
	 */
	if ((byte & 0xfe) == SPI_CMD_DISPLAY_ON) {
	    fprintf(spi_log_fp, "DISPLAY CONTROL: %s \n", (byte & 1) ? "ON":"OFF");
	    return;
	}


	fprintf(spi_log_fp, "unknown SPI command: %x \n", byte);
	break;

    case STATE_WAIT_FOR_BOOSTER:
	fprintf(spi_log_fp, "SET BOOSTER: %s \n", (byte & 1) ? "5x":"4x");
	state = STATE_WAIT_FOR_CMD;
	break;

    case STATE_WAIT_FOR_EV:
	fprintf(spi_log_fp, "SET EV: 0x%x \n", (byte & 0x3f));
	state = STATE_WAIT_FOR_CMD;
	break;

    default:
	fprintf(spi_log_fp, "unknown SPI state: %x \n", byte);
    }

}

void panel_spi_data(uint32_t byte)
{
    open_spi_log();

    if (last_type == LAST_TYPE_CMD) {
	fprintf(spi_log_fp, "\nDATA: ");
	data_count = 0;

	last_type = LAST_TYPE_DATA;
    }

    if (data_count == 16) {

	fprintf(spi_log_fp, "\n      ");
	data_count = 0;
    }

    data_count++;

    fprintf(spi_log_fp, "%02x ", byte);
}
