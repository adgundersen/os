CC      = gcc
CFLAGS  = -Wall -Wextra -O2
LIBS    = -lpam -lmicrohttpd -lpthread
SRC     = src/main.c src/pam_auth.c src/session.c src/user.c
OUT     = crimata-auth

.PHONY: all clean

all:
	$(CC) $(CFLAGS) $(SRC) $(LIBS) -o $(OUT)

clean:
	rm -f $(OUT)
