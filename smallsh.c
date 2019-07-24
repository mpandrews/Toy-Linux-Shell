// Michael Andrews, CS 344, Project 3: smallsh

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "smallsh-u.h"

int main()
{
	struct command_data input;
	input.input_buffer = NULL;
	size_t max_input = 2048;
	getline(&input.input_buffer, &max_input, stdin);

	parse_input(&input);
	for (int i = 0; i < 512; i++)
	{
		if (!input.arg_list[i])
			break;
		printf("%s\n", input.arg_list[i]);
	}
	printf("Input: %s\n", input.input_file);
	printf("Output: %s\n", input.output_file);
	printf("FG: %d\n", input.fg);

	return 0;
}
