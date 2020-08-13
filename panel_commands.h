#ifndef panel_commands_h
#define panel_commands_h 1

typedef enum {
	UIBC_SW_ENAB             = 0x01,
	UIBC_DISP_ENB            = 0x02,
	UIBC_SPI_CLK_CONFIG_stub = 0x04,
	UIBC_DISP_ENB2           = 0x05,
	UIBC_5V_ENB              = 0x06,
	UIBC_AUX_TOUCH_ENB       = 0x07,
	UIBC_DISP_ENB3           = 0x08, /* GEN3LP */
	UIBC_IRQ_STATUS          = 0x1D,
	UIBC_IRQ_MASK            = 0x1E,
	UIBC_AUX_NFC_ENB         = 0x20,
	UIBC_KEY_FIFO            = 0x32,
	UIBC_LED1_CTRL           = 0x40,
	UIBC_LED1_DUTY           = 0x41,
	UIBC_LED2_CTRL           = 0x42,
	UIBC_LED2_DUTY           = 0x43,
	UIBC_LEDCL_CTRL          = 0x44,
	UIBC_LEDCL_DUTY          = 0x45,
	UIBC_MODBUF_PATTERN_ID   = 0x48,
	UIBC_MODBUF_OFFSET       = 0x49,
	UIBC_MODBUF_DATA         = 0x4a,
	UIBC_LED1_MODULATION     = 0x4b,
	UIBC_LED2_MODULATION     = 0x4c,
	UIBC_BACKLIGHT_DUTY      = 0x80,
	UIBC_SPI_CMD_W           = 0x81,
	UIBC_SPI_DATA_W          = 0x82,
	UIBC_RESET_CMD           = 0x99,

	UIBC_UC_ARCH_ID          = 0xA0, /* GEN3LP, enumerated */
	UIBC_UC_DEVICE_ID        = 0xA1, /* GEN3LP, arch-specific encoding */
	UIBC_UC_DERIVATIVE_ID    = 0xA2, /* GEN3LP, arch-specific encoding */
	UIBC_UC_REVISION_ID      = 0xA3, /* GEN3LP, arch-specific encoding */
	UIBC_UC_UNIQUE_ID        = 0xA4, /* GEN3LP, arch-specific encoding, 128 bit / 16 byte response expected */

	UIBC_RSV_A5              = 0xA5, /* GEN3LP, placeholder for compatibility */

	UIBC_UC_RESET_SRC        = 0xA6, /* GEN3LP, arch-specific encoding */

	UIBC_RSV_A7              = 0xA7, /* GEN3LP, placeholder for compatibility */
	UIBC_RSV_A8              = 0xA8, /* GEN3LP, placeholder for compatibility */
	UIBC_RSV_A9              = 0xA9, /* GEN3LP, placeholder for compatibility */

	UIBC_UC_POWER            = 0xAA, /* GEN3LP, placeholder for compatibility */

	UIBC_RSV_AB              = 0xAB, /* GEN3LP, placeholder for compatibility */
	UIBC_RSV_AC              = 0xAC, /* GEN3LP, placeholder for compatibility */
	UIBC_A2D_CH              = 0xAD, /* GEN3LP 5.2 A2D input channel select */
	UIBC_A2D_RD              = 0xAE, /* GEN3LP 5.2 A2D mV, returns 2 bytes, high byte first, r/o */
	UIBC_CFG                 = 0xAF, /* GEN3LP 5.2 misc cfg/mode flags */

	UIBC_TST_MAGIC0          = 0xE6,
	UIBC_TST_MAGIC1          = 0xED,
	UIBC_TST_ECHO            = 0xEE,

	UIBC_PANEL_ID            = 0xF0,
	UIBC_PANEL_VERSION       = 0xF1,
	UIBC_PANEL_VERSION2_rsv  = 0xF2, /* GEN3LP, placeholder for compatibility */

	AVR_EEPROM_BYTE_W_stub   = 0xFA, /* unsupported, stub only */

	UIBC_FW_VERSION_MAJ      = 0xFE,
	UIBC_FW_VERSION_MIN      = 0xFF,

} panel_commands_e;


#endif
