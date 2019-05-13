#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/sem.h>

// Number of Philosophers
#define N 5
// Time it takes to think (in seconds)
#define THINK 5
// Total runtime of the program in seconds
#define RUNTIME 30

void philosopher(int i);
void think(int i);

int main(int argc, char** argv)
{
	// Storing PIDs
	pid_t philo_pids[N];
	int philo_count = 0;
	pid_t pid;
	
	// Creating processes
	for(int i=0; i<N; i++)
	{
		pid = fork();
		// Fork failed
		if(pid < 0)
		{
			fprintf(stderr, "Fork failed. Terminating processes and exiting.\n");
			for( int j=0; j<philo_count; j++)
			{
				kill(philo_pids[j], SIGTERM);
			}
			return 1;
		}
	
		// Parent process
		if(pid > 0)
		{
			philo_pids[i] = pid;
			++philo_count;
		}
		
		// Child process (philosopher)
		if(pid == 0)
		{
			philosopher(i);
			return 0;
		}
	}
	
	// Waiting to finish
	sleep(RUNTIME);
	printf("Runtime over: terminating processes and exiting.\n");
	for(int i=0; i<N; i++)
	{
		kill(philo_pids[i], SIGTERM);
	}
	
	return 0;
}

void philosopher(int i)
{
	while(1)
	{
		think(i);
	}
}

void think(int i)
{
	printf("Philosopher %d: Thinking...\n", i);
	sleep(THINK);
}
