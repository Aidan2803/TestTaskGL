#include <iostream>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstring>
#include <ctime>
#include <csignal>

#define MICROSECONDS_IN_MILLISECOND 1000

using namespace std;

static pid_t childPid{0};	//To store child PID for signal handlers

/*Parent signal handler, which kills child process and then parent process*/
void signalParentHandler( int signumb ) {   
   kill(childPid, SIGTERM);	//Kills child process whith SIGTERM signal, 
   usleep(500000);		//Delay for correct output is stdout
   cout << "Parent process was terminated by silnal (" << signumb << ")\n";

   exit(signumb);  //Kills parent process
}

/*Child signal handler, which kills child process*/
void signalChildHandler( int signumb ) {
   cout << "Child process was terminated by silnal (" << signumb << ")\n";

   exit(signumb);  //kills child process
}

class ProcessManager{
public:
	ProcessManager(char* delayTimeFormMain);
	~ProcessManager();
	
	/*Delays process for given amount of milliseconds*/
	void delayMKS(int milliseconds);
	
	/*Process that happerns in child*/
	void childProcess();
	/*Process that happerns in parent*/
	void parentProcess();
	
	/*Loop where processes are divided with fork*/
	void mainLoop();	
	
private:
	int delayTime;	//Delay time from console
	
	pid_t pid;	//Pid for fork()
	
	int counter{0};	//Inner counter	
	
	char line[33];		//Buffer for pipe
	
	int nCP, fromChildToParent[2];	//Pipe		
};

ProcessManager::ProcessManager(char* delayTimeFormMain) : pid{0}, counter{0}
{		
	/*In constructor we accept delayTimeFormMain which is argv[1] - delay form console
	delayTimeTemp is used for verification of given argument, so if given argument < 1 or > 1000 we terminate process with error message*/
	int delayTimeTemp{0};
	delayTimeTemp = atoi(delayTimeFormMain);
	if(delayTimeTemp < 1 || delayTimeTemp > 1000){
		perror("Invalid delay time. Process will kill itself now");
		exit(-1);
	}
	/*If verification was succesfull assign delayTimeTemp to delayTime(which is private member of class)*/
	else{
		delayTime = delayTimeTemp;		
	}
}

//We have no dynamic memory allocated so destructor is empty
ProcessManager::~ProcessManager(){}

void ProcessManager::delayMKS(int milliseconds){
	/*We accept argument named milliseconds but in fact those are microseconds, so to turn them in millisecond we multiply argument by MICROSECONDS_IN_MILLISECOND
	Variable is called milliseconds instead of microseconds because user thinks that delay will be in milliseconds, which is true, but achived by multiplication*/
	int cClock = clock() + milliseconds * MICROSECONDS_IN_MILLISECOND;
	/*While clock() is smaller than cClock do nothing which is the delay*/
	while(clock() < cClock);
}

void ProcessManager::childProcess(){
	delayMKS(500);	//Delay for correct output in stdout
	cout << "Child: process created" << endl;			
	cout << "Child: PID = " << getpid() << endl;	//Out child PID, to see it from console and use kill on this PID
	childPid = getpid();		//Remember child PID for signal handler(in case parent will be killed)
			
	signal(SIGTERM, signalChildHandler);	//Connect function to signal handler
			
			
	/*In endless loop we increase counter by 1 and every time pass this value through pipe to parent process*/	
	while(1){					
		++counter;
		cout << "Child: Counter " << counter << endl;
			
		sprintf(line, "%d", counter);		//From int counter to char buffer for pipe		
		close (fromChildToParent [0]);	//Close read chanel of pipe
		write (fromChildToParent [1],  line, 33);	//Write buffer(counter in char form) into pipe
					
			 
		delayMKS(delayTime);		//Delays for time which was passed from console		
	}
			
}

void ProcessManager::parentProcess(){
	signal(SIGTERM, signalParentHandler);	//Connect function to signal handler
	cout << "Parent: PID " << getpid() << endl;	//Out parent PID, to see it from console and use kill on this PID		
						
	cout << "Parent: Waiting for child..." << endl;		
		 	
	while(1){	//We need loop to read all pipe till the last value
		 close (fromChildToParent [1]);	//Close write chanel of pipe	 		
		 nCP = read(fromChildToParent[0], line, 33);	//Read each line of pipe	 		
		 		
		 if(nCP != 33){	//When read will return -1 this means that we reached end of the pipe	 				 			
		 		break;
		 	}		 		
		 }		 			 		 		
		 counter = atoi(line);		/*If we reached end of pipe it means in line we have correct value of counter, using atoi to convert counter from char to int and store value in counter 							variable*/	
}

void ProcessManager::mainLoop(){
	while(1){	//Run following loop while parent is alive		
	     	if (pipe(fromChildToParent) == -1){	//Create pipe
	     		perror("Pipe: can not create pipe"); 
	     	}  
	     	switch(pid = fork()){	//Divide processes with fork()
			case -1:		//can not create child process
			cout << "Fork: Error, can not create child process" << endl;
			exit(1);
			case 0:		//child process
			this->childProcess();
			default:		//parent process
			this->parentProcess();		
		}
     	}
}

int main(int argc, char* argv[]){	

	/*Verification for correct amount of arguments*/
	if(argc == 1 || argc > 2){
		perror("Wrong amount of arguments, usage \"./processManager delayTime(1-1000)\", for example \"./processManager 1000\". Process will kill itself now");
		exit(-1);
	}	
	
	ProcessManager pm(argv[1]);	//Creating object of class
	pm.mainLoop();	//Starts main loop
	
	return 0;
}
