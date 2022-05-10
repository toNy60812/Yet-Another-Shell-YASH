#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdbool.h>

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

extern int parseString(char *str, char ***parsedStr); //parses an command into strings 
extern void action(int tokenNumber, char **parsedCmd, job_t *newNode, bool ampersand); // pipe and redirections
void sig_handler(int signo);
void newJob(job_t *newNode);
extern void deleteJob(int jobid);

job_t *root = NULL;
bool fg = false; // check if there exist a foreground process (used for ^C ^Z)

int main(){
    int cpid, tokenNumber, status;
    bool ampersand;
    char *inString, *originalCmd;
    char **parsedCmd;
    job_t *MostRecentJob;
    signal(SIGINT, sig_handler); // hijack ^C
    signal(SIGTSTP, sig_handler); // hijack ^Z
    signal(SIGTTOU, SIG_IGN); // ignore terminal output for background process if using tcsetpgrp()
    
    while (inString = readline("# ")) {
        // catch exited bg process
        int pid = waitpid(-1, &status, WNOHANG);
        if (pid > 0) {
            job_t *exitedNode = root;
            while ((exitedNode -> pid1 != pid) && (exitedNode -> pid2 != pid)) {
                exitedNode = exitedNode -> next;
            }
            exitedNode -> child = exitedNode -> child - 1;
            if (exitedNode -> child == 0) {
                if (exitedNode -> next == NULL) {
                    printf("[%d]+  Done\t\t%s\n", exitedNode -> jobid, exitedNode -> jobName);
                } else {
                    printf("[%d]-  Done\t\t%s\n", exitedNode -> jobid, exitedNode -> jobName);
                }
                deleteJob(exitedNode -> jobid);
            }
        }

        ampersand = false;
        if (strlen(inString) > 0) {
            originalCmd = strdup(inString);
            tokenNumber = parseString(inString, &parsedCmd);

            if (!strcmp(parsedCmd[tokenNumber - 1], "&")) {
                if (tokenNumber > 1) {
                    // valid & command
                    ampersand = true;
                    parsedCmd[--tokenNumber] = NULL;
                }
            }

            if (!strcmp(parsedCmd[0], "fg")) {
                // fg, no need fork
                if (!root) continue;
                MostRecentJob = root;
                while (MostRecentJob -> next) { // find the most recent job first
                    MostRecentJob = MostRecentJob -> next;
                }
                fg = true;
                kill(-(MostRecentJob -> pgid), SIGCONT);
                printf("%s\n", MostRecentJob -> jobName);
                MostRecentJob -> status = Running;
                // wait for fg job finish (blocking)
                // tcsetpgrp(0, MostRecentJob -> pgid);
                waitpid(-(MostRecentJob -> pgid), &status, WUNTRACED);
                // tcsetpgrp(0, getpid());
                fg = false;
                if (!WIFSTOPPED(status)) {
                    deleteJob(MostRecentJob -> jobid);
                }
            } else if (!strcmp(parsedCmd[0], "bg")) {
                // bg, no need fork
                if (!root) continue;
                MostRecentJob = root;
                while (MostRecentJob -> next) { // find the most recent job first
                    MostRecentJob = MostRecentJob -> next;
                }
                kill(-(MostRecentJob -> pgid), SIGCONT);
                printf("[%d]+ %s &\n", MostRecentJob -> jobid, MostRecentJob -> jobName);
                MostRecentJob -> status = Running;
                // no need to wait for bg job finish (non-blocking)
                waitpid(-(MostRecentJob -> pgid), &status, WNOHANG);
            } else if (!strcmp(parsedCmd[0], "jobs")) {
                // jobs, no need fork
                if (!root) continue;
                job_t *node = root;
                char *status_s;
                while (1) {
                    if (node -> status == Running) {
                        status_s = "Running";
                    } else if (node -> status == Stopped) {
                        status_s = "Stopped";
                    }

                    if (node -> next) {
                        printf("[%d]-  %s\t\t%s\n", node -> jobid, status_s, node -> jobName);
                        node = node -> next;
                    } else {
                        printf("[%d]+  %s\t\t%s\n", node -> jobid, status_s, node -> jobName);
                        break;
                    }
                }
            } else {
                // pipe, redirections, and other commands
                job_t *newNode = calloc(1, sizeof(job_t));
                newNode -> jobName = originalCmd;
                // find the biggest job id
                if (!root) {
                    newNode -> jobid = 1;
                } else {
                    job_t *lastnode = root;
                    while (lastnode -> next) {
                        lastnode = lastnode -> next;
                    }
                    newNode -> jobid = (lastnode -> jobid) + 1;
                }
                newNode -> child = 1;
                newNode -> status = Running;
                newNode -> next = NULL;
                newJob(newNode);
                if (!ampersand) {
                    fg = true;
                }
                action(tokenNumber, parsedCmd, newNode, ampersand);
                fg = false;
            }

            // free memory
            free(parsedCmd);
        }
    }
    return 0;
}

void sig_handler(int signo) {
    if (root && fg) { // fg: there exist a foreground process
        job_t *MostRecentJob = root;
        job_t *node = root;
        int status;
        while (MostRecentJob -> next) { // find the most recent job first
            MostRecentJob = MostRecentJob -> next;
        }
        switch(signo) {
            case SIGINT:
                kill(-(MostRecentJob -> pgid), SIGINT);
                waitpid(-(MostRecentJob -> pgid), &status, WNOHANG);
                fg = false;
                break;
            case SIGTSTP:
                MostRecentJob -> status = Stopped;
                kill(-(MostRecentJob -> pgid), SIGTSTP);
                waitpid(-(MostRecentJob -> pgid), &status, WNOHANG);
                fg = false;
                break;
        }
    }
}

void newJob(job_t *newNode) {
    if (root == NULL) {
        root = newNode;
    } else {
        job_t *node = root;
        while (node -> next) {
            node = node -> next;
        }
        node -> next = newNode;
    }
}

extern void deleteJob(int jobid) {
    job_t *delNode = root;
    job_t *prev = NULL;
    while (delNode && delNode -> jobid != jobid) {
        prev = delNode;
        delNode = delNode -> next;
    }
    if (!delNode) {
        // printf("jobid not exist\n");
        return;
    }
    
    if (prev) {
        prev -> next = delNode -> next;
        delNode -> next = NULL;
        free(delNode);
        
    } else {
        root = NULL;
        free(root);
    }
}
