// clang-format off
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/wait.h>
#include <pwd.h>
#include <fcntl.h>
#include <errno.h>

#ifdef __APPLE__
#include <sys/syslimits.h>
#include <unistd.h>
#define HWSH_PATH_MAX_LENGTH PATH_MAX
#define HWSH_DIR_SEPARATOR '/'
#endif

#ifdef __linux__
#include <linux/limits.h>
#include <unistd.h>
#define HWSH_PATH_MAX_LENGTH PATH_MAX
#define HWSH_DIR_SEPARATOR '/'
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
#define HWSH_PATH_MAX_LENGTH MAX_PATH
#define HWSH_DIR_SEPARATOR '\\'
#endif

#define HWSH_CMDLINE_MAX_LENGTH   250

#define ever (;;)

#define OUTPUT 0
#define INPUT 1

#define V_REG  1
#define V_INFO 1
#define V_WARN 1
#define V_ERR  1
#define V_LEX  0

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
#define LOG_LEX  6

typedef struct {
    char **args;
    int argc;
} parallel_cmd_t;

typedef struct {
    parallel_cmd_t **parallel_cmds;
    int num_parallel_cmds;
} pipe_cluster_t;

// clang-format on

int main_interactive(int argc, char *argv[], char line[], char *command);
int main_batch(int argc, char *argv[], char line[], char *command,
               FILE *batch_file);
int hwsh_exec(char *command);
int hwsh_builtin_command(char *command, char *first_arg);
void hwsh_command_chdir(char *path);
void hwsh_command_history(char **history, int history_count);
char *hwsh_util_get_username(void);
char *hwsh_util_get_hostname(void);
void hwsh_util_str_trim(char *str);
void hwsh_util_str_only_one_space(char *str);
void hwsh_cli_show_usage(void);
void hwsh_cli_show_options(void);
int logger(int logType, const char *format, ...);

int main(int argc, char *argv[]) {
  if (argc == 2 &&
      (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "--h") == 0)) {
    hwsh_cli_show_usage();
    return EXIT_SUCCESS;
  } else if (argc == 2 &&
             (strcmp(argv[1], "--version") == 0 ||
              strcmp(argv[1], "--ver") == 0 || strcmp(argv[1], "--v") == 0 ||
              strcmp(argv[1], "-v") == 0)) {
    logger(LOG_REG,
           "HW Shell Development Build\nCopyright (C) Meric Baglayan, 2024");
    return EXIT_SUCCESS;
  } else if (argc == 2 && strncmp(argv[1], "--", (size_t)2) == 0) {
    logger(LOG_HWSH, "unknown option: %s", argv[1]);
    hwsh_cli_show_usage();
    hwsh_cli_show_options();
    return EXIT_FAILURE;
  }

  char line[HWSH_CMDLINE_MAX_LENGTH];
  char *command = NULL;
  FILE *batch_file = NULL;

  if (argc == 1) {
    main_interactive(argc, argv, line, command);
    return EXIT_SUCCESS;
  } else {
    batch_file = fopen(argv[1], "r");
    if (batch_file == NULL) {
      logger(LOG_HWSH, "unable to open file %s", argv[1]);
      return EXIT_FAILURE;
    }

    main_batch(argc, argv, line, command, batch_file);

    fclose(batch_file);
    return EXIT_SUCCESS;
  }

  return EXIT_SUCCESS;
}

int main_interactive(int argc, char *argv[], char line[], char *command) {
  logger(LOG_INFO, "Interactive mode");

  char cwd[HWSH_PATH_MAX_LENGTH];
  getcwd(cwd, sizeof(cwd));

  char *username = hwsh_util_get_username();
  char *hostname = hwsh_util_get_hostname();

    for
      ever {
        getcwd(cwd, sizeof(cwd));
        fprintf(stdout, "[%s@%s] %s $ ", username, hostname, cwd);
        fgets(line, HWSH_CMDLINE_MAX_LENGTH, stdin);

        command = strtok(line, "\n");
        if (command != NULL) {
          hwsh_exec(command);
        }
      }

    free(username);
    free(hostname);

    return EXIT_SUCCESS;
}

int main_batch(int argc, char *argv[], char line[], char *command,
               FILE *batch_file) {
    logger(LOG_INFO, "Batch mode");

    fgets(line, HWSH_CMDLINE_MAX_LENGTH, batch_file);
    command = strtok(line, "\n");
    if (strncmp(command, "#!", (size_t)2) == 0) {
      if (strstr(command, "hwsh") != NULL) {
        logger(LOG_INFO, "Batch file designed for hwsh");
      } else {
        logger(LOG_WARN, "Batch file not designed for hwsh");
      }
    } else {
      hwsh_exec(command);
    }

    while (fgets(line, HWSH_CMDLINE_MAX_LENGTH, batch_file) != NULL) {
      command = strtok(line, "\n");
      hwsh_exec(command);
    }

    return EXIT_SUCCESS;
}

int hwsh_exec(char *command) {
    logger(LOG_INFO, "executing %s", command);

    char *pipe_save = NULL;
    char *cluster = strtok_r(command, "|", &pipe_save);

    pipe_cluster_t **clusters = NULL;
    int num_clusters = 0;

    while (cluster) {
        hwsh_util_str_trim(cluster);
        logger(LOG_LEX, "[PIPE CLUSTER]: [BEGIN]%s[END]", cluster);

        char *parallel_save = NULL;
        char *parallel_cmd = strtok_r(cluster, ";", &parallel_save);

        pipe_cluster_t *new_cluster = malloc(sizeof(pipe_cluster_t));
        new_cluster->num_parallel_cmds = 0;
        new_cluster->parallel_cmds = NULL;

        while (parallel_cmd) {
            hwsh_util_str_trim(parallel_cmd);
            hwsh_util_str_only_one_space(parallel_cmd);
            logger(LOG_LEX, "[PARALLEL CMD]: [BEGIN]%s[END]", parallel_cmd);

            parallel_cmd_t *new_parallel_cmd = malloc(sizeof(parallel_cmd_t));
            new_parallel_cmd->argc = 0;
            new_parallel_cmd->args = NULL;

            char *arg_save = NULL;
            char *arg = strtok_r(parallel_cmd, " ", &arg_save);

            while (arg) {
                new_parallel_cmd->args = realloc(new_parallel_cmd->args, (new_parallel_cmd->argc + 1) * sizeof(char *));
                new_parallel_cmd->args[new_parallel_cmd->argc++] = strdup(arg);
                arg = strtok_r(NULL, " ", &arg_save);
            }

            new_parallel_cmd->args = realloc(new_parallel_cmd->args, (new_parallel_cmd->argc + 1) * sizeof(char *));
            new_parallel_cmd->args[new_parallel_cmd->argc] = NULL;

            new_cluster->parallel_cmds = realloc(new_cluster->parallel_cmds, (new_cluster->num_parallel_cmds + 1) * sizeof(parallel_cmd_t *));
            new_cluster->parallel_cmds[new_cluster->num_parallel_cmds++] = new_parallel_cmd;

            parallel_cmd = strtok_r(NULL, ";", &parallel_save);
            logger(LOG_LEX, "==SEMICOLON==");
        }

        clusters = realloc(clusters, (num_clusters + 1) * sizeof(pipe_cluster_t *));
        clusters[num_clusters++] = new_cluster;

        cluster = strtok_r(NULL, "|", &pipe_save);
        logger(LOG_LEX, "==PIPE==");
    }

    int pipefd[num_clusters - 1][2];
    int prev_pipefd[2] = {-1, -1};

    for (int i = 0; i < num_clusters; i++) {
        pipe_cluster_t *cluster = clusters[i];

        if (i != num_clusters - 1) { // if not the last cluster, create a pipe for each command
            for (int j = 0; j < cluster->num_parallel_cmds; j++) {
                if (pipe(pipefd[i]) == -1) {
                    perror("pipe");
                    exit(EXIT_FAILURE);
                }
            }
        }

        int num_pids = 0;
        pid_t child_pids[cluster->num_parallel_cmds];

        for (int j = 0; j < cluster->num_parallel_cmds; j++) {
            parallel_cmd_t *parallel_cmd = cluster->parallel_cmds[j];

            int builtin_result = hwsh_builtin_command(parallel_cmd->args[0], parallel_cmd->args[1]);

            if (builtin_result != 0) {
                pid_t pid_parallel = fork();
                child_pids[num_pids++] = pid_parallel;

                if (pid_parallel == 0) {
                    if (prev_pipefd[0] != -1) { // if there is a previous pipe, read from it
                        if (dup2(prev_pipefd[0], STDIN_FILENO) == -1) {
                            perror("dup2");
                            exit(EXIT_FAILURE);
                        }
                    }

                    if (i != num_clusters - 1) { // if not the last cluster, write to the pipe
                        if (dup2(pipefd[i][1], STDOUT_FILENO) == -1) {
                            perror("dup2");
                            exit(EXIT_FAILURE);
                        }
                    }

                    execvp(parallel_cmd->args[0], parallel_cmd->args);
                    perror("execvp");
                    exit(EXIT_FAILURE);
                } else if (pid_parallel < 0) {
                    perror("fork");
                    exit(EXIT_FAILURE);
                }
            }
        }

        if (prev_pipefd[0] != -1) { // close the previous pipe if it exists
            close(prev_pipefd[0]);
        }

        if (i != num_clusters - 1) { // if not the last cluster, close the write end of the pipe
            for (int j = 0; j < cluster->num_parallel_cmds; j++) {
                close(pipefd[i][1]);
            }
        }

        prev_pipefd[0] = pipefd[i][0]; // save the read end of the pipe for the next cluster

        for (int k = 0; k < num_pids; k++) {
            int status;
            waitpid(child_pids[k], &status, 0);
        }
    }

    for (int i = 0; i < num_clusters; i++) {
        pipe_cluster_t *cluster = clusters[i];

        for (int j = 0; j < cluster->num_parallel_cmds; j++) {
            parallel_cmd_t *parallel_cmd = cluster->parallel_cmds[j];

            for (int k = 0; k < parallel_cmd->argc; k++) {
                free(parallel_cmd->args[k]);
            }

            free(parallel_cmd->args);
            free(parallel_cmd);
        }

        free(cluster->parallel_cmds);
        free(cluster);
    }

    free(clusters);

    return EXIT_SUCCESS;
}

int hwsh_builtin_command(char *command, char *firstArg) {
    if (strcmp(command, "quit") == 0) {
      exit(0);
    } else if ((strcmp(command, "cd") == 0) ||
               (strcmp(command, "chdir") == 0)) {
      if (firstArg == NULL) {
        hwsh_command_chdir(getenv("HOME"));
      } else {
        hwsh_command_chdir(firstArg);
      }
      return EXIT_SUCCESS;
    } else if (strcmp(command, "history") == 0) {
      /* call hwsh_command_history with appropriate parameters */

      logger(LOG_HWSH, "history not yet implemented");

      return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}

void hwsh_command_chdir(char *path) {
    if (chdir(path) != 0) {
      logger(LOG_HWSH, "unable to change directory to %s", path);
    }
}

char *hwsh_util_get_username(void) {
#if defined _WIN32 || defined _WIN64
    char *username = (char *)malloc(sizeof(char) * (UNLEN + 1));
    DWORD username_len = UNLEN + 1;
    GetUserName(username, &username_len);
    return username;
#elif defined __linux__ || __APPLE__
    struct passwd *_username = getpwuid(getuid());
    size_t username_len = strlen(_username->pw_name);
    char *username = (char *)malloc(sizeof(char) * (username_len + 1));
    strcpy(username, _username->pw_name);
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
    char _hostname[HWSH_PATH_MAX_LENGTH];
    gethostname(_hostname, sizeof(_hostname));
    size_t hostname_length = strlen(_hostname) + 1;
    char *hostname = (char *)malloc(sizeof(char) * hostname_length);
    strcpy(hostname, _hostname);
    return hostname;
#endif
}

void hwsh_util_str_trim(char *str) {
    if (!str) {
      logger(LOG_ERR, "str_trim: str is NULL");
      exit(EXIT_FAILURE);
    }

    const char *firstNonSpace = str;

    while (isspace((unsigned char)*firstNonSpace)) {
      ++firstNonSpace;
    }

    size_t len = strlen(firstNonSpace) + 1;
    memmove(str, firstNonSpace, len);

    char *endOfString = str + len - 1;

    while (str < endOfString && isspace((unsigned char)endOfString[-1])) {
      --endOfString;
    }

    *endOfString = '\0';
}

void hwsh_util_str_only_one_space(char *str) {
    if (!str || !*str)
      return;

    char *writePtr = str;
    char *readPtr = str;

    int previousWasSpace = 0;
    while (*readPtr) {
      if (isspace((unsigned char)*readPtr)) {
        if (!previousWasSpace) {
          *writePtr++ = ' ';
          previousWasSpace = 1;
        }
      } else {
        *writePtr++ = *readPtr;
        previousWasSpace = 0;
      }
      readPtr++;
    }
    *writePtr = '\0';
}

void hwsh_cli_show_usage(void) {
    logger(LOG_REG,
           "Usage:  hwsh [option] ...\n        hwsh [option] script-file ...");
}

void hwsh_cli_show_options(void) {
    logger(LOG_REG, "Shell options:\n        --help\n        --version");
}

int logger(int logType, const char *format, ...) {
    if (logType < 0 || logType > 6) {
      logger(LOG_ERR, "function logger: wrong logType specified");
      return EXIT_FAILURE;
    }
        va_list args;
        va_start(args, format);

        char formatln[strlen(format) + 20];
        switch (logType) {
        case STDIN_FILENO:
        logger(LOG_ERR, "function logger: cannot print to STDIN");
        return EXIT_FAILURE;
        break;
        case LOG_REG:
        sprintf(formatln, ANSI_COLOR_RESET "%s\n", format);
        if (V_REG) vfprintf(stdout, formatln, args);
        break;
        case LOG_INFO:
        sprintf(formatln, ANSI_COLOR_BLUE_BOLD "info: " ANSI_COLOR_RESET "%s\n", format);
        if (V_INFO) vfprintf(stderr, formatln, args);
        break;
        case LOG_WARN:
        sprintf(formatln, ANSI_COLOR_YELLOW_BOLD "warn: " ANSI_COLOR_RESET "%s\n", format);
        if (V_WARN) vfprintf(stderr, formatln, args);
        break;
        case LOG_ERR:
        sprintf(formatln, ANSI_COLOR_RED_BOLD "error: " ANSI_COLOR_RESET "%s\n", format);
        if (V_ERR) vfprintf(stderr, formatln, args);
        break;
        case LOG_HWSH:
        sprintf(formatln, ANSI_COLOR_MAGENTA_BOLD "hwsh: " ANSI_COLOR_RESET "%s\n", format);
        vfprintf(stdout, formatln, args);
        break;
        case LOG_LEX:
        sprintf(formatln, ANSI_COLOR_MAGENTA_BOLD "lexer: " ANSI_COLOR_RESET "%s\n", format);
        if (V_LEX) vfprintf(stderr, formatln, args);
        break;
        default:
        logger(LOG_ERR, "catastrophic failure");
        return EXIT_FAILURE;
        }
    return EXIT_SUCCESS;
}
