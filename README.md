# hwsh

Created for BLG312E Computer Operating Systems Spring 2024 course, as the first homework

This assignment involves implementing a command-line shell that can be run in batch mode. The shell is initiated by specifying a batch file on its command line, which contains a list of commands to be executed. Each line in the batch file can contain multiple commands separated by `;` or `|`. Commands separated by `;` should be run simultaneously or concurrently, while commands separated by `|` should be executed with the output of the preceding commands piped as input to the subsequent commands.

## Features

- Execute multiple commands separated by `;` simultaneously
- Pipe the outputs of each command before `|` to each command after `|` (check REQUIREMENTS.md)
- Support for `cd` command to change the working directory
- Support for `history` command to display previous commands
- Exit the shell using the `quit` command

## Implementation Details

- Tokenize the command based on `|` and `;`
- Use `fork()` to create new processes
- Use `execvp()` to execute commands
- Use `waitpid()` system call to wait for concurrent commands to finish
- Use temporary files to handle many-to-many "piping"
- Properly handle command arguments and termination with `NULL`

## Compilation

To compile the project, navigate to the project directory and run the following command in your terminal:

```sh
make
```

This will create an executable file named hwsh.

## Usage

To run the shell with a script, use the following command:

```sh
./hwsh <batchfile>
```

Replace `<batchfile>` with the name of your batch file. For example, if your batch file is named `script.sh`, you would run:

```sh
./hwsh script.sh
```
