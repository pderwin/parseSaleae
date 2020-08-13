#include <stdlib.h>
#include "hdr.h"
#include "panel_commands.h"
#include "panel_i2c.h"
#include "parseSaleae.h"

// #define VERBOSE 1

/*
 * This is the address shifted up by one to accomodate the r/w bit.
 */
#define ST25DV_USER_I2C_ADDRESS (0xa6)
#define ST25DV_SYS_I2C_ADDRESS  (0xae)

static log_file_t *lf = NULL;

static uint8_t  data_buffer[256];
static uint32_t data_count;

static void process_packet(void);
static time_nsecs_t packet_start_time_nsecs;

void st25dv_connect(void)
{
    lf = log_file_create("st25dv.log");
}

/*-------------------------------------------------------------------------
 *
 * name:        st25dv_i2c
 *
 * description: just accumulate bytes into our data array.  The bytes will
 *              get processed when we get a repeated start or stop.
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
void st25dv_i2c(uint32_t byte, time_nsecs_t time_nsecs)
{
    uint32_t i;

#if (VERBOSE > 1)
    fprintf(lf->fp, "ST25DV_I2C: byte: %02x data_count: %d \n", byte, data_count);
#endif

    if (data_count == 0) {
	packet_start_time_nsecs = time_nsecs;
    }

    if (data_count >= ARRAY_SIZE(data_buffer)) {

	fprintf(lf->fp, "%s: data_buffer array overrun\n", __func__);

	for (i=0; i < ARRAY_SIZE(data_buffer); i++) {
	    fprintf(lf->fp, "   %2d %x\n", i, data_buffer[i]);
	}

	fprintf(lf->fp, "%s: data_buffer array overrun\n", __func__);
	fprintf(stderr, "%s: data_buffer array overrun\n", __func__);
	data_count = 0;
    }

    data_buffer[data_count] = byte;
    data_count++;
}

void st25dv_start (void)
{
    process_packet();
}

void st25dv_stop (void)
{
    process_packet();
}

#if 0
static char *offset_str(uint16_t offset)
{
    static char *dynamic_strs[] =
	{
	 "GPO_Ctrl_Dyn",
	 "ST Reserved",
	 "EH_Ctrl_Dyn",  // 2
	 "RF_MGNT_Dyn",  // 3
	 "I2C_SSO_Dyn",  // 4
	 "IT_STS_Dyn",   // 5
	 "MB_CTRL_Dyn"   // 6
	};

    uint16_t idx;

    if (offset >= 0x2000) {
	idx = offset - 0x2000;
	return dynamic_strs[idx];
    }

    return "";
}
#endif

static void dump_hex(uint8_t *packet, uint16_t packet_length)
{
    uint16_t i;

    fprintf(lf->fp, ">>>  ");

    for (i=0; i<packet_length; i++) {
	fprintf(lf->fp, "%02x ", packet[i]);
	}

    fprintf(lf->fp, "<<< ");
}

static void process_dynamic(uint16_t offset, uint8_t *packet, uint16_t packet_length)
{
    uint8_t i2c_sso;
    uint8_t mb_ctrl;

    if (packet_length == 0) {
	fprintf(lf->fp, "%s: invalid packet length", __func__);
	return;
    }



    switch (offset) {

    case 0x2004: // I2C_SSO
	i2c_sso = packet[0];
	fprintf(lf->fp, "I2C Security Session: %s", (i2c_sso & 1) ? "Open":"Closed" );
	break;

    case 0x2006: // MB_CTRL

	//	dump_hex(packet, packet_length);

	mb_ctrl = packet[0];

	fprintf(lf->fp, "MB_EN: %s", mb_ctrl & 1 ? "Enable":"Disable");
	break;
    }
}

static void process_lock_ccfile(uint8_t lock_ccfile)
{
    uint8_t bits;
    uint8_t i;
    static char *locked_strs[] =
	{
	 "Not ",
	 ""
	};

    for (i=0; i<2; i++) {

	bits = (lock_ccfile >> i) & 1;

	fprintf(lf->fp, "LCKBCK%d: %sLocked, ", i, locked_strs[bits]);
    }
}

static void process_rf_mngt(uint8_t rf_mngt)
{
    fprintf(lf->fp, "RF_DISABLE: %s ", (rf_mngt & (1 << 0)) ? "disable":"enable" );
    fprintf(lf->fp, "RF_SLEEP:   %s",   (rf_mngt & (1 << 1)) ? "disable":"enable" );
}

static void process_i2css(uint8_t i2css)
{
    uint8_t bank;
    uint8_t bits;

    static char *prot_strs[] =
	{
	 "R/W",
	 "R, W if SS open",
	 "R/W",
	 "R, W if SS open"
	};

    fprintf(lf->fp, "%x ", i2css);

    for (bank = 0 ; bank < 4; bank++) {
	bits = (i2css >> (2 * bank)) & 0x3;

	fprintf(lf->fp, "RW_PROT_%d: %s, ", bank + 1, prot_strs[bits]);
    }

}


static void process_rfaxss(uint8_t which, uint8_t rfass)
{
    static char *pwd_ctrl_strs[] =
	{
	 "no pwd",
	 "PWD_1",
	 "PWD_2",
	 "PWD_3"
	};
    static char *rw_protection_strs[] =
	{
	 "R/W",
	 "R,W if sec session open",
	 "R,W if sec session open",
	 "RO"
	};

    fprintf(lf->fp, "PWD: %s, ", pwd_ctrl_strs      [(rfass >> 0) & 3]);
    fprintf(lf->fp, "RW: %s",  rw_protection_strs [(rfass >> 2) & 3]);
}

static void process_sys(uint16_t offset, uint8_t *payload, uint16_t payload_length)
{
    uint16_t memsize;
    static uint8_t memsize_lsb;
    static uint8_t memsize_msb;

    static char *reg_strs[] =
	{
	 "GPO1",        // 0
	 "GPO2",        // 1
	 "EH_MODE",     // 2
	 "RF_MNGT",     // 3
	 "RFA1SS",      // 4
	 "ENDA1",       // 5
	 "RFA2SS",      // 6
	 "ENDA2",       // 7
	 "RFA3SS",      // 8
	 "ENDA3",       // 9
	 "RFA4SS",      // a
	 "I2CSS",       // b
	 "LOCK_CCFILE", // c
	 "FTM",         // d
	 "I2C_CFG",     // e
	 "LOCK_CFG",    // f
	 "LOCK_DSFID",  // 10
	 "LOCK_AFI",    // 11
	 "DSFID",       // 12
	 "AFI",         // 13
	 "MEM_SIZE_LSB",// 14
	 "MEM_SIZE_MSB",// 15
	 "BLK_SIZE",    // 16
	 "IC_REF",      // 17
	};


    if (payload_length == 0) {
	return;
    }

    if (offset >= ARRAY_SIZE(reg_strs)) {
	fprintf(lf->fp, "%s: invalid offset: %x ", __func__, offset);
	return;
    }

    fprintf(lf->fp, "%s: ", reg_strs[offset]);

    switch (offset) {

    case 0x3: //
	process_rf_mngt(payload[0]);
	break;

    case 0x4: // RFA1SS
    case 0x6: // RFA2SS
    case 0x8: // RFA3SS
    case 0xa: // RFA4SS
	process_rfaxss(((offset - 4) / 2) + 1, payload[0]);
	break;

    case 0xb: // I2CSS
	process_i2css(payload[0]);
	break;

    case 0xc: // LOCK_CCFILE
	process_lock_ccfile(payload[0]);
	break;

    case 0x14: // Memsize LSB

	memsize_lsb = payload[0];
	fprintf(lf->fp, "MEMSIZE_LSB: %x", memsize_lsb);
	break;

   case 0x15: // Memsize MSB

	memsize_msb = payload[0];
	memsize = (memsize_msb << 8) | memsize_lsb;

	fprintf(lf->fp, "MEMSIZE_MSB: %x (0x%x %d) ", memsize_msb, memsize, memsize);
	break;

   case 0x16: // block size
	fprintf(lf->fp, "%d", payload[0]);
	break;

   case 0x17: // IC_REG
	fprintf(lf->fp, "0x%x", payload[0]);
	break;

    default:
	fprintf(lf->fp, "Unparsed SYS register at offset: %x ", offset);
	break;
    }
}

static void process_packet(void)
{
    char     hdr_str[64];
    char     *rd_wr_str;
    char     *user_sys_str;
    uint8_t  addr;
    uint8_t  is_read;
    uint8_t  is_sys;
    uint8_t  *payload;
    uint16_t i;
    uint16_t payload_count;

    static uint16_t offset;  // save between write and read packets.

    /*
     * If nothing queued up, then bail
     */
    if (data_count == 0) {
	return;
    }

#if (VERBOSE > 0)
    {
	uint32_t i;

	fprintf(lf->fp, ">>> ");

	for (i=0; i < data_count; i++) {
	    fprintf(lf->fp, "%02x ", data_buffer[i]);
	}

	fprintf(lf->fp, "\n");
    }

#endif

    addr = data_buffer[0];

    switch(addr & 0xfe) {

    case ST25DV_USER_I2C_ADDRESS:
	user_sys_str = "User";
	is_sys = 0;
	break;
    case ST25DV_SYS_I2C_ADDRESS:
	user_sys_str = "Sys";
	is_sys = 1;
	break;
    default:
	data_count = 0;
	return;
    }

    if (addr & 1) {
	rd_wr_str = "RD";
	is_read = 1;
    }
    else {
	rd_wr_str = "WR";
	is_read = 0;
    }

    (void) is_read;

    sprintf(hdr_str, "%s %s", rd_wr_str, user_sys_str);

    if (is_read) {
	/*
	 *
	 */
	hdr(lf, packet_start_time_nsecs, hdr_str);

	payload        = &data_buffer[1];
	payload_count  = data_count - 1;

	fprintf(lf->fp, "offset: %x", offset);

	if (payload_count) {

	    /*
	     * If we're reading a dynamic register, then parse that.
	     */
	    if (offset >= 0x2000 && offset <= 0x2008 ) {
		process_dynamic(offset, payload, payload_count);
	    }
	    else if (is_sys) {
		process_sys(offset, payload, payload_count);
	    }
	    else {
		dump_hex(payload, payload_count);
	    }
	}

    }

    else {

	/*
	 * write side.  The address will always be the next two bytes.
	 */
	if (data_count < 3) {
	    //	    fprintf(lf->fp, "Error: not enough data to parse write operation");
	    data_count = 0;
	    return;
	}

	offset = (data_buffer[1] << 8) | data_buffer[2];

	{
	    static uint16_t last_offset = -1;

	    if (offset != last_offset) {
		fprintf(lf->fp, "\n");
		last_offset = offset;
	    }
	}

	/*
	 *
	 */
	hdr(lf, packet_start_time_nsecs, hdr_str);

	payload        = &data_buffer[3];
	payload_count  = data_count - 3;

	fprintf(lf->fp, "offset: %x ", offset);

	if (payload_count) {

	    fprintf(lf->fp, "l: %d ", payload_count);

	    for (i=0; i<payload_count; i++) {
		fprintf(lf->fp, "%02x ", payload[i]);
	    }
	}
    }

    /*
     * ready for next packet.
     */
    data_count = 0;
}
