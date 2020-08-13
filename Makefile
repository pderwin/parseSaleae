.SUFFIXES:

# dump_tags = -r

all : Objs
all : digital.lst
# all : jake-full.lst

workspace = /home/erwin/panel_051222
workspace = /home/erwin/jake-v-firmware.041522

# elf = --elf=$(workspace)/build/zephyr-jake-v.elf
zephyr_includes = $(workspace)/source/inc/drivers


zephyr_source   = $(workspace)/source/zephyr-jake-v
zephyr_includes = $(zephyr_source)/include/drivers

auto_download = /home/erwin/auto-download


csv_dir = ~/Downloads/trace

fname = digital

libs = $(HOME)/libs

# clock \

files = \
	addr2line \
	bootloader_i2c \
	csv \
	hdr \
	hex \
	lbtrace \
	localtime \
	log_file \
	lookup \
	packet \
	panel_i2c \
	panel_spi \
	parseSaleae \
	parser \
	parser_backlight \
	parser_cbi \
	parser_ftdi_di \
	parser_ftdi_do \
	parser_hpal \
	parser_i2c \
	parser_lbtrace \
	parser_lbtrace_old \
	parser_lr1110_spi \
	parser_spi_device \
	parser_spi_uart \
	parser_hsync \
	parser_uart \
	parser_vsync \
	st25dv \
	tag

#	parser_uart \
#	lbtrace
#	lbtrace_word
#	rts

includes = \
	-I.       \
	-I$(libs) \
	-I$(zephyr_includes)


desc.lst : digital.lst Makefile
	echo "egrep $< into $@"
	egrep -e DESC_WRITE -e FT60X_ISR_BAND -e VIDEO_DMA_ISR -e UPDATE_CHAIN digital.lst > $@

.PHONY: RSYNC

RSYNC :
	rsync -a chiron:Downloads/trace/digital.csv ~/Downloads/trace

%.lst : $(csv_dir)/%.csv ./parseSaleae Makefile
	@echo "Parsing Trace file:" $< ;
	./parseSaleae $(no_uart) $(dump_tags) $(elf) $(parsers) $<

oFiles := $(files:%=Objs/%.o)

# parseSaleae.h $(zephyr_includes)/trace.h $(auto_download)/syscall_list.h

$(zephyr_includes)/trace.h :
	echo "Missing trace.h file: $@"
	exit 1

Objs/%.o : %.c parseSaleae.h $(zephyr_includes)/trace.h
	@echo [cc] $< ; \
	gcc -g -c $(includes) -DZEPHYR_INCLUDES=\"$(zephyr_includes)\" -o $@ -Wall -Werror $<


parseSaleae: $(oFiles) $(libs)/libcsv.so $(libs)/libgdb.a
	@echo [ln] $@ ;
	gcc -g $(includes) -o $@ -Wall -Werror  $(oFiles) -L$(libs) -L$(HOME)/libs/gdb -lcsv -lgdb -lstdc++ -lm -lreadline -ltinfo -lexpat -lpthread -ldl -lz -Wl,-rpath=$(libs)

Objs :
	mkdir Objs

uart2.gsv :  $(csv_dir)/$(fname).csv make_uart2.py Makefile
	./make_uart2.py $(csv_dir)/$(fname).csv > $@
