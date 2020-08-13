typedef enum
    {
     SPI_CMD_SET_COL_ADDRESS_LSB = 0x00,
     SPI_CMD_SET_COL_ADDRESS_MSB = 0x10,
     SPI_CMD_REGULATION_RATIO    = 0x20,
     SPI_CMD_POWER_CONTROL       = 0x28,
     SPI_CMD_DISPLAY_START_LINE  = 0x40,
     SPI_CMD_EV                  = 0x81,
     SPI_CMD_SEG_DIRECTION       = 0xa0,
     SPI_CMD_COM_DIRECTION       = 0xc0,
     SPI_CMD_BIAS_SELECT         = 0xa2,
     SPI_CMD_INVERSE_DISPLAY     = 0xa6,
     SPI_CMD_DISPLAY_ON          = 0xae,
     SPI_CMD_SET_PAGE_ADDRESS    = 0xb0,
     SPI_CMD_SET_BOOSTER         = 0xf8,
    } panel_spi_commands_e;
