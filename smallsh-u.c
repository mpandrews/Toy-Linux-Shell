#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "smallsh-u.h"

extern int bg_permitted;

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
		exit(1);
	}
	char* dollar_ptr = strstr(input, "$$");
	//We'll just kick the original string back out if we come up empty.
	//This will allow us to handle the edge case where we might
	//conceivably have multiple expansions in a single token.
	if (!dollar_ptr)
	{
		return input;
	}
	//Determine the index at which the first dollar sign appears:
	int dollar_idx = dollar_ptr - input;
	//Malloc a new string big enough to hold the input, plus three chars:
	char* return_str = malloc( (strlen(input) + 4) * sizeof(char) );
	if (!return_str)
	{
		fprintf(stderr, "Error: failed to malloc new string in");
		fprintf(stderr, " make_expansion_string()!\n");
		exit(1);
	}
	return_str[0] = '\0';
	//Copy everything up to the first dollar sign:
	strncat(return_str, input, dollar_idx);
	//Copy the pid string.
	strcat(return_str, pid_str);
	//Copy everything after the $$.
	strcat(return_str, dollar_ptr + 2);

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
			exit(1);
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
	char *test_string = make_expansion_string(iter->value, pid_str);
	if (test_string == iter->value)
	{
		return iter->value;
	}
	//This will test to see if we have additional instances of
	//$$ in the string, and repeatedly generate new strings
	//if we do.  It's an edge case, so I'm not going to fret too much
	//about the cycles it will take.
	do
	{
		free(iter->value);
		iter->value = test_string;
		test_string = make_expansion_string(iter->value, pid_str);
	}while (test_string != iter->value);
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
		exit(1);
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
		//>, <, and & must go after all args.
		if (!token || token[0] == '<' ||
				token[0] == '>' || token[0] == '&')
		{
			data->arg_list[i] = NULL;
			break;
		}
		else if (strstr(token, "$$"))
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

	//Now we need to check for redirections and background.

	while (token)
	{
		// We'll switch on the first character of the string.
		switch (token[0])
		{
			case '<':
				token = strtok(NULL, " ");
				if (token)
					data->input_file = token;
				break;
			case '>':
				token = strtok(NULL, " ");
				if (token)
					data->output_file = token;
				break;
			case '&':
				data->fg = 0;
				break;
		}
		// If we already hit a null pointer, we don't need to tokenize
		// further, we can just let the while loop end normally.
		if (token)
			token = strtok(NULL, " ");
	}

	return 0;
}

void redirect_in(const char* source)
{
	int in_descriptor = open(source, O_RDONLY);
	if (in_descriptor == -1)
	{
		fprintf(stderr, "Could not redirect input from file %s.\n",
				source);
		perror("");
		exit(1);
	}
	dup2(in_descriptor, STDIN_FILENO);
}

void redirect_out(const char* dest)
{
	int out_descriptor = open(dest, O_WRONLY);
	if (out_descriptor == -2)
	{
		fprintf(stderr, "Could not redirect output to file %s.\n",
				dest);
		perror("");
		exit(1);
	}
	dup2(out_descriptor, STDOUT_FILENO);
}

void spawn_fg(struct command_data *data)
{
	int fork_ret = fork();
	int return_info;
	
	switch (fork_ret)
	{
		case -1:
			perror("Forking failed");
			break;
		case 0:
			if (data->input_file)
				redirect_in(data->input_file);
			if (data->output_file)
				redirect_out(data->output_file);
			//The actual execution!
			execvp(data->arg_list[0], data->arg_list);
			fprintf(stderr, "Execution failed!\n");
			exit(1);
			break;
		default:
			waitpid(fork_ret, &return_info, 0);
			break;
	}


}
