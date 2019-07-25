// Michael Andrews, CS 344, Project 3: smallsh

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "smallsh-u.h"

#define MAX_INPUT_SIZE 2048

/* These variables need to be global because signal handlers will interact
 * with them. */
int bg_permitted = 1; //Toggle whether programs are allowed to run in the bg.

int sig_pipe[2]; //Pipe for enqueuing messages main will spit out when free.

int fg_active = 0; //Toggle to let the SIGSTP handler know whether to print
		     //immediately, or enqueue.

int last_fg_status = 0; //The status var.  If a forked child fails its call
			//to exec, we'll use SIGUSR1 to convey that information
			//back, so the handler needs this to be global.

int main()
{
	//This is a struct that we'll use to package up all the
	//information about the command the user inputs; we pass
	//it by reference into the parsing function, which will tokenize it
	//up for us and prepare any redirects, etc.
	struct command_data input;
	input.input_buffer = NULL;
	input.expanded_args = NULL;
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
			if (input.input_buffer)
			{
				free(input.input_buffer);
			}
			free_expansion_links(&(input.expanded_args));
			close(sig_pipe[0]);
			close(sig_pipe[1]);
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
