.SUFFIXES:

# dump_tags = -r

csv_filename = /tmp/digital.csv

elf_filename = ${HOME}/signetik_030923/build/poky/nrf52840-lora-debug/tmp/deploy/images/nrf52840/n1-lora.elf
elf_filename = ${HOME}/moko_081623/build/poky/nrf52840-lora-debug/tmp/deploy/images/nrf52840/lw008.elf
elf_filename = ${HOME}/seeed-wio-wm1110/build/build/zephyr/zephyr.elf

# elf = --elf=${elf_filename}

# no_time_stamp = --no-time-stamp

all : /tmp/digital.lst

.PRECIOUS : /tmp/digital.lst

/tmp/digital.lst : $(csv_filename) ./build/parseSaleae Makefile
	set-title Parse $<
	./build/parseSaleae $(no_uart) $(no_time_stamp) $(dump_tags) $(elf) $< > $@
	set-title Parse `date +"%H:%M" `
