.SUFFIXES:

# dump_tags = -r

csv_filename = ${HOME}/Downloads/digital_almanac_update.csv
csv_filename = ${HOME}/Downloads/digital_reset_synch_wifi.csv
csv_filename = ${HOME}/Downloads/trace/digital.csv

elf_filename = ${HOME}/signetik_030923/build/poky/nrf52840-lora-debug/tmp/deploy/images/nrf52840/n1-lora.elf
elf_filename = ${HOME}/moko_081623/build/poky/nrf52840-lora-debug/tmp/deploy/images/nrf52840/lw008.elf

elf = --elf=${elf_filename}

# no_time_stamp = --no-time-stamp

all : digital.lst

.PHONY: RSYNC

RSYNC :
	rsync -a bolide:Downloads/trace/digital.csv ~/Downloads/trace

digital.lst : $(csv_filename) ./parseSaleae Makefile
	set-title Parse $<
	./parseSaleae $(no_uart) $(no_time_stamp) $(dump_tags) $(elf) $< > $@
	set-title Parse `date +"%H:%M" `

uart2.gsv :  $(csv_dir)/$(fname).csv make_uart2.py Makefile
	./make_uart2.py $(csv_dir)/$(fname).csv > $@


gnss.lst: digital.lst Makefile
	egrep -e GNSS -e "SET_TX " -e "WRITE_BUFFER_8" -e WIFI lr1110_semtech.log > $@

GREP_ARGS = -e RADIO -e SYSTEM

n1.log : lr1110_n1.log Makefile
	egrep $(GREP_ARGS) $< > $@

n1_ok.log : lr1110_n1_ok.log Makefile
	egrep $(GREP_ARGS) $< > $@


n1.tx.log : n1.log n1_ok.log
	printf "\nn1: \n\n" >> $@
	cat n1.log > $@
	printf "\n\nn1_ok: \n\n" >> $@
	cat n1_ok.log >> $@

joins.lst : ${HOME}/Downloads/trace/joins.csv
	./parseSaleae $<
	cp lr1110_n1.log $@
