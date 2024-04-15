cmake_minimum_required(VERSION 3.22)

project(parseSaleae)

# set(CMAKE_VERBOSE_MAKEFILE ON)

set(ZEPHYR "/home/erwin/seeed-wio-wm1110/source/zephyr" )

add_compile_options(-Wall -Werror -Wextra -Wno-missing-field-initializers)

add_executable(parseSaleae
  addr2line.c
  csv.c
  hdr.c
  hex.c
  i2c_parser.c
  lbtrace.c
  localtime.c
  log_file.c
  panel_spi.c
  parseSaleae.c
  parser.c
  parser_uart.c
  parser_uart_lbtrace.c
  signal.c
  signal_names.c
  tag.c
  thread.c

  lis3dh/lis3dh.c

  lr1110/crypto.c
  lr1110/parser.c
  lr1110/gnss.c
  lr1110/radio.c
  lr1110/system.c
  lr1110/wifi.c
  )

include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )
include_directories( $ENV{HOME}/libs )

target_include_directories(parseSaleae PUBLIC ${ZEPHYR}/include )

add_compile_definitions(ZEPHYR_INCLUDES="${ZEPHYR}/include" )

find_library( CSV_LIBRARY NAMES csv PATHS   /home/erwin/libs )
find_library( GDB_LIBRARY NAMES gdb PATHS   /home/erwin/libs )
find_library( STDC_LIBRARY NAMES stdc++ PATHS /usr/lib/gcc/x86_64-linux-gnu/9 )

target_link_libraries(parseSaleae ${CSV_LIBRARY} ${GDB_LIBRARY} gmp mpfr ncurses stdc++ m readline tinfo expat pthread dl z )
