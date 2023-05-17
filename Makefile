.SUFFIXES:

# dump_tags = -r

all : digital.lst

csv_dir = ~/Downloads/trace


.PHONY: RSYNC

RSYNC :
	rsync -a bolide:Downloads/trace/digital.csv ~/Downloads/trace

digital.lst : $(csv_dir)/digital.csv ./parseSaleae Makefile
	cp lr1110.log lr1110.log.last
	./parseSaleae $(no_uart) $(dump_tags) $(elf) $(parsers) $< > $@

uart2.gsv :  $(csv_dir)/$(fname).csv make_uart2.py Makefile
	./make_uart2.py $(csv_dir)/$(fname).csv > $@
