#pragma once

#define PENDING (0x80000000)

enum {
      GET_STATUS                  = 0x0100,
      GET_VERSION                 = 0x0101,
      WRITE_BUFFER_8              = 0x0109,
      READ_BUFFER_8               = 0x010a,
      WRITE_REG_MEM_MASK32        = 0x010c,
      GET_ERRORS                  = 0x010d,
      CLEAR_ERRORS                = 0x010e,
      CALIBRATE                   = 0x010f,
      SET_REG_MODE                = 0x0110,
      SET_DIO_AS_RF_SWITCH        = 0x0112,
      SET_DIO_IRQ_PARAMS          = 0x0113,
      CLEAR_IRQ                   = 0x0114,
      CONFIG_LF_CLOCK             = 0x0116,
      SET_TCXO_MODE               = 0x0117,
      REBOOT                      = 0x0118,
      GET_VBAT                    = 0x0119,
      SET_SLEEP                   = 0x011b,
      SET_STANDBY                 = 0x011c,
      GET_RANDOM                  = 0x0120,
      GET_DEV_EUI                 = 0x0125,
      GET_JOIN_EUI                = 0x0126,
      READ_DEVICE_PIN             = 0x0127,

      GET_STATS                   = 0x0201,
      GET_PACKET_TYPE             = 0x0202,
      GET_RX_BUFFER_STATUS        = 0x0203,
      GET_PACKET_STATUS           = 0x0204,
      SET_LORA_PUBLIC_NETWORK     = 0x0208,
      SET_RX                      = 0x0209,
      SET_TX                      = 0x020a,
      SET_RF_FREQUENCY            = 0x020b,
      SET_PACKET_TYPE             = 0x020e,
      SET_MODULATION_PARAM        = 0x020f,
      SET_PACKET_PARAM            = 0x0210,
      SET_TX_PARAMS               = 0x0211,
      SET_PA_CFG                  = 0x0215,
      STOP_TIMEOUT_ON_PREAMBLE    = 0x0217,
      SET_TXCW                    = 0x0219,
      LORA_SYNCH_TIMEOUT          = 0x021b,
      SET_RX_BOOSTED              = 0x0227,
      SEMTECH_UNKNOWN_0229        = 0x0229,
      SEMTECH_UNKNOWN_022b        = 0x022b,

      GNSS_SET_CONSTELLATION      = 0x0400,
      GNSS_SCAN_AUTONOMOUS        = 0x0409,
      GNSS_GET_RESULT_SIZE        = 0x040c,
      GNSS_READ_RESULTS           = 0x040d,
      // not documented this way      GNSS_SCAN_AUTONOMOUS        = 0x0430,

      CRYPTO_SET_KEY              = 0x0502,
      CRYPTO_DERIVE_AND_STORE_KEY = 0x0503,
      CRYPTO_COMPUTE_AES_CMAC     = 0x0505,
      CRYPTO_STORE_TO_FLASH       = 0x050a,
      CRYPTO_RESTORE_FROM_FLASH   = 0x050b,

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
