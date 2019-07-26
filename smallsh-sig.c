#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
//sys/wait.h is needed for sig_atomic_t
#include <sys/wait.h>

//The flag we toggle.
extern volatile sig_atomic_t bg_permitted;

//This should be self explanatory.  The flag is used to control
//whether processes are allowed to spawn in the background.
void sigtstp_handler(int signal)
{
	if (bg_permitted)
	{
		write(STDOUT_FILENO, "\nEntering foreground-only mode", 30);
		write(STDOUT_FILENO, " (& is now ignored)\n:", 21);
		fsync(STDOUT_FILENO);
		bg_permitted = 0;
	}
	else
	{
		write(STDOUT_FILENO, "\nExiting foreground-only mode\n:", 31);
		fsync(STDOUT_FILENO);
		bg_permitted = 1;
	}
}
