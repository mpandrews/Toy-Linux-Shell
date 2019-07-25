#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

extern sig_atomic_t bg_permitted;

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
		bg_permitted = 1;
	}
}
