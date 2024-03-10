#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#ifdef __APPLE__
#include <sys/syslimits.h>
#include <unistd.h>
#define __HWSH_PATH_MAX_LENGTH PATH_MAX
#define __DIR_SEPARATOR '/'
#endif

#if defined __linux__ && !defined __ANDROID__
#include <linux/limits.h>
#include <unistd.h>
#define __HWSH_PATH_MAX_LENGTH PATH_MAX
#define __DIR_SEPARATOR '/'
#endif

#if defined _WIN32 || defined _WIN64
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Advapi32.lib")
#include <Lmcons.h>
#include <winsock2.h>
#include <windows.h>
#include <winbase.h>
#include <io.h>
#include <process.h>
#define execvp _execvp
#define chdir _chdir
#define isatty _isatty
#define fileno _fileno
#define pipe _pipe
#define STDIN_FILENO 0 // oh my god
#define __HWSH_PATH_MAX_LENGTH MAX_PATH
#define __DIR_SEPARATOR '\\'
#endif

#define __HWSH_CMDLINE_MAX_LENGTH   250
#define __HWSH_ARBITRARY_MAX_LENGTH 127

#define ever (;;)

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#define ANSI_COLOR_RED_BOLD     "\x1b[1;31m"
#define ANSI_COLOR_GREEN_BOLD   "\x1b[1;32m"
#define ANSI_COLOR_YELLOW_BOLD  "\x1b[1;33m"
#define ANSI_COLOR_BLUE_BOLD    "\x1b[1;34m"
#define ANSI_COLOR_MAGENTA_BOLD "\x1b[1;35m"
#define ANSI_COLOR_CYAN_BOLD    "\x1b[1;36m"
#define ANSI_COLOR_RESET_BOLD   "\x1b[1;0m"

#define LOG_REG  1
#define LOG_ERR  2
#define LOG_WARN 3
#define LOG_INFO 4
#define LOG_HWSH 5

int main_interactive(int argc, char *argv[], char line[], char *command);
int main_batch(int argc, char *argv[], char line[], char *command, FILE *batch_file);
int hwsh_exec(char *command);
void hwsh_command_chdir(char *path);
void hwsh_command_history(char **history, int history_count);
char *hwsh_util_get_username(void);
char *hwsh_util_get_hostname(void);
void hwsh_util_str_trim(char *str);
void hwsh_util_str_only_one_space(char *str);
void hwsh_cli_show_usage(void);
void hwsh_cli_show_options(void);
int logger(int logType, const char *format, ...);

int main(int argc, char *argv[])
{
    if (argc == 2 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "--h") == 0))
    {
        hwsh_cli_show_usage();
        return EXIT_SUCCESS;
    }
    else if (argc == 2 && (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "--ver") == 0 || strcmp(argv[1], "--v") == 0 || strcmp(argv[1], "-v") == 0))
    {
        logger(LOG_REG, "HW Shell Development Build\nCopyright (C) Meric Baglayan, 2024");
        return EXIT_SUCCESS;
    }
    else if (argc == 2 && strncmp(argv[1], "--", (size_t)2) == 0)
    {
        logger(LOG_HWSH, "unknown option: %s", argv[1]);
        hwsh_cli_show_usage();
        hwsh_cli_show_options();
        return EXIT_FAILURE;
    }

    char line[__HWSH_CMDLINE_MAX_LENGTH];
    char *command = NULL;
    FILE *batch_file = NULL;

    if (argc == 1)
    {
        main_interactive(argc, argv, line, command);
        return EXIT_SUCCESS;
    }
    else
    {
        batch_file = fopen(argv[1], "r");
        if (batch_file == NULL)
        {
            logger(LOG_ERR, "unable to open file %s", argv[1]);
            return EXIT_FAILURE;
        }

        main_batch(argc, argv, line, command, batch_file);

        fclose(batch_file);
        return EXIT_SUCCESS;
    }

    return EXIT_SUCCESS;
}

int main_interactive(int argc, char *argv[], char line[], char *command)
{
    logger(LOG_INFO, "Interactive mode");

    char cwd[__HWSH_PATH_MAX_LENGTH];
    getcwd(cwd, sizeof(cwd));

    char *username = hwsh_util_get_username();
    char *hostname = hwsh_util_get_hostname();

    for ever
        {
            getcwd(cwd, sizeof(cwd));
            fprintf(stdout, "[%s@%s] %s $ ", username, hostname, cwd);
            fgets(line, __HWSH_CMDLINE_MAX_LENGTH, stdin);

            command = strtok(line, "\n");
            hwsh_exec(command);
        }

    free(username);
    free(hostname);

    return EXIT_SUCCESS;
}

int main_batch(int argc, char *argv[], char line[], char *command, FILE *batch_file)
{
    logger(LOG_INFO, "Batch mode");

    fgets(line, __HWSH_CMDLINE_MAX_LENGTH, batch_file);
    command = strtok(line, "\n");
    if (strncmp(command, "#!", (size_t)2) == 0)
    {
        if (strstr(command, "hwsh") != NULL)
        {
            logger(LOG_INFO, "Batch file designed for hwsh");
        }
        else
        {
            logger(LOG_WARN, "Batch file not designed for hwsh");
        }
    }
    else
    {
        hwsh_exec(command);
    }

    while (fgets(line, __HWSH_CMDLINE_MAX_LENGTH, batch_file) != NULL)
    {
        command = strtok(line, "\n");
        hwsh_exec(command);
    }

    return EXIT_SUCCESS;
}

/**
 * This function is a total mess.
 * Probably needs a rewrite from scratch.
 * Hopefully with a clear mind.
*/
int hwsh_exec(char *command)
{
    logger(LOG_INFO, "executing %s", command);

    /* tokenizer logic */

    // command is one line
    //      example: `cat f1.txt ; ls f2.txt | grep hw ; grep odev`

    // result of all commands before the | should be
    //      passed as args to all commands after the |

    char *pipe_save, *parallel_save;
    for (char *cluster = strtok_r(command, "|", &pipe_save);
         cluster;
         cluster = strtok_r(NULL, "|", &pipe_save))
    {
        hwsh_util_str_trim(cluster);

        logger(LOG_REG, "[PIPEE CLUSTER]: [BEGIN]%s[END]", cluster);
        logger(LOG_INFO, "==PIPE==");
        for (char *parallel_cmd = strtok_r(cluster, ";", &parallel_save);
             parallel_cmd;
             parallel_cmd = strtok_r(NULL, ";", &parallel_save))
        {
            hwsh_util_str_trim(parallel_cmd);
            hwsh_util_str_only_one_space(parallel_cmd);

            logger(LOG_REG, "[PARALLEL CMD]: [BEGIN]%s[END]", parallel_cmd);

            char *parallel_cmd_argv[__HWSH_ARBITRARY_MAX_LENGTH];
            int parallel_cmd_argc = 0;

            char *parallel_cmd_arg_save;

            for (char *parallel_cmd_arg = strtok_r(parallel_cmd, " ", &parallel_cmd_arg_save);
                 parallel_cmd_arg;
                 parallel_cmd_arg = strtok_r(NULL, " ", &parallel_cmd_arg_save))
            {
                parallel_cmd_argv[parallel_cmd_argc++] = parallel_cmd_arg;
            }
            parallel_cmd_argv[parallel_cmd_argc] = NULL;

            if (strcmp(parallel_cmd_argv[0], "quit") == 0)
            {
                exit(0);
            }
            else if ((strcmp(parallel_cmd_argv[0], "cd") == 0) || (strcmp(parallel_cmd_argv[0], "chdir") == 0))
            {
                if (parallel_cmd_argv[1] == NULL)
                {
                    hwsh_command_chdir(getenv("HOME"));
                }
                else
                {
                    hwsh_command_chdir(parallel_cmd_argv[1]);
                }
            }
            else if (strcmp(parallel_cmd_argv[0], "history") == 0)
            {
                /* call hwsh_command_history with appropriate parameters */

                logger(LOG_HWSH, "history not yet implemented");
            }
            else
            {
                /* code */
            }
            

            pid_t pid_parallel = fork();

            if (pid_parallel == 0)
            {
                execvp(parallel_cmd_argv[0], parallel_cmd_argv);
                exit(0);
            }
            else if (pid_parallel < 0)
            {
                logger(LOG_ERR, "fork failed");
            }
            else
            {
                wait(NULL);
            }

            logger(LOG_INFO, "==SEMICOLON==");
        }
    }

    return EXIT_SUCCESS;
}

void hwsh_command_chdir(char *path)
{
    if (chdir(path) != 0)
    {
        logger(LOG_ERR, "Error: Unable to change directory to %s", path);
    }
}

char *hwsh_util_get_username(void)
{
#if defined _WIN32 || defined _WIN64
    char *username = (char *)malloc(sizeof(char) * (UNLEN + 1));
    DWORD username_len = UNLEN + 1;
    GetUserName(username, &username_len);
    return username;
#elif defined __linux__ || __APPLE__
    char *_username = getlogin();
    char *username = (char *)malloc(sizeof(char) * __HWSH_ARBITRARY_MAX_LENGTH);
    strcpy(username, _username);
    return username;
#endif
}

char *hwsh_util_get_hostname(void)
{
#if defined _WIN32 || defined _WIN64
    char *hostname = (char *)malloc(sizeof(char) * (UNLEN + 1));
    DWORD hostname_len = UNLEN + 1;
    GetComputerName(hostname, &hostname_len);
    return hostname;
#elif defined __linux__ || __APPLE__
    char _hostname[__HWSH_PATH_MAX_LENGTH];
    gethostname(_hostname, sizeof(_hostname));
    char *hostname = (char *)malloc(sizeof(char) * __HWSH_ARBITRARY_MAX_LENGTH);
    strcpy(hostname, _hostname);
    return hostname;
#endif
}

void hwsh_util_str_trim(char *str)
{
    if (!str)
    {
        logger(LOG_ERR, "str_trim: str is NULL");
        exit(EXIT_FAILURE);
    }

    const char *firstNonSpace = str;

    while (isspace((unsigned char)*firstNonSpace))
    {
        ++firstNonSpace;
    }

    size_t len = strlen(firstNonSpace) + 1;
    memmove(str, firstNonSpace, len);

    char *endOfString = str + len - 1;

    while (str < endOfString && isspace((unsigned char)endOfString[-1]))
    {
        --endOfString;
    }

    *endOfString = '\0';
}

void hwsh_util_str_only_one_space(char *str)
{
    if (!str || !*str)
        return;

    char *writePtr = str;
    char *readPtr = str;

    int previousWasSpace = 0;
    while (*readPtr)
    {
        if (isspace((unsigned char)*readPtr))
        {
            if (!previousWasSpace)
            {
                *writePtr++ = ' ';
                previousWasSpace = 1;
            }
        }
        else
        {
            *writePtr++ = *readPtr;
            previousWasSpace = 0;
        }
        readPtr++;
    }
    *writePtr = '\0';
}

void hwsh_cli_show_usage(void)
{
    logger(LOG_REG, "Usage:  hwsh [option] ...\n        hwsh [option] script-file ...");
}

void hwsh_cli_show_options(void)
{
    logger(LOG_REG, "Shell options:\n        --help\n        --version");
}

int logger(int logType, const char *format, ...)
{
    if (logType < 0 || logType > 5)
    {
        logger(LOG_ERR, "function logger: wrong logType specified");
        return EXIT_FAILURE;
    }

    va_list args;
    va_start(args, format);

    switch (logType)
    {
    case STDIN_FILENO:
        logger(LOG_ERR, "function logger: cannot print to STDIN");
        return EXIT_FAILURE;
        break;
    case LOG_REG:
        vfprintf(stdout, format, args);
        fprintf(stdout, "\n");
        break;
    case LOG_INFO:
        fprintf(stdout, ANSI_COLOR_BLUE_BOLD "info: " ANSI_COLOR_RESET);
        vfprintf(stdout, format, args);
        fprintf(stdout, "\n");
        break;
    case LOG_WARN:
        fprintf(stdout, ANSI_COLOR_YELLOW_BOLD "warn: " ANSI_COLOR_RESET);
        vfprintf(stdout, format, args);
        fprintf(stdout, "\n");
        break;
    case LOG_ERR:
        fprintf(stdout, ANSI_COLOR_RED_BOLD "error: " ANSI_COLOR_RESET);
        vfprintf(stdout, format, args);
        fprintf(stdout, "\n");
        break;
    case LOG_HWSH:
        fprintf(stdout, ANSI_COLOR_MAGENTA_BOLD "hwsh: " ANSI_COLOR_RESET);
        vfprintf(stdout, format, args);
        fprintf(stdout, "\n");
        break;
    default:
        logger(LOG_ERR, "catastrophic failure");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
