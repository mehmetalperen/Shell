#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <signal.h>
#include <fcntl.h>

#define MAX_INPUT_SIZE 1024
#define MAX_ARG_SIZE 100
#define MAX_JOB_AMOUNT 10
pid_t foreground_pid = -1; // ourshel has the value of -1. when we create forground jobs, we will assing their foreground_pid = pid

typedef struct
{
    int job_id;
    pid_t pid;
    char command_line[30];
    char status[10]; // Foreground | Background | Stopped
} Job;

Job jobs[MAX_JOB_AMOUNT];
int job_count = 0;

void ctrl_c_handler(int signum)
{
    if (foreground_pid != -1) // so we would not kill our shell
    {
        // printf("foreground_pid == %d\n=====================\n", foreground_pid);
        kill(foreground_pid, SIGINT); // terminates foreground job with that pid
        foreground_pid = -1;          // reset foreground_pid to -1
        // no need to call remove_job_by_pid bc sigchld_handler will do that
    }
}

void ctrl_z_handler(int signum)
{
    if (foreground_pid != -1) // so we would not stop our shell
    {
        kill(foreground_pid, SIGTSTP); // stops process and puts it in background. SIGSTOP not used here bc it stops our shell and we can't custimize it

        for (int i = 0; i < job_count; i++)
        {
            if (jobs[i].pid == foreground_pid)
            {
                strcpy(jobs[i].status, "Stopped");
                break;
            }
        }
    }
}
void remove_job_by_pid(pid_t pid)
{
    // printf("remove pid == %d\n=====================\n", pid);
    for (int i = 0; i < job_count; i++)
    {
        if (jobs[i].pid == pid)
        {
            for (int j = i; j < job_count - 1; j++)
            {
                jobs[j] = jobs[j + 1];
                // printf("pid == %d\n=====================\n", pid);
            }
            job_count--;
            break;
        }
    }
}

void sigchld_handler(int signum)
// handles SIGCHLD; the signal sent to the parent process whenever a child process terminates, stops, or continues
{
    int status; // exit status of the child process. waitpid will set it.
    pid_t pid;
    // printf("YOOOOOO\n");
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    // The WNOHANG flag tells waitpid not to block if there are no processes that have exited.
    // The loop continues as long as waitpid returns a positive PID, meaning there is a child process has changed state
    {
        // printf("sigchld_handler returned pid == %d\n", pid);
        for (int i = 0; i < job_count; i++)
        {
            if (jobs[i].pid == pid)
            {
                if (WIFSTOPPED(status))
                {
                    // printf("YO: %d", pid);
                    strcpy(jobs[i].status, "Stopped");
                }
                else
                {
                    // printf("sigchld_handler == %d\n=====================\n", pid);
                    remove_job_by_pid(pid);
                }
                break;
            }
        }
    }
}

void parse_input(char *input, char **command, char **args, int *background, char **outfile, char **infile)
{
    char *token = strtok(input, " \t\n");
    *command = token;

    int i = 0;
    args[i] = token;
    i++;

    *outfile = NULL; // Initialize to NULL for no redirection
    *infile = NULL;  // Initialize to NULL for no input redirection

    while (token != NULL)
    {
        token = strtok(NULL, " \t\n");
        if (token != NULL && strcmp(token, ">") == 0)
        {
            // Next token is the outfile
            token = strtok(NULL, " \t\n");
            if (token != NULL)
            {
                *outfile = token;
            }
            break; // No more args after redirection
        }
        else if (token != NULL && strcmp(token, "<") == 0)
        {
            // Next token is the infile
            token = strtok(NULL, " \t\n");
            if (token != NULL)
            {
                *infile = token;
            }
            break; // No more args after input redirection
        }
        else
        {
            args[i] = token;
            i++;
        }
    }

    if (i > 1 && strcmp(args[i - 2], "&") == 0)
    {
        *background = 1;
        args[i - 2] = NULL;
    }
    else
    {
        *background = 0;
    }
}

int main()
{
    char input[MAX_INPUT_SIZE];
    char *command = NULL;
    char *args[MAX_ARG_SIZE] = {NULL};
    int background;

    mode_t mode = S_IRWXU | S_IRWXG | S_IRWXO;
    signal(SIGCHLD, sigchld_handler); // custom signal handler for child termination
    signal(SIGINT, ctrl_c_handler);   // signal handler for child intruption aka ctrl-C
    signal(SIGTSTP, ctrl_z_handler);  // signal handler for child suspension aka ctrl-Z

    while (1)
    {
        printf("prompt > ");
        fgets(input, sizeof(input), stdin);

        char *outfile = NULL;
        char *infile = NULL;
        parse_input(input, &command, args, &background, &outfile, &infile);

        if (strcmp(command, "cd") == 0)
        {
            if (args[1] == NULL)
            {
                char *home = getenv("HOME");
                if (home)
                {
                    if (chdir(home) != 0) // change directory to HOME
                    {
                        perror("cd");
                    }
                }
            }
            else if (args[2] != NULL)
            {
                fprintf(stderr, "cd: wrong number of arguments\n");
            }
            else if (chdir(args[1]) != 0)
            {
                perror("cd");
            }
        }
        else if (strcmp(command, "pwd") == 0)
        {
            if (args[1] != NULL)
            {
                fprintf(stderr, "pwd: too many arguments\n");
            }
            else
            {
                char current_directory[MAX_INPUT_SIZE];
                if (getcwd(current_directory, sizeof(current_directory)) != NULL) // get_current_working_directory
                {
                    printf("%s\n", current_directory);
                }
                else
                {
                    perror("pwd");
                }
            }
        }
        else if (strcmp(command, "quit") == 0)
        {
            for (int i = 0; i < job_count; i++)
            {
                kill(jobs[i].pid, SIGTERM); // terminates all children so we don't have children running after we quit our shell
            }
            exit(0);
        }
        else if (strcmp(command, "jobs") == 0)
        {
            int original_stdout = dup(STDOUT_FILENO);
            if (outfile != NULL)
            {
                int fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, mode);
                if (fd < 0)
                {
                    perror("open");
                    exit(EXIT_FAILURE);
                }
                if (dup2(fd, STDOUT_FILENO) < 0)
                {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
                close(fd);
            }
            for (int i = 0; i < job_count; i++)
            {
                printf("[%d] (%d) %s %s\n", jobs[i].job_id, jobs[i].pid, jobs[i].status, jobs[i].command_line);
            }
            if (outfile != NULL)
            {
                dup2(original_stdout, STDOUT_FILENO);
                close(original_stdout);
            }
        }

        else if (strcmp(command, "fg") == 0)
        {
            char *arg1 = args[1];
            if (arg1[0] == '%') // fg by job_id
            {
                int job_id = atoi(arg1 + 1); // pointer arithmetic
                int found = 0;
                for (int i = 0; i < job_count; i++)
                {
                    if (jobs[i].job_id == job_id)
                    {
                        foreground_pid = jobs[i].pid;
                        kill(jobs[i].pid, SIGCONT); // continue job
                        int status;
                        waitpid(jobs[i].pid, &status, WUNTRACED);
                        if (WIFEXITED(status) || WIFSIGNALED(status)) // WIFEXITED = normal termination. WIFSIGNALED = termination by signal
                        {
                            remove_job_by_pid(jobs[i].pid);
                        }

                        foreground_pid = -1; // set foregound_pid to -1 (our shell)
                        found = 1;
                        break;
                    }
                }
                if (!found)
                {
                    printf("Job not found\n");
                }
            }
            else // fg by pid
            {
                pid_t pid = atoi(arg1);
                int found = 0;
                for (int i = 0; i < job_count; i++)
                {
                    if (jobs[i].pid == pid)
                    {
                        foreground_pid = jobs[i].pid;
                        kill(jobs[i].pid, SIGCONT);
                        waitpid(jobs[i].pid, NULL, WUNTRACED);
                        int status;
                        waitpid(pid, &status, WUNTRACED);
                        if (WIFEXITED(status) || WIFSIGNALED(status)) // WIFEXITED = normal termination. WIFSIGNALED = termination by signal
                        {
                            remove_job_by_pid(pid);
                        }
                        foreground_pid = -1;
                        found = 1;
                        break;
                    }
                }
                if (!found)
                {
                    printf("Job not found\n");
                }
            }
        }
        else if (strcmp(command, "bg") == 0)
        {
            char *arg1 = args[1];
            if (arg1[0] == '%') // bg by job_id
            {
                int job_id = atoi(arg1 + 1);
                int found = 0;
                for (int i = 0; i < job_count; i++)
                {
                    if (jobs[i].job_id == job_id && strcmp(jobs[i].status, "Stopped") == 0)
                    {
                        strcpy(jobs[i].status, "Background"); // change the status
                        kill(jobs[i].pid, SIGCONT);           // continue job. no need to wait bc it's a background job
                        found = 1;
                        break;
                    }
                }
                if (!found)
                {
                    printf("Job not found or not stopped\n");
                }
            }
            else // bg by pid
            {
                pid_t pid = atoi(arg1);
                int found = 0;
                for (int i = 0; i < job_count; i++)
                {
                    if (jobs[i].pid == pid && strcmp(jobs[i].status, "Stopped") == 0)
                    {
                        strcpy(jobs[i].status, "Background");
                        kill(jobs[i].pid, SIGCONT);
                        found = 1;
                        break;
                    }
                }
                if (!found)
                {
                    printf("Process not found or not stopped\n");
                }
            }
        }
        else if (strcmp(command, "kill") == 0)
        {
            char *arg1 = args[1];
            if (arg1 != NULL)
            {
                if (arg1[0] == '%') // kill by job_id
                {
                    int job_id = atoi(arg1 + 1); // pointer arithmetic
                    int found = 0;
                    for (int i = 0; i < job_count; i++)
                    {
                        if (jobs[i].job_id == job_id)
                        {
                            if (strcmp(jobs[i].status, "Stopped") == 0)
                            {
                                strcpy(jobs[i].status, "Background");
                                kill(jobs[i].pid, SIGCONT);
                            }
                            kill(jobs[i].pid, SIGINT);
                            // printf("kill: %d", job_id);
                            found = 1;
                            break;
                        }
                    }
                    if (!found)
                    {
                        printf("Job not found\n");
                    }
                }
                else // kill by pid
                {
                    pid_t pid = atoi(arg1);
                    int found = 0;
                    for (int i = 0; i < job_count; i++)
                    {
                        if (jobs[i].pid == pid)
                        {
                            if (strcmp(jobs[i].status, "Stopped") == 0)
                            {
                                strcpy(jobs[i].status, "Background");
                                kill(jobs[i].pid, SIGCONT);
                            }
                            kill(jobs[i].pid, SIGINT);
                            // printf("kill: %d", pid);
                            found = 1;
                            break;
                        }
                    }
                    if (!found)
                    {
                        printf("Job not found\n");
                    }
                }
            }
            else
            {
                printf("Invalid kill command format\n");
            }
        }
        else
        {
            pid_t pid = fork();
            if (pid == 0) // Child process
            {
                if (background)
                {
                    setpgid(0, 0); // Put the process in a new process group
                }
                int original_stdout = dup(STDOUT_FILENO);
                if (outfile != NULL)
                {
                    int fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, mode);
                    if (fd < 0)
                    {
                        perror("open");
                        exit(EXIT_FAILURE);
                    }
                    if (dup2(fd, STDOUT_FILENO) < 0)
                    {
                        perror("dup2");
                        exit(EXIT_FAILURE);
                    }
                    close(fd);
                }

                int original_stdin = dup(STDIN_FILENO);
                if (infile != NULL)
                {
                    int fd = open(infile, O_RDONLY);
                    if (fd < 0)
                    {
                        perror("open");
                        exit(EXIT_FAILURE);
                    }
                    if (dup2(fd, STDIN_FILENO) < 0)
                    {
                        perror("dup2");
                        exit(EXIT_FAILURE);
                    }
                    close(fd);
                }
                execvp(command, args);
                execv(command, args);
                perror("execv/execvp");
                exit(EXIT_FAILURE); // Exit with failure if execv or execvp fails
            }
            else if (pid > 0) // Parent process
            {
                Job new_job;
                new_job.job_id = job_count + 1;
                new_job.pid = pid;
                strncpy(new_job.command_line, command, 29);
                new_job.command_line[29] = '\0';
                strcpy(new_job.status, background ? "Background" : "Foreground");
                jobs[job_count++] = new_job;

                if (!background)
                {
                    foreground_pid = pid;

                    int status;
                    waitpid(pid, &status, WUNTRACED);
                    if (WIFEXITED(status) || WIFSIGNALED(status)) // WIFEXITED = normal termination. WIFSIGNALED = termination by signal
                    {
                        remove_job_by_pid(pid);
                    }
                    foreground_pid = -1;
                }
            }
            else
            {
                perror("fork");
            }
        }
    }

    return 0;
}
