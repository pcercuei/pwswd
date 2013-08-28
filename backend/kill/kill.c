
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "../backends.h"

#define TTY1 "/dev/tty1"

void do_kill(void)
{
	int status;
	pid_t son = fork();
	if (!son)
		execlp("fuser", "fuser", "-k", "-HUP", FRAMEBUFFER, NULL);

	waitpid(son, &status, 0);
	if (!status)
		return;

	/* Perhaps the framebuffer was not in use;
	 * try to kill a console-based application. */
	son = fork();
	if (!son)
		execlp("fuser", "fuser", "-k", "-HUP", TTY1, NULL);

	waitpid(son, &status, 0);
}
