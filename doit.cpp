/* doit.cpp */

#include <iostream>
using namespace std;
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

void runCommand(char *args[]);

int main (int argc, char *argv[]){
	if (argc > 1){	//temporary
		char *newargs[argc];
		for (int i = 1; i < argc; i++){
			newargs[i-1] = argv[i];
		}
		newargs[argc -1] = 0;
		runCommand(newargs);
		wait(0);
	}
	cout << "done\n";
	exit(0);
}

/*
 * Forks a new process to run a bash command
 * Params:
 * 	args - the command line arguments
 */
void runCommand(char *args[]){
	int pid;
	if ((pid = fork()) < 0){	//error
		cerr << "Error forking process\n";
		exit(1);
	}
	else if (pid == 0){	//is child process
		execvp(args[0], args);
		cerr << "Error with execvp\n";
		exit(1);
	}
	else{	//is parent process
	
	}	
}
