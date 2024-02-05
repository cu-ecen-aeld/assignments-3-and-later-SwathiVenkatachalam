#include "systemcalls.h"
#include <sys/types.h> // To use data type pid_t
#include <unistd.h> //Fork, execv
#include <sys/wait.h>  // wait
#include <stdlib.h> //exit
#include <stdio.h> //printf
#include <fcntl.h> //file control

#define SUCCESS (0)
#define FAILURE (-1)
/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{

/*
 * TODO  add your code here
 *  Call the system() function with the command set in the cmd
 *   and return a boolean true if the system() call completed with success
 *   or false() if it returned a failure
*/
    if (system(cmd) != SUCCESS) //if system function failure, return false
    {
	    return false;
    }
    return true; //if system function success, return true
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    //command[count] = command[count];

/*
 * TODO:
 *   Execute a system command by calling fork, execv(),
 *   and wait instead of system (see LSP page 161).
 *   Use the command[0] as the full path to the command to execute
 *   (first argument to execv), and use the remaining arguments
 *   as second argument to the execv() command.
 *
*/
    va_end(args);

    pid_t pid = fork(); //process identification datatype; fork creates child process
    if(pid == FAILURE)
    {
	    printf("Error: Fork Failed! No child process created\n");
	    return false;
    }
    else if (pid == SUCCESS)
    {
	    printf("Child process\n");
	    // int execv(const char *pathname, char *const argv[]); //Ref: Linux Man page
	    int execv_retval = execv(command[0],command);
	    if(execv_retval == FAILURE) //returns only on err
	    {
		    printf("Error: Execv Failed!\n");
		    exit(EXIT_FAILURE);
	    }
    }
    else
    {
	    int wstatus;
	    printf("Parent Process\n");
	    //wait for child process to finish
	    pid = wait(&wstatus); //Ref: https://man7.org/linux/man-pages/man2/wait.2.html
	    if (pid == FAILURE || wstatus != 0)
	    {
		    printf("Error: Wait Failure!\n");
		    return false;
	    }
    }

    return true;
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    //command[count] = command[count];


/*
 * TODO
 *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a refernce,
 *   redirect standard out to a file specified by outputfile.
 *   The rest of the behaviour is same as do_exec()
 *
*/

    int fd = open(outputfile, O_WRONLY|O_TRUNC|O_CREAT, 0644); //Given stackoverflow ref
    if (fd == FAILURE)
    {
	    printf("Error: File Opening!\n");
	    exit(FAILURE);
    }

    pid_t pid = fork(); //process identification datatype; fork creates child process
    if(pid == FAILURE)
    {
            printf("Error: Fork Failed! No child process created\n");
            return false;
    }
    else if (pid == SUCCESS)
    {
            printf("Child process\n");
	    if (dup2(fd, 1) == FAILURE) //duplicate file descriptor fd to stdout 1; Given stackoverflow ref
	    { 
		    printf("Error: Duplicate file descriptor fd to stdout\n");
		    return false;
	    }
	    close(fd);

            // int execv(const char *pathname, char *const argv[]); //Ref: Linux Man page
            execv(command[0],command); //returns only on err
            printf("Error: Execv Failed!\n");
            return false;
    }
    else
    {
            int wstatus;
            printf("Parent Process\n");
            //wait for child process to finish
            pid = wait(&wstatus); //Ref: https://man7.org/linux/man-pages/man2/wait.2.html
            if (pid == FAILURE || wstatus != SUCCESS)
            {
                    printf("Error: Wait Failure!\n");
                    return false;
            }
    }
    va_end(args);

    return true;
}
