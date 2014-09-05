/* doit.cpp 
* A simple program for calling programs both through command line arguments and a simple shell
*/

#include <iostream>
#include <sstream>
using namespace std;
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <vector>
#include <signal.h>
#include <map>

int runCommand(char *args[]);
void getStats(double startin, int pid);
double timevalToMs (struct timeval time);
void runShell();
void printJobs();

//Struct to hold running total of statistics
struct rusage total;
//Structure to hold info on running processes
struct process {
	int pid;
	string title;
	double startTime;
	int num;
};
map <int, process> running;

/*
 * The process that gets called when a child end signal is recieved
 * Params:
 * 	num - the signal number (not used)
 */
void processComplete (int num){
	int pid, status;
	pid = waitpid(-1, &status, WNOHANG);
	if (!(running.find(pid) == running.end())){
		process info = running[pid];
		running.erase(pid);
		cout << "[" << info.num << "] " << pid << " completed\n";
		getStats(info.startTime, pid);
	}
}


int main (int argc, char *argv[]){
	//Set baseline value for usage totals
	getrusage(RUSAGE_CHILDREN, &total);

	//Setup listener for child termination
	struct sigaction sigchld_action; 
  	memset (&sigchld_action, 0, sizeof (sigchld_action)); 
  	sigchld_action.sa_handler = &processComplete; 
  	sigaction (SIGCHLD, &sigchld_action, NULL);

	if (argc > 1){
		//Handle running from arguments
		char *newargs[argc];
		for (int i = 1; i < argc; i++){
			newargs[i-1] = argv[i];
		}
		newargs[argc -1] = 0;
		runCommand(newargs);
		getStats(-1, 0);
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
		cin.clear();

		//Get line and convert to vector
		getline(cin, line);
		istringstream input (line);
		string word;
		vector<string> list;
		while(input >> word){
			list.push_back(word);
			if (word == "exit" && list.size() == 1){
				exit = true;
			}
		}

		if(exit){
			//Check if background processes are running
			if( running.size() == 0)
				break;
			else{
			
			cout << "There are processes running, please wait before exiting\n";
			exit = false;
			}
		}else if (list.size() < 1){
			//Do nothing
		}else if(list[0] == "cd"){
			//Change directory

			if (chdir(list[1].c_str()) < 0){
				cerr << "Error changing directory\n";
			}
		}else if(list[0] == "jobs"){

			printJobs();
		}else {
			//copy args
			char *newargs[list.size() + 1];
			for(int i = 0; i < (int)list.size(); i++){
				newargs[i] = (char *)list[i].c_str();
			} 
			newargs[list.size()] = 0;

			//remove & if it is in the list
			bool back = false;
			if(newargs[list.size() -1][0] ==  '&'){
				newargs[list.size() -1 ] = 0;
				back = true;
			}
			int pid = runCommand(newargs);
			if(back){
				//Setup task as background task
				struct timeval astart;
				gettimeofday(&astart, NULL);
				process p = {pid, newargs[0], timevalToMs(astart), running.size()+1};
				running[pid] = p;
				cout << "[" << running.size() << "] " << pid << "\n";
			}else{
				//wait and get stats
				getStats(-1, pid);
			}
		}
		list.clear();

	}
}

/*
 * Prints the currently running jobs to the console
 */
void printJobs(){
	typedef map<int, process>::iterator it_type;
	for (it_type it = running.begin(); it != running.end(); it++){
		process info = it->second;
		cout << "[" << info.num << "] " << info.pid << " " << info.title << "\n";
	}
}

/*
 * Finds and prints statistics of the completed child process
 * Params:
 * 	startin - the wall clock time the process started in ms, -1 if the function should wait for it to complete
 * 	pid - the pid of the process to wait on, -1 if not waiting
 */
void getStats(double startin, int pid = -1){
	struct rusage usage;
	struct timeval start, end;
	if( startin < 0){
		gettimeofday(&start, NULL);
		startin = timevalToMs(start);
		int status;
		waitpid(pid, &status, 0 );	//Wait for child to complete
	}
	gettimeofday(&end, NULL);
	if(getrusage(RUSAGE_CHILDREN, &usage) != 0){
        	cerr << "Error getting usage\n";
        }
	double utime = timevalToMs(usage.ru_utime) - timevalToMs(total.ru_utime);
	double stime = timevalToMs(usage.ru_stime) - timevalToMs(total.ru_stime);
	double wtime = timevalToMs(end) - startin;

	//Print statistics
	cout << "\n--Statistics--\n";
	cout << "Wall time : " << wtime << "ms\n";
        cout << "User Time: " << utime  << "ms\n";
	cout << "System Time : " << stime << "ms\n";
	cout << "Involuntarily preempted " << usage.ru_nivcsw - total.ru_nivcsw << " times\n";
	cout << "Voluntarily preempted " << usage.ru_nvcsw - total.ru_nvcsw << " times\n";
	cout << "Page faults : " << usage.ru_majflt  - total.ru_majflt << "\n";
	cout << "Page faults satisfied by memory reclaim: " << usage.ru_minflt - total.ru_minflt << "\n";
	
	//Set total of all completed tasks
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
int runCommand(char *args[]){
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
		return pid;	
	}	
}
