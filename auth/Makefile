CC      = gcc
CFLAGS  = -Wall -Wextra -O2
LIBS    = -lpam -lmicrohttpd
SRC     = src/main.c src/pam_auth.c
OUT     = crimata-auth

.PHONY: all clean

all:
	$(CC) $(CFLAGS) $(SRC) $(LIBS) -o $(OUT)

clean:
	rm -f $(OUT)
