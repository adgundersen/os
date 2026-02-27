#include "session.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>

#define MAX_SESSIONS 1024
#define TOKEN_LEN    32

typedef struct {
    char token[TOKEN_LEN + 1];
    char username[256];
    int  active;
} session_t;

static session_t   sessions[MAX_SESSIONS];
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

/* Generate a random hex token */
static void gen_token(char *out)
{
    static const char hex[] = "0123456789abcdef";
    unsigned char buf[TOKEN_LEN / 2];
    FILE *f = fopen("/dev/urandom", "rb");
    if (f) {
        fread(buf, 1, sizeof(buf), f);
        fclose(f);
    } else {
        /* fallback: not cryptographically strong but avoids hanging */
        srand((unsigned)time(NULL));
        for (size_t i = 0; i < sizeof(buf); i++)
            buf[i] = (unsigned char)rand();
    }
    for (size_t i = 0; i < sizeof(buf); i++) {
        out[i * 2]     = hex[buf[i] >> 4];
        out[i * 2 + 1] = hex[buf[i] & 0xf];
    }
    out[TOKEN_LEN] = '\0';
}

const char *session_create(const char *username)
{
    pthread_mutex_lock(&lock);

    /* Find a free slot */
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (!sessions[i].active) {
            gen_token(sessions[i].token);
            strncpy(sessions[i].username, username, sizeof(sessions[i].username) - 1);
            sessions[i].username[sizeof(sessions[i].username) - 1] = '\0';
            sessions[i].active = 1;
            pthread_mutex_unlock(&lock);
            return sessions[i].token;
        }
    }

    pthread_mutex_unlock(&lock);
    return NULL; /* no free slots */
}

const char *session_lookup(const char *token)
{
    if (!token || !*token) return NULL;

    pthread_mutex_lock(&lock);
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (sessions[i].active &&
            strncmp(sessions[i].token, token, TOKEN_LEN) == 0) {
            pthread_mutex_unlock(&lock);
            return sessions[i].username;
        }
    }
    pthread_mutex_unlock(&lock);
    return NULL;
}

void session_destroy(const char *token)
{
    if (!token || !*token) return;

    pthread_mutex_lock(&lock);
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (sessions[i].active &&
            strncmp(sessions[i].token, token, TOKEN_LEN) == 0) {
            sessions[i].active = 0;
            memset(sessions[i].token,    0, sizeof(sessions[i].token));
            memset(sessions[i].username, 0, sizeof(sessions[i].username));
            break;
        }
    }
    pthread_mutex_unlock(&lock);
}
