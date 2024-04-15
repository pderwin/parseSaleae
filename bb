#!/bin/sh

inotifywait --event close_write /tmp/digital.csv

#
# Seems to be a race with the JSON config file being re-written by the Saleae
# after a trigger occurs.  Sleep a bit to see if the parser remains stable.
#
sleep 2

make --quiet

date
