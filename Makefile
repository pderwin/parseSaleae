.SUFFIXES:

# dump_tags = -r

csv_filename = ${HOME}/Downloads/digital_almanac_update.csv
csv_filename = ${HOME}/Downloads/trace/digital.csv
csv_filename = ${HOME}/Downloads/digital_reset_synch_wifi.csv


# no_time_stamp = --no-time-stamp

all : digital.lst

.PHONY: RSYNC

RSYNC :
	rsync -a bolide:Downloads/trace/digital.csv ~/Downloads/trace

digital.lst : $(csv_filename) ./parseSaleae Makefile
	set-title Parse $<
	./parseSaleae $(no_uart) $(no_time_stamp) $(dump_tags) $(elf) $< > $@

uart2.gsv :  $(csv_dir)/$(fname).csv make_uart2.py Makefile
	./make_uart2.py $(csv_dir)/$(fname).csv > $@


gnss.lst: digital.lst Makefile
	egrep -e GNSS -e "SET_TX " -e "WRITE_BUFFER_8" -e WIFI lr1110_semtech.log > $@
