// Michael Andrews, CS 344, Project 3: smallsh

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include "smallsh-u.h"
#include "smallsh-sig.h"

#define MAX_INPUT_SIZE 2048

sig_atomic_t bg_permitted = 1; //Toggle whether programs are allowed 
				//to run in the bg.

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

	signal(SIGTSTP, sigtstp_handler);
	//We'll want to know what the PID of the master shell is for a couple
	//of reasons; one is so that we can abort any fork() call that
	//comes from some other instance of the shell (which shouldn't happen)
	//and the other is so that the parser can do $$ expansion.
	master_pid = getpid();

	//We need to ignore SIGINT in the main shell.  Alarmingly.
	signal(SIGINT, SIG_IGN);

	//We'll give a stringified copy of the master PID to the input
	//struct, which it will use for $$ expansion.
	sprintf(input.pid, "%d", (int) master_pid);
	while (1)
	{
		/*The first thing we do on each pass is see if we have any
		 * zombies in need of collection.
		 */
		int reaped = 0;
		int reaped_status;
		while ( (reaped = waitpid(-1, &reaped_status, WNOHANG)) > 0 )
		{
			printf("background pid %d is done: ", reaped);
			print_status(&reaped_status);
		}

		//The prompt.  We start with a carriage return because
		//we might end up with a double-colon if, say, the user
		//hits Ctrl-Z while a foreground process is running.
		//Because the signal handler provides a colon of its own,
		//this might be redundant.
		printf("\r:");
		fflush(stdout);
		getline(&input.input_buffer, &max_input, stdin);

		//parse_input returns 1 if it received a comment line,
		//or an empty line.
		if (parse_input(&input))
		{
			continue;
		}
		/*The builtins are simple enough that I didn't break them
		 *out into functions, with the exception of status, which
		 *is used a few other places. */
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
		}
		else if (!strcmp(input.arg_list[0], "status"))
		{
			print_status(&last_fg_status);
		}
		/* If we're here, we've got valid input which wasn't a builtin,
		 * so we need to check if bg status has been requested and
		 * is permitted.
		 */
		else if (!input.fg && bg_permitted)
		{
			spawn_bg(&input);
		}
		else
		{
			spawn_fg(&input, &last_fg_status);
		}
				
	}

	return 0;
}
