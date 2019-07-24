// Michael Andrews, CS 344, Project 3: smallsh

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "smallsh-u.h"

#define MAX_INPUT_SIZE 2048

/* These two variables need to be globals because the signal handlers will
 * user them.  bg_permitted will be toggled on and off by the SIGTSTP handler,
 * while sig_pipe will be used to enqueue terminal messages for later
 * delivery by main().
 */
int bg_permitted = 1;
int sig_pipe[2];

int main()
{
	//This is a struct that we'll use to package up all the
	//information about the command the user inputs; we pass
	//it by reference into the parsing function, which will tokenize it
	//up for us and prepare any redirects, etc.
	struct command_data input;
	input.input_buffer = NULL;
	size_t max_input = MAX_INPUT_SIZE;
	pid_t master_pid;
	//We'll want to know what the PID of the master shell is for a couple
	//of reasons; one is so that we can abort any fork() call that
	//comes from some other instance of the shell (which shouldn't happen)
	//and the other is so that the parser can do $$ expansion.
	master_pid = getpid();
	sprintf(input.pid, "%d", (int) master_pid);
	//This pipe will be used to enqueue terminal output from the signal
	//handlers, so that main can spit them out ahead of the prompt.
	if (pipe(sig_pipe) == - 1)
	{
		perror("Failed to create pipe:");
		exit(1);
	}
	close(sig_pipe[1]);

	char pipe_buffer[100];
	while (1)
	{
		/* The first thing we do on a loop is check the signal
		 * pipe to see if we have been left any messages that we
		 * should print.
		 */
		while ( read(sig_pipe[0], pipe_buffer, 99) > 0)
		{
			printf("%s", pipe_buffer);
		}

		printf("\n:");
		getline(&input.input_buffer, &max_input, stdin);

		//parse_input returns 1 if it received a comment line,
		//or an empty line.
		if (parse_input(&input))
		{
			continue;
		}
		
		if (!strcmp(input.arg_list[0], "exit"))
		{
			exit(0);
		}
		else if (!strcmp(input.arg_list[0], "cd"))
		{
			printf("CD!\n");
			continue;
		}
		else if (!strcmp(input.arg_list[0], "status"))
		{
			printf("STATUS!\n");
			continue;
		}
		else if (!input.fg && bg_permitted)
		{
			printf("BG!");
			continue;
		}
		else
		{
			spawn_fg(&input);
			continue;
		}
				
	}

	return 0;
}
