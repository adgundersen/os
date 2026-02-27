#include "user.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

/*
 * Run a command with two arguments, writing optional stdin data to it.
 * Returns the exit code, or -1 on fork/exec failure.
 */
static int run_cmd(const char *path, const char *const argv[],
                   const char *stdin_data)
{
    int pipefd[2] = {-1, -1};

    if (stdin_data) {
        if (pipe(pipefd) != 0) return -1;
    }

    pid_t pid = fork();
    if (pid < 0) return -1;

    if (pid == 0) {
        /* child */
        if (stdin_data) {
            close(pipefd[1]);
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[0]);
        }
        execv(path, (char *const *)argv);
        _exit(127);
    }

    /* parent */
    if (stdin_data) {
        close(pipefd[0]);
        size_t len = strlen(stdin_data);
        write(pipefd[1], stdin_data, len);
        close(pipefd[1]);
    }

    int status;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    return -1;
}

int user_create(const char *username, const char *password)
{
    /* Validate: username must be non-empty and only contain safe chars */
    if (!username || !*username || !password || !*password) return -1;
    for (const char *p = username; *p; p++) {
        char c = *p;
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '_' || c == '-'))
            return -1;
    }

    /* useradd -m -s /bin/bash <username> */
    const char *useradd_argv[] = {
        "/usr/sbin/useradd", "-m", "-s", "/bin/bash", username, NULL
    };
    int rc = run_cmd("/usr/sbin/useradd", useradd_argv, NULL);
    if (rc != 0) return -1;

    /* chpasswd reads "username:password\n" from stdin */
    char chpasswd_input[512];
    snprintf(chpasswd_input, sizeof(chpasswd_input), "%s:%s\n", username, password);

    const char *chpasswd_argv[] = { "/usr/sbin/chpasswd", NULL };
    rc = run_cmd("/usr/sbin/chpasswd", chpasswd_argv, chpasswd_input);

    /* Zero out the password from the stack before returning */
    memset(chpasswd_input, 0, sizeof(chpasswd_input));

    return (rc == 0) ? 0 : -1;
}
