#ifndef SYSTEMD_H
#define SYSTEMD_H

/* Returns 1 if active, 0 if inactive/not found, -1 on error */
int  systemd_is_active(const char *unit);

/* Returns 0 on success, -1 on error */
int  systemd_start(const char *unit);
int  systemd_stop(const char *unit);

#endif
