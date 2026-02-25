#include <stdlib.h>
#include <string.h>
#include <security/pam_appl.h>
#include "pam_auth.h"

typedef struct {
    const char *password;
} pam_credentials_t;

static int pam_conversation(int num_msg, const struct pam_message **msg,
                            struct pam_response **resp, void *appdata_ptr)
{
    pam_credentials_t *creds = (pam_credentials_t *)appdata_ptr;

    *resp = calloc(num_msg, sizeof(struct pam_response));
    if (!*resp) return PAM_BUF_ERR;

    for (int i = 0; i < num_msg; i++) {
        if (msg[i]->msg_style == PAM_PROMPT_ECHO_OFF ||
            msg[i]->msg_style == PAM_PROMPT_ECHO_ON) {
            (*resp)[i].resp = strdup(creds->password);
            if (!(*resp)[i].resp) {
                free(*resp);
                return PAM_BUF_ERR;
            }
        }
    }

    return PAM_SUCCESS;
}

int authenticate(const char *username, const char *password)
{
    pam_credentials_t creds = { .password = password };
    struct pam_conv conv    = { pam_conversation, &creds };
    pam_handle_t *pamh      = NULL;
    int result;

    result = pam_start("login", username, &conv, &pamh);
    if (result != PAM_SUCCESS) goto done;

    result = pam_authenticate(pamh, PAM_SILENT);
    if (result != PAM_SUCCESS) goto done;

    result = pam_acct_mgmt(pamh, PAM_SILENT);

done:
    pam_end(pamh, result);
    return (result == PAM_SUCCESS) ? 0 : -1;
}
