/* ICS 53
 * Project: Simple Shell
 * Instructor: Ian Harris
 *
 * Student 1: Hongji Wu (ID: )
 * Student 2: Yi-Ju, Tsai (ID: 31208132)        
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdbool.h>


typedef struct job
{
    int jid; // Job ID
	pid_t pid; // Process ID
    char ground; // bg = 1, fg = 0 
    char status[11]; // Running, Foreground, Stopped
    char title[80]; // cmdline, ex: "hello &"
}Job;

int arg_count;
int remove_job(pid_t pid);
int add_job(pid_t pid, char ground, char **argv);
void unix_error(const char *msg);
int numOfJob;
struct job job_list[5];
bool stopped;
int child_Pid;

// parse the cmdline and build the argv list
int parseline(char *cpycmd, char **argv)
{
    cpycmd[strlen(cpycmd)-1] = ' '; 
    char *token = strtok(cpycmd, " ");
    while(token != NULL)
    {
        argv[arg_count++] = token;
        token = strtok(NULL," ");
    }
    argv[arg_count] = NULL;
    int bg = 0;
    if (arg_count==0)
    {
        return -1;
    }
    if (!strcmp(argv[arg_count-1],"&")) // bg process
    {
        bg = 1;
    }
    return bg; // return 1 if is bg process
}

void IOredirection(char **argv)
{
    int fd_out = dup(STDOUT_FILENO);
    int fd_in = dup(STDIN_FILENO);
    int i;
    for (i=0; i<arg_count; i++)
    {
        if (argv[i]==NULL)
        {
            break;
        }
        else if (strcmp(argv[i], "<") == 0)
        {
            mode_t mode = S_IRWXU | S_IRWXG | S_IRWXO; 
            int inFileID = open(argv[i + 1], O_RDONLY, mode);
            if(inFileID < 0)
            {
                printf("Input Error!!\n");
                exit(1); // end of the program
            }
            dup2(inFileID, STDIN_FILENO);
        }
        else if (strcmp(argv[i], ">") == 0) // output
        {
            mode_t mode = S_IRWXU | S_IRWXG | S_IRWXO;
            close(1);
            int outFileID = open(argv[i + 1], O_CREAT|O_WRONLY|O_TRUNC, mode);
            if(outFileID < 0)
            {
                printf("Output Error!!\n");
                exit(1); // end of the program
            }
        }
    }
}

void display_jobs(char **argv)
{
    int i;
    for(i=0; i<numOfJob; i++)
    {
        
        if (job_list[i].pid != 0)
        {
            printf("[%d] (%d) %s %s\n", job_list[i].jid, job_list[i].pid, job_list[i].status, job_list[i].title);
        }
    }
}


int builtin_command(char **argv)
{
    int pid = 0;
    if (strcmp(argv[0], "jobs")==0)
    {
        display_jobs(argv);
        return 1;
    }
    if (strcmp(argv[0], "fg")==0)
    {
        // Change a stopped or running background job to a running in the foreground.
        if (strncmp(argv[1], "%", 1)==0) // Job ID
        {
            int i;
            for(i=0; i<numOfJob; i++)
            {
                int jd = atoi(&argv[1][1]);
                if (job_list[i].pid != 0 && job_list[i].jid==jd)
                {
                    // foreground the background job which is already RUNNING
                    if (strcmp(job_list[i].status, "Running")==0)
                    {
                        pid = waitpid(-1, NULL, WUNTRACED);
                    }
                    else
                    {
                        // foregroung the background job which is STOPPED
                        strcpy(job_list[i].status, "Running");
                        kill(job_list[i].pid, SIGCONT);
                        pid = waitpid(-1, NULL, WUNTRACED);
                    }
                }
            }
        }
        else // Process ID
        {
            int i;
            for(i=0; i<numOfJob; i++)
            {
                int pd = atoi(argv[1]);
                if (job_list[i].pid != 0 && job_list[i].pid==pd)
                {
                    // foreground the background job which is already RUNNING
                    if (strcmp(job_list[i].status, "Running")==0)
                    {
                        pid = waitpid(-1, NULL, WUNTRACED);
                    }
                    else
                    {
                        // foregroung the background job which is STOPPED
                        strcpy(job_list[i].status, "Running");
                        kill(job_list[i].pid, SIGCONT);
                        pid = waitpid(-1, NULL, WUNTRACED);
                    }
                }
            }
        }
        int i;
        for (i=0; i<numOfJob; i++)
        {
            if (job_list[i].pid !=0 && job_list[i].pid==pid)
            {
                strcpy(job_list[i].status, "Stopped");
                job_list[i].ground = 'b';
            }
        }
        return 1;
    }
    if (strcmp(argv[0], "bg")==0)
    {
        if (strncmp(argv[1], "%", 1)==0) // Job ID
        {
            int i;
            for(i=0; i<numOfJob; i++)
            {
                int input_jid = atoi(&argv[1][1]);
                if (job_list[i].jid == input_jid) // argv[1]= "%1", 
                {
                    kill(job_list[i].pid, SIGCONT);
                    strcpy(job_list[i].status, "Running");
                }
            }
        }
        else
        {
            int i;
            for(i=0; i<numOfJob; i++)
            {
                int input_pid = atoi(argv[1]);
                if (job_list[i].pid == input_pid)
                {
                    kill(job_list[i].pid, SIGCONT);
                    strcpy(job_list[i].status, "Running");
                    return 1;
                }
            }
        }
        return 1;
    }
    if (strcmp(argv[0], "kill")==0)
    {
        if (strncmp(argv[1], "%", 1)==0) // Job ID
        {
            int i;
            for(i=0; i<numOfJob; i++)
            {
                int input_jid = atoi(&argv[1][1]);
                if (job_list[i].jid == input_jid) 
                {
                    if (strcmp(job_list[i].status, "Stopped")==0)
                    {
                        kill(job_list[i].pid, SIGCONT);
                        kill(job_list[i].pid, SIGINT);
                    }
                    else
                    {
                        kill(job_list[i].pid, SIGINT);
                    }
                    job_list[i].pid = 0;
                    job_list[i].ground = 'n';
                    strcpy(job_list[i].status, " ");
                    strcpy(job_list[i].title, " ");
                    return 1;
                }
            }
        }
        else // Process ID
        {
            int i;
            for(i=0; i<numOfJob; i++)
            {
                int input_pid = atoi(argv[1]);
                if (job_list[i].pid == input_pid)
                {
                    if (strcmp(job_list[i].status, "Stopped")==0)
                    {
                        kill(job_list[i].pid, SIGCONT);
                        kill(job_list[i].pid, SIGINT);
                    }
                    else
                    {
                        kill(job_list[i].pid, SIGINT);
                    }
                    job_list[i].pid = 0;
                    job_list[i].ground = 'n';
                    strcpy(job_list[i].status, " ");
                    strcpy(job_list[i].title, " ");;
                    return 1;
                }
            }
        }
        return 1;
    }
    if (strcmp(argv[0], "quit")==0)
    {
        int i;
        for(i =0; i<numOfJob; i++)
        {
            if(job_list[i].pid != 0 && job_list[i].ground== 'b')
            {
                pid_t pid = job_list[i].pid;
                kill(pid, SIGKILL);
                job_list[i].pid = 0;
                job_list[i].ground = 'n';
                strcpy(job_list[i].status, " ");
                strcpy(job_list[i].title, " ");
            }
        }
        exit(0);
        return 1;
    }
    return 0;
}
 

void eval(char *cmdline)
{
    char cpycmd[80]; // line of command
    char *argv[80]; // split cmdline to argv list
    int bg;
    arg_count = 0;
    pid_t pid;

    strcpy(cpycmd, cmdline);

    bg = parseline(cpycmd, argv);
    if (!builtin_command(argv))
    {
        pid = fork();
        if(pid > 0)
        {
            if (!bg)
            {
                pid = waitpid(-1, NULL, WUNTRACED);
            }    
        }
        else if(pid == 0)// child runs user job 
        {  
            IOredirection(argv);
            unsigned i = 0;
            for (i = 0; i< arg_count; i++)
            {
                if (!strcmp(argv[i], ">") || !strcmp(argv[i], "<"))
                {
                    argv[i] = NULL;
                }
            }
            if (execvp(argv[0],argv)<0 && execv(argv[0],argv)<0)
            {
                printf("%s: Command not found.\n", argv[0]);
                exit(0);
            }
            wait(NULL);
        }

        
        if (!bg)
        {
            int status;
            add_job(pid, 'f', argv); // add fg job to job_list
            if (!stopped)
            {
                remove_job(pid); // remove the completed fg job
            }
            else
            {
                int i;
                for (i=0; i<numOfJob; i++)
                {
                    if (job_list[i].pid !=0 && job_list[i].pid==pid)
                    {
                        strcpy(job_list[i].status, "Stopped");
                        job_list[i].ground = 'b';
                    }
                }
            }
        }
        else // if job runs in background
        {
            add_job(pid, 'b', argv); // add bg job to job_list
            if (stopped)
            {
                int i;
                for (i=0; i<numOfJob; i++)
                {
                    if (job_list[i].pid !=0 && job_list[i].pid==pid)
                    {
                        strcpy(job_list[i].status, "Stopped");
                        job_list[i].ground = 'b';
                    }
                }
            }
        }
    }   
}


void unix_error(const char *msg)
{
    int errnum = errno;
    fprintf(stderr, "%s (%d: %s)\n", msg, errnum, strerror(errnum));
    exit(EXIT_FAILURE);
}

void int_handler(int sig)
{
    // printf("Process %d received INT signal %d\n", getpid(), sig);
}

void tstp_handler(int sig)
{
    stopped = true;
}

void child_handler(int sig)
{
    int real_pid = getpid();
    int pid = waitpid(-1,NULL,WNOHANG);
    if (pid>1)
    {
        int i;
        for(i=0; i<numOfJob; i++)
        {
            if(job_list[i].pid == pid)
            {
                job_list[i].pid = 0;
                job_list[i].ground = 'n';
                strcpy(job_list[i].status, " ");
                strcpy(job_list[i].title, " ");
            }
        }
    }
}

void term_handler(int sig)
{
    // printf("Process %d received TERMINATE signal %d\n", getpid(), sig);
}

void cont_handler(int sig)
{
    // printf("Process %d received CONTINUE signal %d\n", getpid(), sig);
}


int add_job(pid_t pid, char ground, char **argv)
{
    if(pid < 1)
    {
        return 0;
    }
    int i = 0;
    if(ground == 'f')
    {
        if(numOfJob == 0)
        {
            job_list[0].jid = i+1;
            job_list[0].pid = pid;
            job_list[0].ground = ground;
            strcpy(job_list[0].status,"Running");
            int k;
            for (k=0; k<arg_count; k++)
            {
                strcat(job_list[0].title, argv[k]);
                strcat(job_list[0].title, " ");
            }
            numOfJob += 1;
            return 1;
        }
        for(i=0; i<numOfJob+1; i++)
        { 
            if(job_list[i].pid ==0)
            {
                job_list[i].jid = i+1;
                job_list[i].pid = pid;
                job_list[i].ground = ground;
                strcpy(job_list[i].status,"Running");
                int k;
                for (k=0; k<arg_count; k++)
                {
                    strcat(job_list[i].title, argv[k]);
                    strcat(job_list[i].title, " ");
                }
                numOfJob += 1;
                return 1; // make the new job to be background
            }
        }
        
    }
    else
    {
        if(numOfJob == 0)
        {
            job_list[0].jid = i+1;
            job_list[0].pid = pid;
            job_list[0].ground = ground;
            strcpy(job_list[0].status,"Running");
            int k;
            for (k=0; k<arg_count; k++)
            {
                strcat(job_list[0].title, argv[k]);
                strcat(job_list[0].title, " ");
            }
            numOfJob += 1;
            return 1;
        }
        for(i=0; i<numOfJob+1; i++)
        { 
            if(job_list[i].pid ==0)
            {
                job_list[i].jid = i+1;
                job_list[i].pid = pid;
                job_list[i].ground = ground;
                strcpy(job_list[i].status,"Running");
                int k;
                for (k=0; k<arg_count; k++)
                {
                    strcat(job_list[i].title, argv[k]);
                    strcat(job_list[i].title, " ");
                }
                numOfJob += 1;
                return 1; // make the new job to be background
            }
        } 
    }
    return 0;
}


int remove_job(pid_t pid)
{
    if (pid<0) // non-exist job
    {
        return 0;
    }
    int i;
    for(i=0; i<numOfJob; i++)
    {
        if (job_list[i].pid==pid)
        {
            job_list[i].pid = 0;
            job_list[i].ground = 'n';
            strcpy(job_list[i].status, " ");
            strcpy(job_list[i].title, " ");
            return 1;
        }
    }
}

void create_job_list()
{
    int i;
    for (i=0; i<5; i++)
    {
        job_list[i].jid = 0;
        job_list[i].pid = 0;
        job_list[i].ground = 'n';
        strcpy(job_list[i].status, " ");
        strcpy(job_list[i].title, " "); 
    }
}

int main(int argc, char *argv[])
{
    char cmdline[80];
    create_job_list();
    signal(SIGINT, int_handler);
    signal(SIGTSTP, tstp_handler);
    signal(SIGCHLD, child_handler);
    signal(SIGTERM, term_handler);
    signal(SIGCONT, cont_handler);
    numOfJob = 0;
    while(1)
    {
        stopped = false;
        child_Pid = 0;
        printf("prompt> ");
        fgets(cmdline, 80, stdin);
        if (feof(stdin))
        {
            exit(0);
        } 
        eval(cmdline);
    }
    exit(0);
}