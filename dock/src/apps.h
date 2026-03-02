#ifndef APPS_H
#define APPS_H

#define MAX_APPS  32
#define MAX_STR   256

typedef struct {
    char id[MAX_STR];
    char name[MAX_STR];
    char icon[MAX_STR];
    int  port;
    int  running;             /* populated at request time via systemd check */
    char default_component[MAX_STR];
    char components_json[2048]; /* raw JSON array, e.g. ["contacts.list","contacts.card"] */
    char api_json[8192];        /* raw JSON array of ApiEndpoint objects */
} app_t;

/* Scan /usr/lib/crimata-*/crimata.json — returns app count */
int apps_scan(app_t *apps);

#endif
