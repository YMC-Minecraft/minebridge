CFLAGS= \
	-I.\
       -std=c99 \
       -Wall \
       -D_POSIX_C_SOURCE=200809L \
       

LDFLAGS= \
	 -ljson-c \
	 -lcurl \
	 -lpthread \


OBJ=main.o environ.o tg/tg.o net/net.o rcon/rcon.o net/curlutils.o mcin/mcin.o

BIN=minebridge

debug: CFLAGS += -fsanitize=address -g3 -O0 -rdynamic
debug: $(BIN)

release: CFLAGS += -DDISABLE_DEBUG
release: $(BIN)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

.PHONY: clean
clean:
	$(RM) *~ $(OBJ) $(BIN)

ifeq ($(PREFIX),)
    PREFIX := /usr/local
endif

install: $(BIN)
	install -d $(DESTDIR)$(PREFIX)/bin/
	install -m 755 $(BIN) $(DESTDIR)$(PREFIX)/bin/
