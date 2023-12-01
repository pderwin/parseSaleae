#pragma once

#include "parser.h"

typedef enum
    {
     STATE_IDLE,
     STATE_COLLECT_ADDRESS,
     STATE_RW_BIT,
     STATE_ADDRESS_ACK,
     STATE_COLLECT_DATA,
     STATE_DATA_ACK,
     STATE_WAIT_FOR_STOP_OR_REP_START,
    } state_e;

typedef struct i2c_data_s {

    state_e
       state;

    parser_t
       *orig_parser;

    signal_t
       *scl,
       *sda;

    uint32_t
	accumulated_bits,
	accumulated_byte;

    void (*parse_packet)(parser_t *parser,
			 time_nsecs_t time_nsecs,
			 uint8_t *packet,
			 uint32_t packet_count);

    time_nsecs_t
	packet_start_time_nsecs;

#define NUMBER_PACKET_BYTES (32)

    uint8_t
	packet[NUMBER_PACKET_BYTES];
    uint32_t
	packet_count;

} i2c_data_t;
