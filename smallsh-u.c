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

int parse_input(struct command_data *data)
{
	int i;
	data->fg = 1;
	data->input_file = NULL;
	data->output_file = NULL;
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
	data->arg_list[0] = token;
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
		else if (!strcmp(token, "$$"))
		{
			data->arg_list[i] = data->pid;
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
