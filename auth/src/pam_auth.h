#ifndef PAM_AUTH_H
#define PAM_AUTH_H

/*
 * authenticate - verify username/password against PAM.
 * Returns 0 on success, -1 on failure.
 */
int authenticate(const char *username, const char *password);

#endif
