/*
	Publish/subscribe message bus

	Be certain that the firewall allows the discovery port:
	$ sudo firewall-cmd --add-port=PORT/udp

	Note that port 5670 is the IANA-registered port for this use.

	Text to standard input is a topic name followed by whitespace
	and the message text up to, but not including, the newline.
	The message text is limited to 511 bytes; truncation is not logged.

	Received messages are written to standard output as
	PEER\tTOPIC\tTEXT...\n

	PEER presents as NAME[UUID] where NAME is the peer's given name
	and UUID is the first seven characters of the hexadecimal
	representation of the peer's UUID. NAME is not guaranteed unique.

	Errors are logged to standard error.

	Terminate upon EOF or SIGTERM.

	To prevent suspension when run in background, do
	tail -f /dev/null|./pubsub ... &
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "msgbus.h"

static void receiver(const char *name, const char *type, const char *peer,
			const char *peer_uuid, const char *topic,
                     void * userptr,
							const char *msg_text) {
	if (streq(type, "SHOUT"))
		fprintf(stdout, "%s[%.7s]\t%s\t%s\n",
					peer, peer_uuid, topic, msg_text);
		fflush(stdout);
}

static int read_pubmsg(const char **pub_topic, const char **pub_text) {
	const char *wsp = " \t";
	static char msg[512];
	if (!fgets(msg, 512, stdin)) return -1;
	msg[strlen(msg)-1] = '\0';
	char *topic = msg;
	size_t topic_end;
	topic[topic_end = strcspn(topic, wsp)] = '\0';
	if (topic_end == 0) {
		zsys_error("no topic");
		return 1;
	}
	const char *text =
		&msg[topic_end+1+strspn(&msg[topic_end+1], wsp)];
	if (strlen(text) == 0) {
		zsys_error("no text");
		return 1;
	}
	*pub_topic = topic;
	*pub_text = text;
	return 0;
}

int main(int argc, char *argv[]) {
	int subonly = 0;
	if ((argc > 1 && (subonly = !strcmp(argv[1], "-l")) && argc < 6) || argc < 5) {
		fprintf(stderr, "usage: %s [-l] NAME INTERFACE PORT TOPIC...\n",
								argv[0]);
		return 1;
	}
	const char *vcs =
		"abcdefghijklmnopqrstuvwxyz"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"01234567890._+-/";
	if (subonly) ++argv;
	if (strlen(argv[1]) == 0 || strspn(argv[1], vcs) != strlen(argv[1])) {
		fprintf(stderr, "%s:%s:name?\n", argv[0], argv[1]);
		return 1;
	}
	for (const char **tc = (const char**)&argv[4]; *tc; ++tc) {
		if (strlen(*tc) == 0 || strspn(*tc, vcs) != strlen(*tc)) {
			fprintf(stderr, "%s:%s:topic?\n", argv[0], *tc);
			return 1;
		}
	}
	zsys_set_logstream(stderr);
	zactor_t *msgbus = msgbus_open(atoi(argv[3]), argv[1],
					*argv[2] ? argv[2] : NULL,
               (const char**)&argv[4], receiver, NULL);
	if (msgbus == NULL) {
		zsys_error("msgbus_open failed");
		return 1;
	}
	zsys_handler_set(NULL);
	zclock_sleep(100);  /* give the actor and socket time to start */
	while (!zsys_interrupted) {
		if (subonly) {
			pause();
		} else {
			const char *topic, *text;
			int rc = read_pubmsg(&topic, &text);
			if (rc == -1) break;  /* EOF */
			if (rc == 1) continue;  /* parse failure */
			if (msgbus_publish(msgbus, topic, text) != 0)
				zsys_error("msgbus_publish failed");
		}
	}
	zclock_sleep(100);  /* give in-flight messages a chance to arrive */
	(void)msgbus_close(&msgbus);
	return 0;
}
