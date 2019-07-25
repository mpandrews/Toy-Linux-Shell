// Michael Andrews, CS 344, Project 3: smallsh

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "smallsh-u.h"
#include "smallsh-sig.h"

#define MAX_INPUT_SIZE 2048

/* These variables need to be global because signal handlers will interact
 * with them. */
sig_atomic_t bg_permitted = 1; //Toggle whether programs are allowed 
				//to run in the bg.
sig_atomic_t fg_active = 0; //Toggle to let the SIGSTP handler know whether 
			     //to print immediately, or enqueue.

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
	int last_fg_status;
	//We'll want to know what the PID of the master shell is for a couple
	//of reasons; one is so that we can abort any fork() call that
	//comes from some other instance of the shell (which shouldn't happen)
	//and the other is so that the parser can do $$ expansion.
	master_pid = getpid();
	sprintf(input.pid, "%d", (int) master_pid);
	while (1)
	{
		/* The first thing we do on a loop is check the signal
		 * pipe to see if we have been left any messages that we
		 * should print.
		 */
		int reaped = 0;
		int reaped_status;
		while ( (reaped = waitpid(-1, &reaped_status, WNOHANG)) > 0 )
		{
			printf("background pid %d is done: ", reaped);
			print_status(&reaped_status);
		}

		printf(":");
		fflush(stdout);
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
			exit(0);
		}
		else if (!strcmp(input.arg_list[0], "cd"))
		{
			if (!input.arg_list[1])
			{
				chdir(getenv("HOME"));
			}
			else
			{
				chdir(input.arg_list[1]);
			}
			continue;
		}
		else if (!strcmp(input.arg_list[0], "status"))
		{
			print_status(&last_fg_status);
			continue;
		}
		else if (!input.fg && bg_permitted)
		{
			spawn_bg(&input);
			continue;
		}
		else
		{
			spawn_fg(&input, &last_fg_status);
			continue;
		}
				
	}

	return 0;
}
