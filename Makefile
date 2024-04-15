.SUFFIXES:


# dump_tags = -r

config_filename = ${HOME}/tmp.json
config_filename = ${HOME}/.config/Logic/config.json

csv_filename = /tmp/digital.csv

# dump_tags = -t

elf_filename = ${HOME}/seeed-wio-wm1110/build/build/zephyr/zephyr.elf

elf = --elf=${elf_filename}

# no_time_stamp = --no-time-stamp

signal_names = ./saleae.signal_names

all : /tmp/digital.lst

.PRECIOUS : /tmp/digital.lst

/tmp/digital.lst : $(csv_filename) ./build/parseSaleae Makefile ~/tmp.json $(signal_names)
	set-title Parse $<
	-./build/parseSaleae \
		--signal-names=$(signal_names) \
		$(no_time_stamp) $(dump_tags) $(elf) $<  > $@
	head -20 $@
	set-title Parse `date +"%H:%M" `

$(signal_names) : ./find_signal_names
	./find_signal_names $(config_filename) > $@

find_signal_names : find_signal_names.c
	gcc -Wall -Werror $< -o $@
