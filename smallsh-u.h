#ifndef SMALLSH_U
#define SMALLSH_U

// Basic linked-list struct.
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


/* free_expansion_links()
 * Precondition: The link pointer pointed to by the argument must be
 *  either null or dynamically allocated.  If an automatic variable is passed
 *  in, the free calls will break everything.
 * Postcondition: Any linked-list headed by the pointed-to link will be freed
 *  in its entirety.
 */
void free_expansion_links(struct dollar_expansion_link**);

/* parse_input: The main input parser.
 * Preconditions: 
 * 	The input member should point to a valid string.
 * 	The expanded_args member should be the head of a dynamically allocated
 * 	linked list, or null.
 * 	The pid member should point to a valid string. (The pid, ideally, but
 * 	it will work as long as there's a string there.)
 * Postcondition:
 * 	The struct will be parsed and can be used for passing to a command
 * 	spawning function. Dynamic memory may be allocated within the
 * 	expanded_args linked list.  The input member will be destructively 
 * 	tokenized into potentially hundreds of 0 delimited substrings.
 * 	Note that no cleanup is required before parsing a new input other than
 * 	providing a new input to parse.  The function leaves the struct in a
 * 	state it can clean up by itself, assuming a fresh string is assigned
 * 	to the input member.
 */
int parse_input(struct command_data*);

/* spawn_fg: Spawns a foreground process.
 * Params: A populated input struct, and a pointer-to-int to hold the exit
 * data for the spawned process after its termination.
 * Precondition:
 * 	The data struct must be validly parsed.  It is not strictly speaking
 * 	a precondition that it be a valid command.
 * Postcondition: 
 * 	The function will only return once the command run has
 * 	terminated.  If all goes well, the only change to the program state
 * 	afterwards will be that the exit status will be repopulated.  If
 * 	fork() or exec() fail, though, all bets are off.
 */
void spawn_fg(struct command_data*, int*);

/* spawn_bg: Spawns a background process.
 * Preconditions:
 * 	The data struct must be validly parsed.
 * Postcondition:
 * 	Ideally, a process will be spawned in the background and shell
 * 	control will return to the user.  The program's state is not
 * 	modified beyond that, unless an error occurs.
 */
void spawn_bg(struct command_data*);

/* print_status: Prints a human readable string describing the fate of a
 * process.
 * Preconditions:
 * 	The argument passed in should ideally be the flag variable set by
 * 	wait() or waitpid().  I'm calling it undefined if you pass in, like, 6
 * 	or something.
 * Postcondition:
 * 	status will be modified iff exec fails, but not otherwise.  Absent that,
 * 	the program state is not modified.
 */
void print_status(int*);

/* term_and_exit: This will block SIGTERM, raise SIGTERM for the process group,
 * and then call exit with the specified code.  This is just to make sure that
 * we always murder our children before dying ourselves, which is of course
 * the right and proper thing to do.
 * Precondition: This should not be called before the shell has assigned itself
 * a new process group, or it may do unwholesome things to its own ancestors.
 * Postcondition: It will have made a desert and called it peace.
 */
void term_and_exit(int exitcode);

#endif
