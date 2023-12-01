#!/bin/sh

inotifywait --event close_write ~/Downloads/trace/digital.csv

make --quiet

date
