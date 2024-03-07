# BLG312E Computer Operating Systems Spring 2024 Homework 1

This assignment involves implementing a command-line shell that can be run in batch mode. The shell is initiated by specifying a batch file on its command line, which contains a list of commands to be executed. Each line in the batch file can contain multiple commands separated by `;` or `|`. Commands separated by `;` should be run simultaneously or concurrently, while commands separated by `|` should be executed with the output of the preceding commands piped as input to the subsequent commands.

## Features
- Execute multiple commands separated by `;` or `|` simultaneously
- Support for `cd` command to change the working directory
- Support for `history` command to display previous commands
- Exit the shell using the `quit` command

## Implementation Details
- Echo each line read from the batch file before executing it
- Stop accepting new commands when encountering `quit` or reaching the end of input stream
- Exit after all running processes have terminated
- Use `wait()` and/or `waitpid()` system calls to wait for concurrent commands to finish
- Use `fork()` to create new processes
- Use `execvp()` to execute commands
- Properly handle command arguments and termination with `NULL`

## Usage
- To be filled