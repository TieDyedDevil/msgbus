#! /usr/bin/env sh

# Be certain that port 5670 can pass UDP traffic.
# Otherwise, you'll see no output.
# $ sudo firewall-cmd --add-port=5670/udp

# Example:
# $ ./demo.sh `./iface.sh

[ $# -eq 1 ] || {
	echo 'interface?'
	exit 1
}
NIF=$1
for i in `seq 101 110`; do echo topic1 Message $i; done \
	|./pubsub foo $NIF 5670 topic1 &
for i in `seq 201 210`; do echo topic1 Message $i; done \
	|./pubsub bar $NIF 5670 topic1 &
wait
echo 'If no output, ensure that port 5670 can pass UDP traffic.'
