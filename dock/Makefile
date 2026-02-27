CC     = gcc
CFLAGS = -Wall -Wextra -O2
LIBS   = -lmicrohttpd -lsystemd
SRC    = src/main.c src/apps.c src/systemd.c
OUT    = crimata-dock

.PHONY: all clean

all:
	$(CC) $(CFLAGS) $(SRC) $(LIBS) -o $(OUT)

clean:
	rm -f $(OUT)
