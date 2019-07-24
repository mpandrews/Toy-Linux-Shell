#ifndef SMALLSH_U
#define SMALLSH_U

struct command_data
{
	char* input_buffer;
	int fg;
	char* output_file;
	char* input_file;
	char* arg_list[513];
	char pid[6];
};

struct bg_children
{
	int* pids;
	int num_children;
};

int parse_input(struct command_data*);

void spawn_fg(struct command_data*);

#endif
