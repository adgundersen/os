#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <microhttpd.h>
#include "pam_auth.h"
#include "session.h"
#include "user.h"

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
    while (*pos == ' ' || *pos == ':') pos++;
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
    MHD_add_response_header(resp, "Access-Control-Allow-Origin", "*");
    enum MHD_Result ret = MHD_queue_response(conn, status, resp);
    MHD_destroy_response(resp);
    return ret;
}

/* Extract Bearer token from Authorization header */
static const char *bearer_token(struct MHD_Connection *conn)
{
    const char *auth = MHD_lookup_connection_value(conn, MHD_HEADER_KIND, "Authorization");
    if (!auth) return NULL;
    if (strncmp(auth, "Bearer ", 7) != 0) return NULL;
    return auth + 7;
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

/* POST /auth  { username, password } → { token } */
static enum MHD_Result handle_auth(struct MHD_Connection *conn, const char *body)
{
    char username[256] = {0};
    char password[256] = {0};

    if (!json_get(body, "username", username, sizeof(username)) ||
        !json_get(body, "password", password, sizeof(password))) {
        return send_json(conn, MHD_HTTP_BAD_REQUEST,
                         "{\"success\":false,\"error\":\"username and password required\"}");
    }

    if (authenticate(username, password) != 0) {
        return send_json(conn, MHD_HTTP_UNAUTHORIZED,
                         "{\"success\":false,\"error\":\"invalid credentials\"}");
    }

    const char *token = session_create(username);
    if (!token) {
        return send_json(conn, MHD_HTTP_INTERNAL_SERVER_ERROR,
                         "{\"success\":false,\"error\":\"no session slots\"}");
    }

    char resp[320];
    snprintf(resp, sizeof(resp),
             "{\"success\":true,\"token\":\"%s\",\"username\":\"%s\"}", token, username);
    return send_json(conn, MHD_HTTP_OK, resp);
}

/* GET /me  Authorization: Bearer <token> → { username } */
static enum MHD_Result handle_me(struct MHD_Connection *conn)
{
    const char *token = bearer_token(conn);
    if (!token) {
        return send_json(conn, MHD_HTTP_UNAUTHORIZED,
                         "{\"error\":\"missing token\"}");
    }

    const char *username = session_lookup(token);
    if (!username) {
        return send_json(conn, MHD_HTTP_UNAUTHORIZED,
                         "{\"error\":\"invalid token\"}");
    }

    char resp[320];
    snprintf(resp, sizeof(resp), "{\"username\":\"%s\"}", username);
    return send_json(conn, MHD_HTTP_OK, resp);
}

/* POST /logout  Authorization: Bearer <token> */
static enum MHD_Result handle_logout(struct MHD_Connection *conn)
{
    const char *token = bearer_token(conn);
    if (token) session_destroy(token);
    return send_json(conn, MHD_HTTP_OK, "{\"success\":true}");
}

/* POST /users  { username, password } → create Linux user */
static enum MHD_Result handle_create_user(struct MHD_Connection *conn, const char *body)
{
    /* Caller must be authenticated */
    const char *token = bearer_token(conn);
    if (!token || !session_lookup(token)) {
        return send_json(conn, MHD_HTTP_UNAUTHORIZED,
                         "{\"success\":false,\"error\":\"authentication required\"}");
    }

    char username[256] = {0};
    char password[256] = {0};

    if (!json_get(body, "username", username, sizeof(username)) ||
        !json_get(body, "password", password, sizeof(password))) {
        return send_json(conn, MHD_HTTP_BAD_REQUEST,
                         "{\"success\":false,\"error\":\"username and password required\"}");
    }

    if (user_create(username, password) != 0) {
        return send_json(conn, MHD_HTTP_CONFLICT,
                         "{\"success\":false,\"error\":\"could not create user\"}");
    }

    char resp[320];
    snprintf(resp, sizeof(resp), "{\"success\":true,\"username\":\"%s\"}", username);
    return send_json(conn, MHD_HTTP_CREATED, resp);
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

    /* GET /me */
    if (strcmp(url, "/me") == 0 && strcmp(method, "GET") == 0)
        return handle_me(conn);

    /* Routes that need a request body */
    if ((strcmp(url, "/auth")    == 0 ||
         strcmp(url, "/logout")  == 0 ||
         strcmp(url, "/users")   == 0) &&
        (strcmp(method, "POST")  == 0))
    {
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

        if (strcmp(url, "/auth")   == 0) return handle_auth(conn, ctx->body);
        if (strcmp(url, "/logout") == 0) return handle_logout(conn);
        if (strcmp(url, "/users")  == 0) return handle_create_user(conn, ctx->body);
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
