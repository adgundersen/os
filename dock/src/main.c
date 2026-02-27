#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <microhttpd.h>
#include "apps.h"
#include "systemd.h"

#define PORT     7701
#define BUF_SIZE 16384

/* ── JSON response helper ─────────────────────────────────────────────────── */

static enum MHD_Result send_json(struct MHD_Connection *conn,
                                  unsigned int status, const char *body)
{
    struct MHD_Response *resp = MHD_create_response_from_buffer(
        strlen(body), (void *)body, MHD_RESPMEM_MUST_COPY);
    MHD_add_response_header(resp, "Content-Type", "application/json");
    MHD_add_response_header(resp, "Access-Control-Allow-Origin", "*");
    enum MHD_Result r = MHD_queue_response(conn, status, resp);
    MHD_destroy_response(resp);
    return r;
}

/* ── GET /apps ────────────────────────────────────────────────────────────── */

static enum MHD_Result handle_list(struct MHD_Connection *conn)
{
    app_t apps[MAX_APPS];
    int   count = apps_scan(apps);

    /* Check running status for each app */
    for (int i = 0; i < count; i++) {
        char unit[MAX_STR + 16];
        snprintf(unit, sizeof(unit), "crimata-%s.service", apps[i].id);
        apps[i].running = (systemd_is_active(unit) == 1) ? 1 : 0;
    }

    /* Build JSON array */
    char buf[BUF_SIZE];
    int  pos = 0;
    pos += snprintf(buf + pos, sizeof(buf) - pos, "[");

    for (int i = 0; i < count; i++) {
        pos += snprintf(buf + pos, sizeof(buf) - pos,
            "%s{\"id\":\"%s\",\"name\":\"%s\",\"icon\":\"%s\","
            "\"port\":%d,\"running\":%s}",
            i > 0 ? "," : "",
            apps[i].id, apps[i].name, apps[i].icon,
            apps[i].port, apps[i].running ? "true" : "false");
    }

    pos += snprintf(buf + pos, sizeof(buf) - pos, "]");

    return send_json(conn, MHD_HTTP_OK, buf);
}

/* ── POST /apps/{id}/start|stop ───────────────────────────────────────────── */

static enum MHD_Result handle_action(struct MHD_Connection *conn,
                                      const char *app_id, int start)
{
    /* Verify app exists */
    app_t apps[MAX_APPS];
    int   count = apps_scan(apps);
    int   found = 0;

    for (int i = 0; i < count; i++) {
        if (strcmp(apps[i].id, app_id) == 0) { found = 1; break; }
    }

    if (!found)
        return send_json(conn, MHD_HTTP_NOT_FOUND,
                         "{\"error\":\"app not found\"}");

    char unit[MAX_STR + 16];
    snprintf(unit, sizeof(unit), "crimata-%s.service", app_id);

    int r = start ? systemd_start(unit) : systemd_stop(unit);

    if (r < 0)
        return send_json(conn, MHD_HTTP_INTERNAL_SERVER_ERROR,
                         "{\"error\":\"systemd call failed\"}");

    return send_json(conn, MHD_HTTP_OK, "{\"ok\":true}");
}

/* ── Main request handler ─────────────────────────────────────────────────── */

static enum MHD_Result handler(void *cls,
                                struct MHD_Connection *conn,
                                const char *url,
                                const char *method,
                                const char *version,
                                const char *upload_data,
                                size_t *upload_data_size,
                                void **con_cls)
{
    (void)cls; (void)version; (void)upload_data; (void)upload_data_size;
    (void)con_cls;

    if (strcmp(url, "/health") == 0 && strcmp(method, "GET") == 0)
        return send_json(conn, MHD_HTTP_OK, "{\"status\":\"ok\"}");

    if (strcmp(url, "/apps") == 0 && strcmp(method, "GET") == 0)
        return handle_list(conn);

    char app_id[MAX_STR];

    if (strcmp(method, "POST") == 0) {
        if (sscanf(url, "/apps/%127[^/]/start", app_id) == 1)
            return handle_action(conn, app_id, 1);

        if (sscanf(url, "/apps/%127[^/]/stop", app_id) == 1)
            return handle_action(conn, app_id, 0);
    }

    return send_json(conn, MHD_HTTP_NOT_FOUND, "{\"error\":\"not found\"}");
}

/* ── Entry point ──────────────────────────────────────────────────────────── */

int main(void)
{
    struct MHD_Daemon *daemon = MHD_start_daemon(
        MHD_USE_INTERNAL_POLLING_THREAD,
        PORT, NULL, NULL,
        &handler, NULL,
        MHD_OPTION_END
    );

    if (!daemon) {
        fprintf(stderr, "failed to start daemon on port %d\n", PORT);
        return 1;
    }

    printf("crimata-dock listening on :%d\n", PORT);
    getchar();

    MHD_stop_daemon(daemon);
    return 0;
}
