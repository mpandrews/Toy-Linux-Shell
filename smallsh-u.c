#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include "smallsh-u.h"
#include "smallsh-sig.h"

extern volatile sig_atomic_t bg_permitted;

void free_expansion_links(struct dollar_expansion_link **head)
{
	if ( !(*head) )
	{
		return;
	}
	struct dollar_expansion_link *current, *next;
	current = *head;
	*head = NULL;

	do
	{
		if (current->value)
		{
			free(current->value);
		}
		next = current->next;
		free(current);
		current = next;
	}while(current);

	return;
}

char* make_expansion_string(char* input, char* pid_str)
{
	if (!input || !pid_str)
	{
		fprintf(stderr, "Error: make_expansion_string called");
		fprintf(stderr, " on empty string!\n");
		term_and_exit(1);
	}
	char* dollar_ptr = strstr(input, "$$");
	
	//We should never be in this function if the input string doesn't
	//contain $$, but just in case that happens we'll malloc a new
	//string, copy the input, and return it.  The reason for this is
	//simply that if we don't, we'll 
	if (!dollar_ptr)
	{
		char* return_str = malloc( (strlen(input) + 1) * sizeof(char) );
		if (!return_str)
		{
			fprintf(stderr, "Malloc failure in $$ expansion.\n");
			term_and_exit(1);
		}
		strcpy(return_str, input);
		return return_str;
	}

	int pid_width = strlen(pid_str);
	// Edge cases: 1 and 2 digit pids.  We can simplify the routine in
	// these cases.  We'll still allocate memory and copy the strings over,
	// though, because the cost of the malloc and copy is less than
	// that of adding logic to the linked-list freeing routing to avoid
	// trying to free a pointer to the middle of string.
	
	if (pid_width == 2)
	{
		char* return_str = malloc( (strlen(input) + 1) * sizeof(char) );
		if (!return_str)
		{
			fprintf(stderr, "Malloc failure in $$ expansion.\n");
			term_and_exit(1);
		}
		strcpy(return_str, input);
		//Just copy replace the dollar signs directly.
		while (( dollar_ptr = strstr(return_str, "$$") ))
		{
			strncpy(dollar_ptr, pid_str, 2);
		}
		return return_str;
	}
	else if (pid_width == 1)
	{
		char* return_str = malloc( (strlen(input) + 1) * sizeof(char) );
		if (!return_str)
		{
			fprintf(stderr, "Malloc failure in $$ expansion.");
			term_and_exit(1);
		}
		strcpy(return_str, input);
		//Shift everything left 1.
		while(( dollar_ptr = strstr(return_str, "$$") ))
		{
			char* return_end = return_str + strlen(return_str) - 1;
			for (char* iter = dollar_ptr + 1; iter <= return_end;
								iter++)
			{
				*iter = *(iter + 1);
			}
			*dollar_ptr = *pid_str;
		}
		return return_str;
	}

	//Determine the index at which the first dollar sign appears:
	int dollar_idx = dollar_ptr - input;
	//Get the width of the pid string.
	//Malloc a new string big enough to hold the input, plus the pid_width
	//less 1.  (-2 for the $$, and + 1 for the null terminator.):
	char* return_str = malloc( (strlen(input) + pid_width - 1) 
							* sizeof(char) );
	if (!return_str)
	{
		fprintf(stderr, "Malloc failure in $$ expansion.\n");
		term_and_exit(1);
	}
	return_str[0] = '\0';
	//Copy everything up to the first dollar sign:
	strncat(return_str, input, dollar_idx);
	//Copy the pid string.
	strcat(return_str, pid_str);
	//Copy everything after the $$.
	strcat(return_str, dollar_ptr + 2);

	//And now the routine for the specific case that we have a token
	//with more than one set of $$ to expand, and a pid >= 3 digits.
	while(( dollar_ptr = strstr(return_str, "$$") ))
	{
		
		//Reallocate the return string with enough space to hold
		//another $$ expansion.
		return_str = realloc(return_str, 
				(strlen(return_str) + pid_width - 1)
							* sizeof(char));
		if (!return_str)
		{
			fprintf(stderr, "Failure in reallocating $$ exp.");
			fprintf(stderr, " string!\n");
			term_and_exit(1);
		}

		//We need to re-verify the position of the $$ substring,
		//in case the realloc was not in-place:
		dollar_ptr = strstr(return_str, "$$");
		//Create a pointer to the end of the original string,
		//counting the null terminator.
		char *iter = return_str + strlen(return_str);
		//Shift everything to the right.
		for (; iter > dollar_ptr + 1; iter--)
		{
			*(iter + (pid_width - 2) ) = *iter;
		}
		strncpy(dollar_ptr, pid_str, strlen(pid_str));
	}
	return return_str;
}

char* make_expansion_link(struct dollar_expansion_link **head,
		char* input_str, char* pid_str)
{
	if (!(*head))
	{
		*head = malloc(sizeof(struct dollar_expansion_link));
		if (!*(head))
		{
			fprintf(stderr, "Error malloc'ing new expansion ");
			fprintf(stderr, " link list head!\n");
			term_and_exit(1);
		}
		(*head)->next = NULL;
		(*head)->value = NULL;
	}
	struct dollar_expansion_link* iter = *head;
	//Navigate to the end of the list.
	while(iter->next)
	{
		iter = iter->next;
	}
	//Make a new item.
	iter->next = malloc(sizeof(struct dollar_expansion_link));
	iter = iter->next;
	iter->next = NULL;
	iter->value = make_expansion_string(input_str, pid_str);
	return iter->value;
}

int parse_input(struct command_data *data)
{
	int i;
	data->fg = 1;
	data->input_file = NULL;
	data->output_file = NULL;
	free_expansion_links(&(data->expanded_args));
	//We'll check to make sure we weren't passed a null pointer.
	if (!data->input_buffer)
	{
		fprintf(stderr, "parse_input called on null input buffer!\n");
		term_and_exit(1);
	}
	//If we got an empty line or a comment line, we should return 1
	//so that main knows not to try to execute anything.
	if (data->input_buffer[0] == '#' || data->input_buffer[0] == '\n')
	{
		return 1;
	}
	//Because getline() takes the \n character along with everything
	//else, we'll strip that off too.  We need to actually check
	//if it's there, though, because it could be that getline()
	//hit the maximum line length rather than a newline.
	int input_len = strlen(data->input_buffer);

	if (data->input_buffer[input_len - 1] == '\n')
	{
		data->input_buffer[input_len - 1] = '\0';
	}
	//Now we'll break the line up into, well, stuff.
	char* token;
	token = strtok(data->input_buffer, " ");
	//Check for an empty line.  This shouldn't happen.
	if (token == NULL)
	{
		return 1;
	}
	//Populate the argument list.
	if (!strstr(token, "$$"))
	{
		data->arg_list[0] = token;
	}
	else
	{
		data->arg_list[0] = make_expansion_link(&(data->expanded_args), 
							token, data->pid);
	}
	for (i = 1; i < 512; i++)
	{
		token = strtok(NULL, " ");
		//If we find $$, we need to expand any and all instances
		//into instances of the PID.  Because we can't expand
		//in-place, we have a linked list we can populate with
		//expanded token strings.
		if (!token)
		{
			data->arg_list[i] = NULL;
			break;
		}
		if (strstr(token, "$$"))
		{
			data->arg_list[i] = make_expansion_link(
				&(data->expanded_args), token, data->pid);
		}
		else
		{
			data->arg_list[i] = token;
		}
	}
	//Failsafe in case we got the full 512 args.
	data->arg_list[512] = NULL;

	/*Now we need to check for redirections and background.
	 * We start by seeing if the very last argument is an ampersand.
	 * As a failsafe, we'll return immediately if there is only one
	 * arg in the list. At that point, ampersand should probably
	 * be treated as the command itself, rather than an instruction
	 * to background nothing.
	 */
	i--;
	if (i < 1)
	{
		return 0;
	}
	/* If we find an ampersand, we set the foreground flag to false,
	 * and then remove the ampersand from the argument array.
	 */
	if (!strcmp(data->arg_list[i], "&"))
	{
		data->fg = 0;
		data->arg_list[i] = NULL;
		i--;
	}
	/* Now the redirects: we'll loop through twice, because it could be
	 * in any order.  Again, we'll return out if this would either
	 * put us out of bounds or leave us redirecting nothing at all.
	 */
	if (i < 2)
	{
		return 0;
	}
	for (int j = 0; j < 2; j++)
	{
		if (!strcmp(data->arg_list[i - 1], "<"))
		{
			data->input_file = data->arg_list[i];
			data->arg_list[i] = NULL;
			data->arg_list[i - 1] = NULL;
			i -= 2;
		}
		else if (!strcmp(data->arg_list[i - 1], ">"))
		{
			data->output_file = data->arg_list[i];
			data->arg_list[i] = NULL;
			data->arg_list[i - 1] = NULL;
			i -= 2;
		}
		if (i < 2)
		{
			return 0;
		}
	}

	return 0;
}

void redirect_in(const char* source)
{
	int in_descriptor = open(source, O_RDONLY);
	if (in_descriptor == -1)
	{
		fprintf(stderr, "cannot open %s for input:",
				source);
		perror("");
		//We won't call term_and_exit because this should only
		//happen in the child, and we don't want to kill the parent.
		exit(1);
	}
	dup2(in_descriptor, STDIN_FILENO);
}

void redirect_out(const char* dest)
{
	int out_descriptor = open(dest, O_WRONLY | O_TRUNC | O_CREAT, 0700);
	if (out_descriptor == -2)
	{
		fprintf(stderr, "cannot open %s for output:",
				dest);
		perror("");
		exit(1);
	}
	dup2(out_descriptor, STDOUT_FILENO);
}

void print_status(int *status)
{
	if (WIFEXITED(*status))
	{
		printf("exit value %d\n", WEXITSTATUS(*status));
	}
	else if (WIFSIGNALED(*status))
	{
		printf("terminated by signal %d\n", WTERMSIG(*status));
	}
	fflush(stdout);
}

/* TODO: spawn_fg and spawn_bg contain a lot of duplicated code, and should
 * probably be refactored into a single function when there's time.  They
 * started out quite different from each other, but over time they kinda
 * grew into almost-clones.
 */

void spawn_fg(struct command_data *data, int *status)
{
	//We'll temporarily block SIGTSTP to prevent the signal handler
	//from going off while we have a fg process.	
	sigset_t fgsigs;
	sigset_t oldsigs;
	sigemptyset(&fgsigs);
	sigaddset(&fgsigs, SIGTSTP);
	sigprocmask(SIG_BLOCK, &fgsigs, &oldsigs);

	//We'll set up the handlers for the child process, so that it can
	//use the default SIGINT and ignore SIGTSTP.
	struct sigaction child_tstp;
	sigemptyset(&child_tstp.sa_mask);
	struct sigaction child_int;
	sigemptyset(&child_int.sa_mask);
	//SA_RESTART is the only flag set by default, so we'll set it.
	child_int.sa_flags = child_tstp.sa_flags = SA_RESTART;
	child_tstp.sa_handler = SIG_IGN;
	child_int.sa_handler = SIG_DFL;

	int fork_ret = fork();
	switch (fork_ret)
	{
		case -1:
			perror("Forking failed!");
			term_and_exit(1);
			break;
		case 0:
			//foreground processes need to ignore TSTP, but
			//allow INT to hit.  We'll unblock TSTP, though:
			//since we're ignoring it, we may as well not have them
			//pile up on the doorstep, right?
			sigaction(SIGTSTP, &child_tstp, NULL);
			sigaction(SIGINT, &child_int, NULL);
			sigprocmask(SIG_SETMASK, &oldsigs, NULL);
			//If redirection was specified, we'll set it up here.
			int fallback_stdout = dup(STDOUT_FILENO);
			if (data->input_file)
				redirect_in(data->input_file);
			if (data->output_file)
				redirect_out(data->output_file);
			//The actual execution!
			execvp(data->arg_list[0], data->arg_list);
			//Bad, bad things have happened if we're here.
			
			//First thing to do is restore stdout so that we
			//can re-prompt.
			dup2(fallback_stdout, STDOUT_FILENO);
			close(fallback_stdout);
			
			//Now the error messages;
			fprintf(stderr, "\r%s: ", data->arg_list[0]);
			perror("");
			printf("\r: ");
			*status = 1;
			exit(1);
			break;
		default:
			waitpid(fork_ret, status, 0);
			if(WIFSIGNALED(*status))
			{
				print_status(status);
			}
			break;
	}
	//We'll restore the old signal mask out here, to cover the oddball
	//case that fork failed and we're cycling to a new prompt.
	sigprocmask(SIG_SETMASK, &oldsigs, NULL);
}

void spawn_bg(struct command_data *data)
{
	
	//We'll create sigaction structs, so that we can use them to SIGTSTP 
	//in the child process, then unignore
	//SIGINT.
	struct sigaction tstp_handler;
	struct sigaction int_handler;
	sigemptyset(&tstp_handler.sa_mask);
	sigemptyset(&int_handler.sa_mask);
	tstp_handler.sa_flags = int_handler.sa_flags = SA_RESTART;
	tstp_handler.sa_handler = SIG_IGN;
	int_handler.sa_handler = SIG_DFL;

	
	int fork_ret = fork();
	switch (fork_ret)
	{
		case -1:
			perror("Forking failed!");
			term_and_exit(1);
			break;
		case 0:
			sigaction(SIGTSTP, &tstp_handler, NULL);
			sigaction(SIGINT, &int_handler, NULL);
			
			int fallback_stdout = dup(STDOUT_FILENO);
			if (data->input_file)
			{
				redirect_in(data->input_file);
			}
			else
			{
				redirect_in("/dev/null");
			}
			if (data->output_file)
			{
				redirect_out(data->output_file);
			}
			else
			{
				redirect_out("/dev/null");
			}
			execvp(data->arg_list[0], data->arg_list);
			dup2(fallback_stdout, STDOUT_FILENO);
			close(fallback_stdout);
			fprintf(stderr, "\r%s:", data->arg_list[0]);
			perror("");
			printf("\r: ");
			exit(1);
			break;
		default:
			printf("background pid is %d\n", fork_ret);
			fflush(stdout);
			break;
	}
}

void term_and_exit(int exitcode)
{
	sigset_t term_mask;
	sigemptyset(&term_mask);
	sigaddset(&term_mask, SIGTERM);
	sigprocmask(SIG_BLOCK, &term_mask, NULL);
	kill(0, SIGTERM);
	exit(exitcode);
}
