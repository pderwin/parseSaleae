#pragma once

#include <stdio.h>
#include "parser.h"

#define PENDING (0x80000000)

enum {
      LR1110_GROUP_ZERO   = 0,
      LR1110_GROUP_SYSTEM = 1,
      LR1110_GROUP_RADIO  = 2,
      LR1110_GROUP_WIFI   = 3,
      LR1110_GROUP_GNSS   = 4,
      LR1110_GROUP_CRYPTO = 5,
};

typedef enum {
	      PACKET_TYPE_NONE    = 0,
	      PACKET_TYPE_GFSK    = 1,
	      PACKET_TYPE_LORA    = 2,
} packet_type_e;

typedef enum {
	      IRQ_TX_DONE           = (1 <<  2),
	      IRQ_RX_DONE           = (1 <<  3),
	      IRQ_PREAMBLE_DETECTED = (1 <<  4),
	      IRQ_SYNCWORD_VALID    = (1 <<  5),
	      IRQ_HEADER_ERR        = (1 <<  6),
	      IRQ_ERR               = (1 <<  7),
	      IRQ_CAD_DONE          = (1 <<  8),
	      IRQ_CAD_DETECTED      = (1 <<  9),
	      IRQ_TIMEOUT           = (1 << 10),
	      IRQ_GNSS_DONE         = (1 << 19),
	      IRQ_WIFI_DONE         = (1 << 20),
	      IRQ_LBD               = (1 << 21),
	      IRQ_CMD_ERROR         = (1 << 22),
	      IRQ_ERROR             = (1 << 23),
	      IRQ_FSK_LEN_ERROR     = (1 << 24),
	      IRQ_FSK_ADDR_ERROR    = (1 << 25),
} irq_e;


typedef struct {
    uint32_t nss;
    uint32_t clk;
    uint32_t mosi;
    uint32_t miso;
    uint32_t busy;
    uint32_t irq;
} sample_t;

typedef struct lr1110_data_s {
    sample_t last_sample;
    sample_t sample;

    time_nsecs_t packet_start_time;

    uint32_t accumulated_bits;
    uint32_t accumulated_miso_byte;
    uint32_t accumulated_mosi_byte;

    uint32_t count;

#define NUMBER_BYTES (64)

    uint8_t  mosi[NUMBER_BYTES];
    uint8_t  miso[NUMBER_BYTES];

    uint32_t pending_cmd;
    uint32_t pending_group;

} lr1110_data_t;


#define checkPacketSize(__str, __size) _checkPacketSize(parser, MY_GROUP_STR, __str, __size, cmd)
#define clear_pending_cmd(__cmd) _clear_pending_cmd(parser, MY_GROUP, __cmd)
#define set_pending_cmd(__cmd) _set_pending_cmd(parser, MY_GROUP, __cmd)
#define parse_stat1(__stat1) _parse_stat1(parser, __stat1)
#define parse_stat2(__stat2) _parse_stat2(parser, __stat2)

void     _checkPacketSize(parser_t *parser, char *group_str, char *str, uint32_t size, uint32_t cmd);
void     _clear_pending_cmd(parser_t *parser, uint32_t group, uint32_t cmd);
uint32_t get_24(uint8_t *mosi);
uint32_t get_16(uint8_t *mosi);
uint32_t get_32(uint8_t *mosi);
void     hex_dump(FILE *log_fp, uint8_t *cp, uint32_t count);
void     _parse_stat1(parser_t *parser, uint8_t stat1);
void     _parse_stat2(parser_t *parser, uint8_t stat2);
void     _set_pending_cmd(parser_t *parser, uint32_t group, uint32_t cmd);
