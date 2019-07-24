#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "smallsh-u.h"

int parse_input(struct command_data *data)
{
	int i;
	data->fg = 1;
	data->input_file = "/dev/null";
	data->output_file = "/dev/null";
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
		//We copy
		data->arg_list[i] = token;
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
				/* We'll do some error handling, here.
				 * If the user put down a redirector
				 * but didn't actually specify a redirection,
				 * we'll allow that to count as /dev/null.
				 */
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
