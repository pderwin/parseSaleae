#!/usr/local/bin/python

f = open("/home/erwin/Downloads/trace/digital.csv", "r")

#
# Get rid of the header line.
#
f.readline()

last_nsecs = 0;

for x in f:

	tokens = x.split(',')

	time_stamp = tokens[0]
	value      = int(tokens[1])

	#
	# get delta time
	#
	msecs = float(time_stamp) * 1000;
	usecs = msecs * 1000;
	nsecs = usecs * 1000;

	#
	# set last time stamp if unknown
	#
	if (last_nsecs == 0):
		last_nsecs = nsecs

	delta = int(nsecs - last_nsecs)

	last_nsecs = nsecs;

	if (delta):
		print("   #%dns;" % (delta) )

	print("   tx_sr[0] = %d;\n" % (value))
