#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

extern sig_atomic_t bg_permitted;
extern sig_atomic_t fg_active;

void sigtstp_handler(int signal)
{
}
