/*
    foregrounder a way to overcome daemon or software set that cannot work in foreground
    Copyright (C) 2021  Denis Corbin

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

int usage(char* argv0);
int action(char* start_cmd, char* stop_cmd, char** env, int die);
void sighandler(int sig);
void childhandler(int sig);

char* handler_cmd = NULL; /* command to run upon signal reception */
int trapped = 0;          /* set by sighandler */

int main(int argc, char* argv[], char** env)
{
    if(argc != 3
       || (argc == 4 && strcmp("die", argv[3]) != 0))
	return usage(argv[0]);
    else
	return action(argv[1], argv[2], env, argc == 4);
}

int usage(char* argv0)
{
    printf("%s: usage <start command> <stop command> [die]\n", argv0);
    printf("    run <start command>\n");
    printf("    then pause for any signal receiption\n");
    printf("    then run <stop command> and exits.\n");
    printf("    If \"die\" is added as last argument,\n");
    printf("    %s run the stop command and exits upon\n", argv0);
    printf("    child process death\n");
    printf("\nThe objective is to be able to cleanly stop\n");
    printf("a process running in a container while it cannot\n");
    printf("be run in foreground. Such process cannot become the\n");
    printf("process of the container that receives signals from\n");
    printf("from docker upon \"docker stop\". It would be killed\n");
    printf("abruptly after the timeout following the SIGTERM signal\n");
    printf("emission, and could not end gracefully\n");
    return 1;
}


int action(char* start_cmd, char* stop_cmd, char** env, int die)
{
    int signum[] = { SIGTERM, SIGINT, SIGHUP, SIGABRT, SIGUSR1, SIGUSR2, SIGQUIT, SIGPWR, /*!*/ SIGKILL /*!*/ };
	/*
	   here above SIGKILL is only used as end of list and must
	   stay the last element of the list, SIGKILL cannot be trapped
	*/
    int val = 0;
    int loop = 0;

	/* running the start command */
    printf("launching command %s\n", start_cmd);
    fflush(stdout);
    val = system(start_cmd);
    if(val != 0)
    {
	printf("failed to execute %s, aborting\n", start_cmd);
	return -1;
    }

	/* initializing the global variable for sighandler() */
    handler_cmd = stop_cmd;
    trapped = 0;

	/* setting signal handler for all listed signals */
    while(signum[loop] != SIGKILL)
    {
	signal(signum[loop], &sighandler);
	++loop;
    }

    signal(SIGCHLD, &childhandler);

	/* waiting for a signal to come */
    printf("command \"%s\" has completed. Now waiting for signal to run the \"%s\" command\n", start_cmd, stop_cmd);
    fflush(stdout);

    do
    {
	pause();

	    /* checking whether the handler has run the stop command */
	if(!trapped)
	    if(die)
	    {
		printf("signal received but not trapped, launching command \"%s\" right now\n", stop_cmd);
		fflush(stdout);
		sighandler(0); /* manually invoking the stop command */
	    }
	    else
	    {
		printf("Signal is not one of those expected one.\n\"die\" is not set, so we wait for another event\n");
		fflush(stdout);
	    }
	else
	{
	    printf("exiting loop\n");
	    fflush(stdout);
	}
    }
    while(!trapped && !die);

    printf("Command %s has completed, exiting\n", stop_cmd);

    return 0;
}

void sighandler(int sig)
{
    signal(SIGCHLD, SIG_IGN);
    printf("received signal %d, running the command \"%s\"\n", sig, handler_cmd);
    fflush(stdout);
    trapped = 1;
    system(handler_cmd);
}

void childhandler(int sig)
{
    int status;

    printf("a child process died\n");
    (void)wait(&status);
	/* we do not set the "trapped" variable */
}
