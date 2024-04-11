# Simple Shell Implementation

## Overview
This project implements a basic shell interface that allows interactions between the user and the operating system (OS). The shell supports a variety of built-in commands as well as the execution of local executables both in the foreground and background. It is designed as an assignment for ICS 53, Fall 2023.

## Features
- **Built-in Commands:** Supports basic shell commands including `cd`, `pwd`, `quit`, `jobs`, `bg`, `fg`, `kill`.
- **Execution of Local Executables:** Allows executing compiled binaries present in the local directory.
- **Foreground and Background Jobs:** Manages jobs in the foreground and background, implementing job control and signal handling.
- **I/O Redirection:** Supports input and output redirection using `>`, `<`, and `>>` operators for file handling.

## Getting Started
To use this shell:
1. Compile the source code using GCC version 11.3.0 with the command: `gcc -o myshell hw2.c`.
2. Run the shell by executing `./myshell`.
3. You'll be greeted by a prompt `prompt >` where you can start entering your commands.

## Commands
The shell supports the following commands:
- `cd <directory>`: Changes the current working directory.
- `pwd`: Displays the current working directory.
- `quit`: Exits the shell.
- `jobs`: Lists all background jobs with their status (Running or Stopped).
- `bg <job_id|pid>`: Moves a job to the background.
- `fg <job_id|pid>`: Brings a job to the foreground.
- `kill <job_id|pid>`: Terminates the specified job.

Local executables can be run by specifying their name. Add `&` at the end of the command to run them in the background.

## Signal Handling
- `Ctrl+C`: Terminates the current foreground job.
- `Ctrl+Z`: Stops the current foreground job, moving it to the Stopped state.

## I/O Redirection
- To redirect output to a file, use command `> filename`.
- To take input from a file, use command `< filename`.
- To append output to a file, use command `>> filename`.

Ensure that the file permissions are set correctly for I/O redirection operations as described in the assignment document.

## Limitations
- The shell supports a maximum of 80 arguments for any command.
- Up to 5 background jobs can be managed simultaneously.

## Development Environment
Ensure your development is done on `openlab.ics.uci.edu` as the shell is tested and intended to be run in this environment using GCC version 11.3.0.
