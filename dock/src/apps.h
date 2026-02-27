#ifndef APPS_H
#define APPS_H

#define MAX_APPS  32
#define MAX_STR   128

typedef struct {
    char id[MAX_STR];
    char name[MAX_STR];
    char icon[MAX_STR];
    int  port;
    int  running;   /* populated at request time via systemd check */
} app_t;

/* Scan /usr/lib/crimata-*/crimata.json — returns app count */
int apps_scan(app_t *apps);

#endif
