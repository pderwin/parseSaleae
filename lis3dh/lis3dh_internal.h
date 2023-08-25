#pragma once

#include "lis3dh.h"

enum {
      WHO_AM_I      = 0xf,
      CTRL_REG1     = 0x20,
      CTRL_REG2     = 0x21,
      CTRL_REG3     = 0x22,
      CTRL_REG4     = 0x23,
      CTRL_REG5     = 0x24,
      CTRL_REG6     = 0x25,
      REFERENCE     = 0x26,
      STATUS_REG    = 0x27,
      OUT_X_L       = 0x28,
      OUT_X_H       = 0x29,
      OUT_Y_L       = 0x2a,
      OUT_Y_H       = 0x2b,
      OUT_Z_L       = 0x2c,
      OUT_Z_H       = 0x2d,

      INT1_CFG      = 0x30,
      INT1_SRC      = 0x31,
      INT1_THS      = 0x32,
      INT1_DURATION = 0x33,
      INT2_CFG      = 0x34,
      INT2_SRC      = 0x35,
      INT2_THS      = 0x36,
      INT2_DURATION = 0x37,
      CLICK_CFG     = 0x38,

};

#define NUMBER_REGS 0x40

static char *reg_strs[] = {
			   "0", //  0
			   "1", //  1
			   "2", //  1
			   "3", //  1
			   "4", //  1
			   "5", //  1
			   "6", //  1
			   "7", //  1
			   "8", //  1
			   "9", //  1
			   "10", //  1
			   "11", //  1
			   "12", //  1
			   "13", //  1
			   "14", //  1
			   "WHO_AM_I", //  0x0f
			   "16", //  1
			   "17", //  1
			   "18", //  1
			   "19", //  1
			   "20", //  1
			   "21", //  1
			   "22", //  1
			   "23", //  1
			   "24", //  1
			   "25", //  1
			   "26", //  1
			   "27", //  1
			   "28", //  1
			   "29", //  1
			   "30", //  1
			   "31", //  1
			   "CTRL_REG1",     // 0x20
			   "CTRL_REG2",     // 0x21
			   "CTRL_REG3",     // 0x22
			   "CTRL_REG4",     // 0x23
			   "CTRL_REG5",     // 0x24
			   "CTRL_REG6",     // 0x25
			   "REFERENCE",     // 0x26
			   "STATUS",        // 0x27
			   "OUT_X_L",       // 0x28
			   "OUT_X_H",       // 0x29
			   "OUT_Y_L",       // 0x2a
			   "OUT_Y_H",       // 0x2b
			   "OUT_Z_L",       // 0x2c
			   "OUT_Z_H",       // 0x2d
			   "FIFO_CTRL_REG", // 0x2e
			   "FIFO_SRC_REG",  // 0x2f
			   "INT1_CFG",      // 0x30
			   "INT1_SRC",      // 0x31
			   "INT1_THS",      // 0x32
			   "INT1_DURATION", // 0x33
			   "INT2_CFG",      // 0x34
			   "INT2_SRC",      // 0x35
			   "INT2_THS",      // 0x36
			   "INT2_DURATION", // 0x37
			   "CLICK_CFG",     // 0x38
			   "57", //  1
			   "58", //  1
			   "59", //  1
			   "60", //  1
			   "61", //  1
			   "62", //  1
			   "63", //  1
};

typedef struct lis3dh_data_s {

    uint32_t
       reg;

    signal_t *irq;

    time_nsecs_t packet_start_time;

    time_nsecs_t irq_rise_time;

    /*
     * When did the TX_DONE irq get posted?
     */
    time_nsecs_t tx_done_irq_rise_time;

    uint32_t accumulated_bits;
    uint32_t accumulated_miso_byte;
    uint32_t accumulated_mosi_byte;

    uint32_t count;

#define NUMBER_BYTES (1024)

    uint8_t  mosi[NUMBER_BYTES];
    uint8_t  miso[NUMBER_BYTES];

    uint32_t pending_cmd;
    uint32_t pending_group;

    uint32_t last_command_was_sleep;

} lis3dh_data_t;
