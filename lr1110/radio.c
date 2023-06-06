#include <stdlib.h>
#include "hdr.h"
#include "lr1110.h"
#include "radio.h"

#define MY_GROUP     LR1110_GROUP_RADIO
#define MY_GROUP_STR "RADIO"

enum {
      GET_STATS                   = 0x01,
      GET_PACKET_TYPE             = 0x02,
      GET_RX_BUFFER_STATUS        = 0x03,
      GET_PACKET_STATUS           = 0x04,
      SET_LORA_PUBLIC_NETWORK     = 0x08,
      SET_RX                      = 0x09,
      SET_TX                      = 0x0a,
      SET_RF_FREQUENCY            = 0x0b,
      SET_PACKET_TYPE             = 0x0e,
      SET_MODULATION_PARAM        = 0x0f,
      SET_PACKET_PARAM            = 0x10,
      SET_TX_PARAMS               = 0x11,
      SET_PA_CFG                  = 0x15,
      STOP_TIMEOUT_ON_PREAMBLE    = 0x17,
      SET_TXCW                    = 0x19,
      LORA_SYNCH_TIMEOUT          = 0x1b,
      SET_RX_BOOSTED              = 0x27,
      RADIO_SET_RSSI_CALIBRATION  = 0x29,
      SET_LORA_SYNC_WORD          = 0x2b,
};

static packet_type_e packet_type    = PACKET_TYPE_NONE;
static uint32_t      payload_length = 0;

void lr1110_radio(parser_t *parser)
{
    uint8_t
	*miso,
	*mosi;
    uint32_t
	cmd,
	freq,
	timeout;
    lr1110_data_t
	*data = parser->data;
    FILE
	*log_fp = parser->lf->fp;

    mosi = data->mosi;
    miso = data->miso;

    cmd = get_command(data);

    switch (cmd) {

    case GET_PACKET_STATUS:
	checkPacketSize("GET_PACKET_STATUS", 2);
	set_pending_cmd(GET_PACKET_STATUS);
	break;

    case RESPONSE(GET_PACKET_STATUS):
	checkPacketSize("GET_PACKET_STATUS (resp)", 4);

	parse_stat1(miso[0]);

	fprintf(log_fp, "rssipkt: %x ",         miso[1]);
	fprintf(log_fp, "snrpkt: %x ",          miso[2]);
	fprintf(log_fp, "signal_rssi_pkt: %x", miso[3]);
	break;

    case GET_PACKET_TYPE:
	checkPacketSize("GET_PACKET_TYPE", 2);
	set_pending_cmd(GET_PACKET_TYPE);
	break;

    case RESPONSE(GET_PACKET_TYPE):
	checkPacketSize("GET_PACKET_TYPE (resp)", 2);

	fprintf(log_fp, "type: %x", miso[1]);
	break;


    case GET_RX_BUFFER_STATUS:
	checkPacketSize("GET_RX_BUFFER_STATUS", 2);
	set_pending_cmd(GET_RX_BUFFER_STATUS);
	break;

    case RESPONSE(GET_RX_BUFFER_STATUS):
	checkPacketSize("GET_RX_BUFFER_STATUS (resp)", 3);

	parse_stat1(miso[0]);

	fprintf(log_fp, "payload length: %d ", miso[1] );
	fprintf(log_fp, "RX buffer start: %x ", miso[2] );
	break;

    case GET_STATS:
	checkPacketSize("GET_STATS", 2);
	set_pending_cmd(GET_STATS);
	break;

    case RESPONSE(GET_STATS):
	checkPacketSize("GET_STATS (resp)", 9);

	parse_stat1(miso[0]);

	fprintf(log_fp, "nbPackets: %d ", get_16(&miso[1]) );
	fprintf(log_fp, "crc error: %d ", get_16(&miso[3]) );
	fprintf(log_fp, "hdr error: %d ", get_16(&miso[5]) );
	fprintf(log_fp, "false sync: %d", get_16(&miso[7]) );
	break;

    case LORA_SYNCH_TIMEOUT:
	checkPacketSize("LORA_SYNCH_TIMEOUT", 3);
	fprintf(log_fp, "symbol: %02x ( should be zero )", mosi[2]);
	break;

    case RADIO_SET_RSSI_CALIBRATION:

	checkPacketSize("RADIO_SET_RSSI_CALIBRATION", 13);

	fprintf(log_fp, "Bytes: ");

	hex_dump(mosi, 13);

	break;

    case SET_LORA_SYNC_WORD:
	checkPacketSize("SET_LORA_SYNC_WORD", 3);

	fprintf(log_fp, "Word: %x", mosi[2]);
	break;

    case SET_LORA_PUBLIC_NETWORK:
	checkPacketSize("SET_LORA_PUBLIC_NETWORK", 3);
	fprintf(log_fp, "network: %02x", mosi[2]);
	break;

    case SET_MODULATION_PARAM:

	checkPacketSize("SET_MODULATION_PARAM", 6);

	fprintf(log_fp, "SF: %02x ", mosi[2]);
	fprintf(log_fp, "BWL: %02x ", mosi[3]);
	fprintf(log_fp, "CR: %02x ", mosi[4]);
	fprintf(log_fp, "lowDROpt: %02x", mosi[5]);
	break;

    case SET_PA_CFG:
	checkPacketSize("SET_PA_CFG", 6);

	fprintf(log_fp, "PaSel: %02x, ",  mosi[2]);
	fprintf(log_fp, "RegPaSupply: %02x, ",   mosi[3]);
	fprintf(log_fp, "PaDutyCycle: %02x, ",   mosi[4]);
	fprintf(log_fp, "PaHpSel: %02x",   mosi[5]);
	break;

    case SET_PACKET_PARAM:

	switch(packet_type) {

	default:
	    fprintf(log_fp, "SET_PACKET_PARAM: unknown packet type: %x.  Assume LORA\n", packet_type);

	    /* fall-through */

	case PACKET_TYPE_LORA:
	    checkPacketSize("SET_PACKET_PARAM (LORA)", 8);
	    fprintf(log_fp, "PbLength: %04x ",  (mosi[2] << 8) | mosi[3]);
	    fprintf(log_fp, "HeaderType: %02x ",  mosi[4]);

	    payload_length = mosi[5];

	    fprintf(log_fp, "PayloadLen: %02x ",  mosi[5]);
	    fprintf(log_fp, "CRC: %02x ",  mosi[6]);
	    fprintf(log_fp, "InvertIQ: %02x ",  mosi[7]);
	    break;
	}
	break;

    case SET_PACKET_TYPE:
	checkPacketSize("SET_PACKET_TYPE", 3);

	packet_type = mosi[2];

	fprintf(log_fp, "PacketType: %02x ", packet_type);
	break;

    case SET_RF_FREQUENCY:
	checkPacketSize("SET_RF_FREQUENCY", 6);

	freq = (mosi[2] << 24) | (mosi[3] << 16) | (mosi[4] << 8) | (mosi[5] << 0);
	fprintf(log_fp, "Frequency: %04x (%d)",  freq, freq);
	break;

    case SET_RX:
	checkPacketSize("SET_RX", 5);

	timeout = (mosi[2] << 16) | (mosi[3] << 8) | mosi[4];

	fprintf(log_fp, "timeout: 0x%x (%d)", timeout, timeout);
	break;

    case SET_RX_BOOSTED:
	checkPacketSize("SET_RX_BOOSTED", 3);
	fprintf(log_fp, "boosted: %02x", mosi[2]);
	break;

    case SET_TX:
	checkPacketSize("SET_TX", 5);

	timeout = (mosi[2] << 16) | (mosi[3] << 8) | mosi[4];

	fprintf(log_fp, "timeout: 0x%x (%d)", timeout, timeout);
	break;

    case SET_TXCW:
	checkPacketSize("SET_TXCW", 2);
	break;

    case SET_TX_PARAMS:
	checkPacketSize("SET_TX_PARAMS", 4);
	fprintf(log_fp, "TxPower: %02x ",  mosi[2]);
	fprintf(log_fp, "RampTime: %02x ",  mosi[3]);
	break;

    case STOP_TIMEOUT_ON_PREAMBLE:
	checkPacketSize("STOP_TIMEOUT_ON_PREAMBLE", 3);
	fprintf(log_fp, "stop: %02x", mosi[2]);
	break;

    default:
	hdr(parser->lf, data->packet_start_time, "UNHANDLED_RADIO_CMD");
	fprintf(log_fp, "Unhandled RADIO command: %04x length: %d \n", cmd, data->count);
	exit(1);

    }
}
