/*
	Message bus for GliPB
	February 2020
	ADIDRUS project
	Galois, Inc.

	This implements a publish/subscribe messaging model.
	Clients join their peers on a local subnet.
	The clients' peer group is denoted by a specific UDP beacon port.
	Each client may subscribe to one or more topics.
	Topics are denoted by name, represented as a text string.
	A payload is represented as a text string. (Size limit?)
	A client may publish a payload to a topic.
	Clients subscribed to a topic receive payloads published to that topic.
	A client does not receive payloads that it publishes.

	This is a thin wrapper around the zyre API.

	References:
	 https://github.com/zeromq/zyre/blob/master/README.md
	 http://czmq.zeromq.org/
	 http://zguide.zeromq.org/page:all
	 https://rfc.zeromq.org/
*/

#include "msgbus.h"

static void msgbus_recv(zyre_t *node, void* userptr, msgbus_callback_t callback) {
	const char *name = zyre_name(node);
	zyre_event_t *event = zyre_event_new(node);
	const char *type = zyre_event_type(event);
	const char *peer = zyre_event_peer_name(event);
	const char *peer_uuid = zyre_event_peer_uuid(event);
	const char *topic = zyre_event_group(event);
	zmsg_t *message = zyre_event_msg(event);
	const char *msg_text = NULL;
	if (message) msg_text = zmsg_popstr(message);
	(*callback)(name, type, peer, peer_uuid, topic, userptr, msg_text);
	zyre_event_destroy(&event);
	free((void*)msg_text);
}

static void msgbus_send(zyre_t *node, void *src, int *terminated) {
	zmsg_t *msg = zmsg_recv(src);
	if (!msg) *terminated = 1;
	char *cmd = zmsg_popstr(msg);
	if (streq(cmd, "$TERM")) {
		*terminated = 1;
	} else if (streq(cmd, "SHOUT")) {
		char *topic = zmsg_popstr(msg);
		char *text = zmsg_popstr(msg);
		(void)zyre_shouts(node, topic, "%s", text);
		free(topic);
		free(text);
	} else {
		zsys_error("unexpected command %s", cmd);
	}
	free(cmd);
	(void)zmsg_destroy(&msg);
}

typedef struct {
	msgbus_callback_t callback;
	const char **topics;
	const char *name;
	int beacon_port;
	const char *interface;
   void *userptr;
} parms_t;

/* This handles bus communication. */
static void msgbus_actor(zsock_t *pipe, void *args) {
	parms_t *parms = args;
	msgbus_callback_t callback = parms->callback;
	const char **topics = parms->topics;
	const char *name = parms->name;
	int beacon_port = parms->beacon_port;
	const char *interface = parms->interface;
	zyre_t *node = zyre_new(name);
	if (node) {
		zyre_set_port(node, beacon_port);
		if (interface) zyre_set_interface(node, interface);
		if (zyre_start(node) != 0) {
			zyre_destroy(&node);
			return;
		}
		const char **topic;
		for (topic = topics; *topic != NULL; ++topic) {
			(void)zyre_join(node, *topic);
		}
		(void)zsock_signal(pipe, 0);
		int terminated = 0;
		zpoller_t *poller = zpoller_new(pipe, zyre_socket(node), NULL);
		while (!terminated) {
			void *src = zpoller_wait(poller, -1);
			if (src == pipe) {
				msgbus_send(node, src, &terminated);
			} else if (src == zyre_socket(node)) {
				msgbus_recv(node, parms->userptr, callback);
			} else {
				zsys_error("unexpected source");
			}
		}
		zpoller_destroy(&poller);
		zyre_stop(node);
		zclock_sleep(100);
		zyre_destroy(&node);
	}
}

/* Open a new msgbus node using the given port for beacons.
   The msgbus node will advertise the given name.
   If not NULL, use the specified interface for beacons.
   The bus receives message on the given topics.
   Received messages (including administrative) are passed to the callback.
   Return a handle upon success; otherwise NULL. */
zactor_t *msgbus_open(int port, const char *name, const char *interface,
                      const char *topics[], msgbus_callback_t callback,
                      void *userdataptr) {
	if (port < 1024 || name == NULL || topics == NULL || callback == NULL)
		return NULL;
	parms_t * parms = malloc(sizeof(parms_t));
	parms->topics = topics;
	parms->callback = callback;
	parms->name = name;
	parms->beacon_port = port;
	parms->callback = callback;
	parms->interface = interface;
	parms->userptr = userdataptr;
	zactor_t *actor = zactor_new(msgbus_actor, parms);
	return actor;
}

/* Send a message to all peers registered for the given topic.
   Return 0 on success. */
int msgbus_publish(zactor_t *handle, const char *topic, const char *message) {
	if (handle == NULL || topic == NULL || message == NULL) return 1;
	return zstr_sendx(handle, "SHOUT", topic, message, NULL);
}

/* Close a msgbus and invalidate its handle. Return 0 on success. */
int msgbus_close(zactor_t **handle) {
	if (handle == NULL || *handle == NULL) return 1;
	zactor_destroy(handle);
	return 0;
}
