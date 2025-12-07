#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdarg.h>
#include "systemcalls.h"

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
 */
bool do_system(const char *cmd)
{
    int result = system(cmd);

    if (result == 0)
    {
        return true;
    }
    else
    {
        return false;
    }
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
    // iterator for the list of arguments
    va_list args;
    // initialize the iterator from right after the argument `count`.
    va_start(args, count);
    char *command[count + 1];
    int i;
    for (i = 0; i < count; i++)
    {
        // grabs the current argument and moves the pointer to the next one.
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;

    pid_t pid = fork();

    if (pid == -1)
    {
        // Failed to create child process.
        return false;
    }
    else if (pid == 0)
    {
        // Inside child process
        execv(command[0], command);

        // If execv returns, it failed.
        exit(-1);
    }
    else
    {
        // Inside parent process.
        int status;
        waitpid(pid, &status, 0);

        // check if the child exited && the exit status is 0.
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    va_end(args);

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
    char *command[count + 1];
    int i;
    for (i = 0; i < count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;

    pid_t pid = fork();

    if (pid == -1)
    {
        return false;
    }
    else if (pid == 0)
    {
        // child process
        int fd = open(outputfile, O_WRONLY | O_TRUNC | O_CREAT, 0644);

        if (fd < 0)
        {
            perror("Failed to open output file");
            exit(-1);
        }

        if (dup2(fd, STDOUT_FILENO) < 0)
        {
            perror("Failed to redirect stdout to fd");
            close(fd);
            exit(-1);
        }

        close(fd);

        execv(command[0], command);

        perror("Execv failed");
        exit(-1);
    }
    else
    {
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
        {
            return true;
        }
        return false;
    }

    va_end(args);

    return true;
}
