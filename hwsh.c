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

#ifdef __linux__
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
#define isatty(STDIN_FILENO) _isatty(_fileno(stdin))
#define STDIN_FILENO 0 //oh my god
#define __HWSH_PATH_MAX_LENGTH MAX_PATH
#define __DIR_SEPARATOR '\\'
#endif

#define __HWSH_CMDLINE_MAX_LENGTH 250

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

int main_interactive(int argc, char *argv[], char line[], char *commands);
int main_batch(int argc, char *argv[], char line[], char *commands, FILE *batch_file);
void hwsh_command_chdir(char *path);
void hwsh_command_history(char **history, int history_count);
char *hwsh_util_get_username(void);
char *hwsh_util_get_hostname(void);
int logger(int logType, const char *format, ...);

int main(int argc, char *argv[])
{
    if (argc == 2 && strcmp(argv[1], "--help") == 0)
    {
        logger(LOG_REG, "Usage: Something something");
        return EXIT_SUCCESS;
    }
    else if (argc == 2 && strcmp(argv[1], "--version") == 0)
    {
        logger(LOG_REG, "HW Shell Development Build\nCopyright (C) Meric Baglayan, 2024");
        return EXIT_SUCCESS;
    }
    /* Handle wrong arguments here */
    //
    // some func():
    //      printerr("you gave me wrong arguments!!!"")
    //
    //
    //

    char line[__HWSH_CMDLINE_MAX_LENGTH];
    char *commands = NULL;
    FILE *batch_file = NULL;

    if (isatty(STDIN_FILENO)) // doesn't work as I want it to
    {
        main_interactive(argc, argv, line, commands);
        return EXIT_SUCCESS;
    }
    else
    {
        main_batch(argc, argv, line, commands, batch_file);
        return EXIT_SUCCESS;
    }

    return EXIT_SUCCESS;
}

int main_interactive(int argc, char *argv[], char line[], char *commands)
{
    logger(LOG_INFO, "Interactive mode");

    char cwd[__HWSH_PATH_MAX_LENGTH];
    getcwd(cwd, sizeof(cwd));

    char *username = hwsh_util_get_username();
    char *hostname = hwsh_util_get_hostname();

    for ever
    {
        fprintf(stdout, "[%s@%s] $ ", username, hostname);
        fgets(line, __HWSH_CMDLINE_MAX_LENGTH, stdin);
    }

    return EXIT_SUCCESS;
}

int main_batch(int argc, char *argv[], char line[], char *commands, FILE *batch_file)
{
    logger(LOG_INFO, "Batch mode");
    return EXIT_SUCCESS;
}

void hwsh_command_chdir(char *path) {
    if (chdir(path) != 0) {
        logger(LOG_ERR, "Error: Unable to change directory to %s", path);
    }
}

char *hwsh_util_get_username(void) {
    #if defined _WIN32 || defined _WIN64
        char *username = (char *)malloc(sizeof(char) * (UNLEN + 1));
        DWORD username_len = UNLEN + 1;
        GetUserName(username, &username_len);
        return username;
    #elif defined __linux__ || __APPLE__
        char *_username = getlogin();
        char *username = (char *)malloc(sizeof(char) * strlen(_username));
        strcpy(username, _username);
        return username;
    #endif
}

char *hwsh_util_get_hostname(void) {
    #if defined _WIN32 || defined _WIN64
        char *hostname = (char *)malloc(sizeof(char) * (UNLEN + 1));
        DWORD hostname_len = UNLEN + 1;
        GetComputerName(hostname, &hostname_len);
        return hostname;
    #elif defined __linux__ || __APPLE__
        char _hostname[__HWSH_PATH_MAX_LENGTH];
        gethostname(_hostname, sizeof(_hostname));
        char *hostname = (char *)malloc(sizeof(char) * strlen(_hostname));
        strcpy(hostname, _hostname);
        return hostname;
    #endif
}

int logger(int logType, const char *format, ...)
{
    if (logType < 0 || logType > 4)
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
    }
    return EXIT_SUCCESS;
}
