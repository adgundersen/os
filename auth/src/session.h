#ifndef SESSION_H
#define SESSION_H

/* Create a session for username — returns token string (valid until destroyed) */
const char *session_create(const char *username);

/* Look up a token — returns username or NULL if not found */
const char *session_lookup(const char *token);

/* Destroy a session */
void session_destroy(const char *token);

#endif
