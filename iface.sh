#! /usr/bin/env sh

# Print the name of an active broadcast-capable interface.
ip link | grep -v NO-CARRIER | grep BROADCAST | grep UP,LOWER_UP \
	| head -1 | cut -d: -f2
