#ifndef USER_H
#define USER_H

/*
 * Create a new Linux user with useradd + set password via chpasswd.
 * Returns 0 on success, -1 on error (user already exists, permission denied, etc.)
 */
int user_create(const char *username, const char *password);

#endif
