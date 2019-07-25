#ifndef SMALLSH_SIG
#define SMALLSH_SIG

/* sigstp_handler:  A very simple handler for SIGTSTP.
 * It merely prints a descriptive message and toggles a flag.
 * Precondition: It's a signal handler.  In normal operation, it should be
 * toggled off when not desired.  In pratice, we block the signal while
 * a foreground process is running, to keep the printed message from wrecking
 * any output that may be happening.
 * Postcondition: The flag will be toggled.
 */
void sigtstp_handler(int);

#endif
