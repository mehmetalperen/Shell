#include<stdio.h>
#include<math.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<ctype.h>
#include<signal.h>
#include<fcntl.h>

#define MAXLINE 1024
#define MAXARGS 128

/* Function prototypes */
void eval(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv);
extern char **environ;


int main() {
    char cmdline[MAXLINE];

    while (1) {
        /* Read */
        if (feof(stdin))
            exit(0);
        fgets(cmdline, MAXLINE, stdin);
        printf("prompt > ");
        eval(cmdline);
    }
}

/* eval - Evaluate a command line */
void eval(char *cmdline) {
    char *argv[MAXARGS];
    char buf[MAXLINE];
    int bg;
    pid_t pid;

    strcpy(buf, cmdline);
    bg = parseline(buf, argv);

    if (argv[0] == NULL)
        return; /* Ignore empty lines */

    if (!builtin_command(argv)) {
        if ((pid = fork()) == 0) { /* Child runs user job */
            if (execvp(argv[0], argv) < 0) {
                printf("%s: Command not found.\n", argv[0]);
                exit(0);
            }
        }

        /* Parent waits for foreground job to terminate */
        if (!bg) {
            int status;
            if (waitpid(pid, &status, 0) < 0)
                perror("waitfg: waitpid error");
        } else {
            printf("%d %s", pid, cmdline);
        }
    }
}

/* If first arg is a builtin command, run it and return true */
int builtin_command(char **argv) {
    if (!strcmp(argv[0], "quit"))
        exit(0);
    if (!strcmp(argv[0], "&"))
        return 1;
    return 0;
}

int parseline(char *buf, char **argv) {
    char *delim;
    int argc;
    int bg;

    buf[strlen(buf)-1] = ' '; /* Replace trailing '\n' */
    while (*buf && (*buf == ' ')) /* Ignore leading spaces */
        buf++;
    argc = 0;
    while ((delim = strchr(buf, ' '))) {
        argv[argc++] = buf;
        *delim = '\0';
        buf = delim + 1;
        while (*buf && (*buf == ' ')) /* Ignore spaces */
            buf++;
    }
    argv[argc] = NULL;

    if (argc == 0)  /* Ignore blank line */
        return 1;

    /* Should the job run in the background? */
    if ((bg = (*argv[argc-1] == '&')) != 0)
        argv[--argc] = NULL;

    return bg;
}
