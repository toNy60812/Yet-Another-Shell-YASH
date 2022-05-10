#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <fcntl.h>

typedef enum status_t {Stopped, Running} status_t;
typedef struct job_t {
    char *jobName;
    int jobid;
    int child;
    pid_t pgid;
    pid_t pid1;
    pid_t pid2;
    status_t status;
    struct job_t *next;
} job_t;

char** redirect(char **parsedCmd, int start, int end);
extern void deleteJob(int jobid);

extern void action(int tokenNumber, char **parsedCmd, job_t *newNode, bool ampersand) {
    char *In1 = NULL, *Out1 = NULL, *Err1 = NULL;
    char *In2 = NULL, *Out2 = NULL, *Err2 = NULL;
    char **cmd1, **cmd2;
    bool pipeExisted = false; // 1 child
    int separate = tokenNumber;
    int pipefd[2], status;
    pid_t cpid;
    
    for (int i = 0; i < tokenNumber; i++) {
        if (!strcmp(parsedCmd[i], "|")) {
            pipeExisted = true; // pipe exists, 2 childs
            newNode -> child = newNode -> child + 1;
            separate = i;
            pipe(pipefd);
            break;
        }
    }

    cpid = fork();
    pid_t lpid = cpid; // record the pid (pgid) of left child
    newNode -> pgid = lpid;
    newNode -> pid1 = lpid;
    if (cpid == 0) {    // left child
        setpgid(0, 0); // create pg
        if (pipeExisted) {
            close(pipefd[0]); // Close unused read end
            dup2(pipefd[1], STDOUT_FILENO); // Make output go to pipe
        }
        cmd1 = redirect(parsedCmd, 0, separate);
        int validCmd = execvp(cmd1[0], cmd1); // NULL terminated
        if (validCmd == -1) {
            deleteJob(newNode -> jobid);
            kill(-getpgid(0), SIGKILL);
            return;
        }
    }
    
    if (pipeExisted) {
        cpid = fork();
        newNode -> pid2 = cpid;
        if (cpid == 0) {    // right child
            setpgid(0, lpid); // join left child's pg
            close(pipefd[1]); // Close unused write end
            dup2(pipefd[0], STDIN_FILENO); // Get input from pipe
            cmd2 = redirect(parsedCmd, separate + 1, tokenNumber);
            int validCmd = execvp(cmd2[0], cmd2); // NULL terminated
            if (validCmd == -1) {
                deleteJob(newNode -> jobid);
                kill(-getpgid(0), SIGKILL);
                return;
            }
        }
    }
    
    if (pipeExisted) {
        close(pipefd[0]);
        close(pipefd[1]);
        if (ampersand) {
            waitpid(-1, &status, WNOHANG);
        } else {
            waitpid(-1, &status, WUNTRACED); // first child
        }
    }
    
    if (ampersand) {
        waitpid(-1, &status, WNOHANG);
    } else {
        waitpid(-1, &status, WUNTRACED); // second child
        if (!WIFSTOPPED(status)) {
            deleteJob(newNode -> jobid);
        }
    }
}

char** redirect(char **parsedCmd, int start, int end) {
    char **cmd = (char**) calloc(15, sizeof(char*));
    int idx = 0;
    int fd;

    for (int i = start; i < end; i++) {
        if (!strcmp(parsedCmd[i], "<")) {
            fd = open(parsedCmd[++i], O_RDONLY);
            if (fd == -1) {
                printf("bash: %s: No such file or directory\n", parsedCmd[i]);
                exit(1);
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
        } else if (!strcmp(parsedCmd[i], ">")) {
            fd = creat(parsedCmd[++i], 0664);
            dup2(fd, STDOUT_FILENO);
            close(fd);
        } else if (!strcmp(parsedCmd[i], "2>")) {
            fd = creat(parsedCmd[++i], 0664);
            dup2(fd, STDERR_FILENO);
            close(fd);
        } else {
            cmd[idx++] = parsedCmd[i];
        }
    }
    cmd[idx] = NULL;
    return cmd;
}
