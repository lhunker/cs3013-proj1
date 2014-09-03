/* doit.cpp */

#include <iostream>
#include <sstream>
using namespace std;
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <vector>

void runCommand(char *args[]);
void getStats();
double timevalToMs (struct timeval time);
void runShell();

//Struct to hold running total of statistics
struct rusage total;

int main (int argc, char *argv[]){
	getrusage(RUSAGE_CHILDREN, &total);
	if (argc > 1){
		char *newargs[argc];
		for (int i = 1; i < argc; i++){
			newargs[i-1] = argv[i];
		}
		newargs[argc -1] = 0;
		runCommand(newargs);
		getStats();
	}else{
		runShell();
	}
	exit(0);
}

/*
 * Runs the program in command shell mode, listening for commands until the user exits
 */
void runShell(){
	bool exit = false;
	while (!exit) {
		vector<string> args;
		cout << "==>" ;
		string line;
		getline(cin, line);
		istringstream input (line);
		string word;
		vector<string> list;
		while(input >> word){
			//TODO check for eof
			list.push_back(word);
			if (word == "exit"){
				exit = true;
			}
		}

		if(exit){
			break;
		}else if (list.size() < 1){
			//Do nothing
		}else if(list[0] == "cd"){
			//Change directory

			if (chdir(list[1].c_str()) < 0){
				cerr << "Error changing directory\n";
			}
		}else {
			//Child process, run command

			//copy args
			char *newargs[list.size() + 1];
			for(int i = 0; i < (int)list.size(); i++){
				newargs[i] = (char *)list[i].c_str();
			} 
			newargs[list.size()] = 0;
			runCommand(newargs);
			getStats();
		}
		list.clear();
	}
}

/*
 * Finds and prints statistics of the completed child process
 */
void getStats(){
	struct rusage usage;
	struct timeval start, end;
	gettimeofday(&start, NULL);
	wait(0);	//Wait for child to complete
	gettimeofday(&end, NULL);
	if(getrusage(RUSAGE_CHILDREN, &usage) != 0){
        	cerr << "Error getting usage\n";
        }
	double utime = timevalToMs(usage.ru_utime) - timevalToMs(total.ru_utime);
	double stime = timevalToMs(usage.ru_stime) - timevalToMs(total.ru_stime);
	double wtime = timevalToMs(end) - timevalToMs(start);
	cout << "\n--Statistics--\n";
	cout << "Wall time : " << wtime << "ms\n";
        cout << "User Time: " << utime  << "ms\n";
	cout << "System Time : " << stime << "ms\n";
	cout << "Involuntarily preempted " << usage.ru_nivcsw - total.ru_nivcsw << " times\n";
	cout << "Voluntarily preempted " << usage.ru_nvcsw - total.ru_nvcsw << " times\n";
	cout << "Page faults : " << usage.ru_majflt  - total.ru_majflt << "\n";
	cout << "Page faults satisfied by memory reclaim: " << usage.ru_minflt << "\n";
	total = usage;
}


/*
 * Converts a timeval struct to milliseconds
 * 
 * Params:
 * 	time - the time to be converted
 *
 * Returns:
 * 	The timeval in milliseconds as a double
 */
double timevalToMs(struct timeval time){
	return time.tv_sec * 1000 + (time.tv_usec/1000.0);
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
