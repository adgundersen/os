#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glob.h>
#include "apps.h"

#define MANIFEST_GLOB "/usr/lib/crimata-*/crimata.json"
#define MAX_FILE_SIZE 4096

/* Pull a string value out of a flat JSON object (same approach as auth) */
static int json_str(const char *json, const char *key, char *out, size_t out_len)
{
    char search[128];
    snprintf(search, sizeof(search), "\"%s\"", key);
    const char *pos = strstr(json, search);
    if (!pos) return 0;

    pos += strlen(search);
    while (*pos == ' ' || *pos == ':') pos++;
    if (*pos != '"') return 0;
    pos++;

    size_t i = 0;
    while (*pos && *pos != '"' && i < out_len - 1)
        out[i++] = *pos++;
    out[i] = '\0';
    return 1;
}

static int json_int(const char *json, const char *key, int *out)
{
    char search[128];
    snprintf(search, sizeof(search), "\"%s\"", key);
    const char *pos = strstr(json, search);
    if (!pos) return 0;

    pos += strlen(search);
    while (*pos == ' ' || *pos == ':') pos++;
    if (*pos < '0' || *pos > '9') return 0;

    *out = atoi(pos);
    return 1;
}

static int parse_manifest(const char *path, app_t *app)
{
    FILE *f = fopen(path, "r");
    if (!f) return 0;

    char buf[MAX_FILE_SIZE];
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);
    buf[n] = '\0';

    if (!json_str(buf, "id",   app->id,   sizeof(app->id)))   return 0;
    if (!json_str(buf, "name", app->name, sizeof(app->name))) return 0;
    json_str(buf, "icon", app->icon, sizeof(app->icon));
    json_int(buf, "port", &app->port);
    app->running = 0;

    return 1;
}

int apps_scan(app_t *apps)
{
    glob_t g;
    int count = 0;

    if (glob(MANIFEST_GLOB, 0, NULL, &g) != 0)
        return 0;

    for (size_t i = 0; i < g.gl_pathc && count < MAX_APPS; i++) {
        if (parse_manifest(g.gl_pathv[i], &apps[count]))
            count++;
    }

    globfree(&g);
    return count;
}
