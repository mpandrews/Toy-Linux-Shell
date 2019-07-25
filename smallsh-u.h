#ifndef SMALLSH_U
#define SMALLSH_U

/*The basic approach we're going to use to parsing input is to use strtok
 *to carve up the input, and simply populate the argument list with pointers
 *to the tokens.  We can't do that for tokens that involve $$ expansion, so
 *we'll use a simple linked-list structure to store expanded strings.
 */
 struct dollar_expansion_link
{
	char* value;
	struct dollar_expansion_link* next;
};

/* This is the command_data structure we're going to use to hold all the, well,
 * stuff.  It's not really necessary, but having it lets us more simply break
 * the input parsing function out of main.
 */
struct command_data
{
	char* input_buffer; //Sizeless, since we'll use getline().
	int fg; //Flag to indicate fg/bg status.
	char* output_file; //In case we find redirection.
	char* input_file;
	char* arg_list[513];
	char pid[6]; //Used for $$ expansion.
	struct dollar_expansion_link *expanded_args; //Used for $$ expansion.
};

struct bg_children
{
	int* pids;
	int num_children;
};

/* free_expansion_links will free any linked list of $$ expanded arguments
 * we might have.  We have in the header so main() can call it as part
 * of its exit routine.*/
void free_expansion_links(struct dollar_expansion_link**);

/* The main input parser.  It does expect that its input buffer will be filled
 * by main before it's called, but it will break the input down into
 * a command and arguments, do any $$ expansion, and populate the pointers
 * if redirection is requested, as well as setting the fg/bg flag.
 */
int parse_input(struct command_data*);

void spawn_fg(struct command_data*, int*);

void print_status(int*);

#endif
