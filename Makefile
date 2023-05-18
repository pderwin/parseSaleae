.SUFFIXES:

# dump_tags = -r

csv_filename ?= digital

all : digital.lst

csv_dir = ~/Downloads/trace


.PHONY: RSYNC

RSYNC :
	rsync -a bolide:Downloads/trace/digital.csv ~/Downloads/trace

digital.lst : $(csv_dir)/$(csv_filename).csv ./parseSaleae Makefile
	set-title Parse $<
	./parseSaleae $(no_uart) $(dump_tags) $(elf) $(parsers) $< > $@

uart2.gsv :  $(csv_dir)/$(fname).csv make_uart2.py Makefile
	./make_uart2.py $(csv_dir)/$(fname).csv > $@
