LDFLAGS = -L/usr/local/lib -Wl,-rpath=/usr/local/lib -lzyre -lczmq -lzmq
CFLAGS = -ggdb -fPIC
PREFIX = /usr/local

all: test pubsub libs

libs: libmsgbus.so libmsgbus.a

test: test.c msgbus.o

pubsub: pubsub.c msgbus.o

msgbus.o: msgbus.c msgbus.h

libmsgbus.so: msgbus.o
	$(CC) -shared -Wl,-soname,$@.1 -o $@ $<

libmsgbus.a: msgbus.o
	$(AR) rs $@ $<

install:
	install -d $(PREFIX)/bin
	install pubsub $(PREFIX)/bin
	install -d $(PREFIX)/man/man1
	install -m 644 pubsub.1 $(PREFIX)/man/man1
	install -d $(PREFIX)/man/man3
	install -m 644 msgbus.3 $(PREFIX)/man/man3
	install -d $(PREFIX)/lib
	install libmsgbus.so $(PREFIX)/lib/libmsgbus.so.1.0.1
	( cd $(PREFIX)/lib; ln -sf libmsgbus.so.1.0.1 libmsgbus.so.1 )
	( cd $(PREFIX)/lib; ln -sf libmsgbus.so.1.0.1 libmsgbus.so )
	install -m 644 libmsgbus.a $(PREFIX)/lib
	install -d $(PREFIX)/include
	install -m 644 msgbus.h $(PREFIX)/include

uninstall:
	rm -f $(PREFIX)/bin/pubsub $(PREFIX)/man/man1/pubsub.1 \
		$(PREFIX)/man/man3/msgbus.3 $(PREFIX)/lib/libmsgbus.*

clean:
	rm -f test pubsub *.o *.a *.so
