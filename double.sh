#! /usr/bin/env sh

# Demonstrate that each discovery port defines its own pub/sub network.

# Be certain that ports 5670 and 9670 can pass UDP traffic.
# Otherwise, you'll see no output.
# $ sudo firewall-cmd --add-port=5670/udp
# $ sudo firewall-cmd --add-port=9670/udp

# Example:
# $ ./double.sh `./iface.sh

[ $# -eq 1 ] || {
	echo 'interface?'
	exit 1
}
NIF=$1
for i in `seq 101 105`; do echo topic1 Message $i; done \
	|./pubsub foo $NIF 5670 topic1 &
for i in `seq 201 205`; do echo topic1 Message $i; done \
	|./pubsub bar $NIF 5670 topic1 &
for i in `seq 301 305`; do echo topic1 Message $i; done \
	|./pubsub foo $NIF 9670 topic1 &
for i in `seq 401 405`; do echo topic1 Message $i; done \
	|./pubsub bar $NIF 9670 topic1 &
wait
echo 'If missing output, ensure that ports 5670 and 9670 can pass UDP traffic.'
