#! /usr/bin/env sh

# Run the test program.
# Be certain that port 5670 can pass UDP traffic.
# $ sudo firewall-cmd --add-port=5670/udp

# Example:
# $ ./test.sh `./iface.sh

[ $# -eq 1 ] || {
	echo 'interface?'
	exit 1
}
NIF=$1
./test foo $NIF &
./test bar $NIF &
wait
echo 'If no output, ensure that port 5670 can pass UDP traffic.'
