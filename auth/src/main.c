#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <microhttpd.h>
#include "pam_auth.h"

#define PORT       7700
#define MAX_BODY   4096

/* ── JSON helpers ─────────────────────────────────────────────────────────── */

static const char *json_get(const char *json, const char *key, char *out, size_t out_len)
{
    char search[128];
    snprintf(search, sizeof(search), "\"%s\"", key);
    const char *pos = strstr(json, search);
    if (!pos) return NULL;

    pos += strlen(search);
    while (*pos == ' ' || *pos == ':' || *pos == ' ') pos++;
    if (*pos != '"') return NULL;
    pos++;

    size_t i = 0;
    while (*pos && *pos != '"' && i < out_len - 1)
        out[i++] = *pos++;
    out[i] = '\0';
    return out;
}

static enum MHD_Result send_json(struct MHD_Connection *conn,
                                  unsigned int status,
                                  const char *body)
{
    struct MHD_Response *resp = MHD_create_response_from_buffer(
        strlen(body), (void *)body, MHD_RESPMEM_MUST_COPY);
    MHD_add_response_header(resp, "Content-Type", "application/json");
    enum MHD_Result ret = MHD_queue_response(conn, status, resp);
    MHD_destroy_response(resp);
    return ret;
}

/* ── Request context ──────────────────────────────────────────────────────── */

typedef struct {
    char   body[MAX_BODY];
    size_t body_len;
} request_ctx_t;

/* ── Route handlers ───────────────────────────────────────────────────────── */

static enum MHD_Result handle_health(struct MHD_Connection *conn)
{
    return send_json(conn, MHD_HTTP_OK, "{\"status\":\"ok\"}");
}

static enum MHD_Result handle_auth(struct MHD_Connection *conn, const char *body)
{
    char username[256] = {0};
    char password[256] = {0};

    if (!json_get(body, "username", username, sizeof(username)) ||
        !json_get(body, "password", password, sizeof(password))) {
        return send_json(conn, MHD_HTTP_BAD_REQUEST,
                         "{\"success\":false,\"error\":\"username and password required\"}");
    }

    int result = authenticate(username, password);
    if (result == 0) {
        return send_json(conn, MHD_HTTP_OK, "{\"success\":true}");
    }
    return send_json(conn, MHD_HTTP_UNAUTHORIZED,
                     "{\"success\":false,\"error\":\"invalid credentials\"}");
}

/* ── Main handler ─────────────────────────────────────────────────────────── */

static enum MHD_Result handler(void *cls,
                                struct MHD_Connection *conn,
                                const char *url,
                                const char *method,
                                const char *version,
                                const char *upload_data,
                                size_t *upload_data_size,
                                void **con_cls)
{
    (void)cls; (void)version;

    /* Health check */
    if (strcmp(url, "/health") == 0 && strcmp(method, "GET") == 0)
        return handle_health(conn);

    /* Auth — collect body first */
    if (strcmp(url, "/auth") == 0 && strcmp(method, "POST") == 0) {
        if (!*con_cls) {
            request_ctx_t *ctx = calloc(1, sizeof(request_ctx_t));
            if (!ctx) return MHD_NO;
            *con_cls = ctx;
            return MHD_YES;
        }

        request_ctx_t *ctx = *con_cls;

        if (*upload_data_size > 0) {
            size_t remaining = MAX_BODY - ctx->body_len - 1;
            size_t to_copy   = *upload_data_size < remaining ? *upload_data_size : remaining;
            memcpy(ctx->body + ctx->body_len, upload_data, to_copy);
            ctx->body_len    += to_copy;
            *upload_data_size = 0;
            return MHD_YES;
        }

        return handle_auth(conn, ctx->body);
    }

    return send_json(conn, MHD_HTTP_NOT_FOUND, "{\"error\":\"not found\"}");
}

static void request_completed(void *cls, struct MHD_Connection *conn,
                               void **con_cls, enum MHD_RequestTerminationCode toe)
{
    (void)cls; (void)conn; (void)toe;
    if (*con_cls) { free(*con_cls); *con_cls = NULL; }
}

/* ── Entry point ──────────────────────────────────────────────────────────── */

int main(void)
{
    struct MHD_Daemon *daemon = MHD_start_daemon(
        MHD_USE_INTERNAL_POLLING_THREAD,
        PORT,
        NULL, NULL,
        &handler, NULL,
        MHD_OPTION_NOTIFY_COMPLETED, request_completed, NULL,
        MHD_OPTION_END
    );

    if (!daemon) {
        fprintf(stderr, "failed to start daemon on port %d\n", PORT);
        return 1;
    }

    printf("crimata-auth listening on :%d\n", PORT);
    getchar(); /* block until killed */

    MHD_stop_daemon(daemon);
    return 0;
}
