#pragma once

#define PENDING (0x80000000)

enum {
      GET_STATUS               = 0x0100,
      GET_VERSION              = 0x0101,
      WRITE_BUFFER_8           = 0x0109,
      GET_ERRORS               = 0x010d,
      CLEAR_ERRORS             = 0x010e,
      SET_REG_MODE             = 0x0110,
      SET_DIO_AS_RF_SWITCH     = 0x0112,
      SET_DIO_IRQ_PARAMS       = 0x0113,
      CLEAR_IRQ                = 0x0114,
      SET_STANDBY              = 0x011c,

      GET_PACKET_TYPE          = 0x0202,
      SET_LORA_PUBLIC_NETWORK  = 0x0208,
      SET_RX                   = 0x0209,
      SET_TX                   = 0x020a,
      SET_RF_FREQUENCY         = 0x020b,
      SET_PACKET_TYPE          = 0x020e,
      SET_MODULATION_PARAM     = 0x020f,
      SET_PACKET_PARAM         = 0x0210,
      SET_TX_PARAMS            = 0x0211,
      SET_PA_CFG               = 0x0215,
      STOP_TIMEOUT_ON_PREAMBLE = 0x0217,
      LORA_SYNCH_TIMEOUT       = 0x021b,
      SET_RX_BOOSTED           = 0x0227,
};

typedef enum {
      PACKET_TYPE_NONE    = 0,
      PACKET_TYPE_GFSK    = 1,
      PACKET_TYPE_LORA    = 2,
} packet_type_e;
