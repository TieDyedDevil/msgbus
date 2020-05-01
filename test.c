/*
	Test msgbus.

	Be certain that the firewall allows the discovery port:
	$ sudo firewall-cmd --add-port=5670/udp

	$ ./test foo INTERFACE &
	$ ./test bar INTERFACE &
	where INTERFACE names a broadcast-capable network interface
*/

#include <stdio.h>
#include <assert.h>
#include "msgbus.h"

static void receiver(const char *name, const char *type, const char *peer,
                     const char *peer_uuid, const char *topic, void * userptr,
							const char *msg_text) {
	printf("%s %s %s[%.7s] %s %s\n",
				name, type, peer, peer_uuid, topic, msg_text);
}

int main(int argc, char *argv[]) {
	if (argc != 3) {
		fprintf(stderr, "usage: %s NAME INTERFACE\n", argv[0]);
		return 1;
	}
	const char *topics[] = {
		"topic1", "topic2", NULL
	};
	zactor_t *msgbus = msgbus_open(5670, argv[1], argv[2], topics,
							receiver, NULL);
	assert(msgbus != NULL);
	zclock_sleep(500);
	msgbus_publish(msgbus, "topic1", "Test message");
	msgbus_publish(msgbus, "topic2", "We're done");
	zclock_sleep(500);
	msgbus_close(&msgbus);
	return 0;
}
