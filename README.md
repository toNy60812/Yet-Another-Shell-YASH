# Yet-Another-Shell-YASH

## Objective

In this lab you will be introduced to both the command line interface and the Unix programming environment. 
You will write a command line interpreter known as a shell that takes commands from standard input and executes the commands by creating processes.
A standard shell like bash/tcsh/csh etc. has a rich set of features that it supports. We picked a subset of these features for you to implement.

## Features

Here is the complete list of features you must implement:  
- File redirection
  - with creation of files if they don't exist for output redirection
  - fail command if input redirection (a file) does not exist
  - < will replace stdin with the file that is the next token
  - &gt; will replace stdout with the file that is the next token
  - 2> will replace stderr with the file that is the next token
  - A command can have all three (or a subset) of the redirection symbols 

- Piping
  - | separates two commands
    - The left command will have stdout replaced with the input to a pipe
    - The right command will have stdin replaced with the output from the same pipe
  - Children within the same pipeline will be in one process group for the pipeline
  - Children within the same pipeline will be started and stopped simultaneously
  - You are only required to support (at most) one | per command

- Signals (SIGINT, SIGTSTP, SIGCHLD)
  - Ctrl-c must quit current foreground process (if one exists) and not the shell and should not print the process (unlike bash)
  - Ctrl-z must send SIGTSTP to the current foreground process and should not print the process (unlike bash)
  - The shell will not be stopped on SIGTSTP

- Job control
  - Background jobs using &
  - fg must send SIGCONT to the most recent background or stopped process, print the process to stdout , and wait for completion
  - bg must send SIGCONT to the most recent stopped process, print the process to stdout in the jobs format, and not wait for completion (as if &)
  - jobs will print the job control table similar to bash. HOWEVER there are important differences between your yash shell's output and bash's output for the jobs command!
    - with a `[<jobnum>]`
    - a + or - indicating the current job. Which job would be run with fg is indicated with a + and all others with a -
    - a "Stopped" or "Running" indicating the status of the process
    - and finally the original command
    - e.g:
    
      [1] - Running       sleep 5 &
      
      [2] - Stopped       sleep 5
      
      [3] + Running       running_command | grep > out.txt &
  - Terminated background jobs will be printed after the newline character sent on stdin with a Done in place of the Stopped or Running.
  - A job is all children within one pipeline as defined above
  - Max number of jobs running at the same time: 20

- Misc
  - Children must inherit the environment from the parent
  - Must search the PATH environment variable for every executable
  - All child processes will be dead on exit
  - The prompt must be printed as a "# " (pound sign with a space after it) before accepting user input.
  - Make sure that your shell doesn't segfault (for whatever reason) when you run something like this because our grading script uses the 'printf' command with the '%b' format. If you see the quotation marks at the beginning and end, that's fine (and expected, unless you do fancy parsing).
    - printf %b "\033[34m Blue \040 text \033[0m\n"
    - ls

## Restrictions on the input

These restrictions will help you simplify the parsing of the command line:
- Everything that can be a token (<, >, 2>, etc.) will have a space before and after it. Also, any redirections will follow the command after all its args.
- If a command has a & symbol (indicating that it will be run in the background), then the & symbol will always be the last token in the line.
  - Valid examples
    - ls &
    - sleep 4 | sleep 6 &
  - Invalid examples
    - sleep 4 & | sleep 6 &
    - sleep 4 & | sleep 6
- Each line contains either one command or two commands in one pipeline
- Lines will not exceed 2000 characters
- Each token will be no more than 30 characters
- All characters will be ASCII
- Ctrl-d will exit the shell
