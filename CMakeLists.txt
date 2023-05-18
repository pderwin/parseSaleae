cmake_minimum_required(VERSION 3.22)

project(parseSaleae)

add_compile_options(-Wall -Werror -Wextra -Wno-missing-field-initializers)

add_executable(parseSaleae
	addr2line.c
	bootloader_i2c.c
	csv.c
	hdr.c
	hex.c
	lbtrace.c
	localtime.c
	log_file.c
	lookup.c
	packet.c
	panel_i2c.c
	panel_spi.c
	parseSaleae.c
	parser.c
	parser_backlight.c
	parser_cbi.c
	parser_ftdi_di.c
	parser_ftdi_do.c
	parser_hpal.c
	parser_i2c.c
	parser_lbtrace.c
	parser_lr1110_spi.c
	parser_spi_device.c
	parser_spi_uart.c
	parser_hsync.c
	parser_uart.c
	parser_vsync.c
	signal.c
	st25dv.c
	tag.c )

target_include_directories(parseSaleae PUBLIC /home/erwin/libs )

target_include_directories(parseSaleae PUBLIC /home/erwin/jake-v-firmware.041522/source/zephyr-jake-v/include)
target_include_directories(parseSaleae PUBLIC /home/erwin/jake-v-firmware.041522/source/zephyr-jake-v/include/drivers)
target_include_directories(parseSaleae PUBLIC /home/erwin/jake-v-firmware.041522/build/zephyr/include/generated)

add_compile_definitions(ZEPHYR_INCLUDES="/home/erwin/jake-v-firmware.041522/source/zephyr-jake-v/include" )

find_library( CSV_LIBRARY NAMES csv PATHS   /home/erwin/libs )
find_library( GDB_LIBRARY NAMES gdb PATHS   /home/erwin/libs )
find_library( STDC_LIBRARY NAMES stdc++ PATHS /usr/lib/gcc/x86_64-linux-gnu/9 )

target_link_libraries(parseSaleae ${CSV_LIBRARY} ${GDB_LIBRARY} stdc++ m readline tinfo expat pthread dl z )
