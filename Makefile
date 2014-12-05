LDFLAGS += $(shell pkg-config --libs json-c)
CFLAGS += $(shell pkg-config --cflags json-c)

OBJS = relai_server.c relai_gpio.c sha1.c base64.c ws_protocol.c

a:
all:
	gcc $(OBJS) -o relai_server $(CFLAGS)  $(LDFLAGS) -pthread -std=c99 -g -lwiringPi 
