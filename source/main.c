#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/sem.h>

// Number of Philosophers
#define N 5
// Time it takes to think and eat (in seconds)
#define THINK 2
#define EAT 1
// Total runtime of the program in seconds
#define RUNTIME 30

// Semaphore Keys
#define FORKKEY 0x1111
#define CRITKEY 0x1110

// Defines for readibilty
#define LEFT (i-1+N)%N
#define RIGHT (i+1)%N
#define THINKING 0
#define HUNGRY 1
#define EATING 2

void philosopher(int i);
void think(int i);
void hungry(int i);
void eat(int i);

void grab_forks(int left_fork_id);
void put_away_forks(int left_fork_id);

// Shared variables
int times_eaten[N];
int philo_states[N];

// Semaphore operations
struct sembuf down = {0, -1, SEM_UNDO};
struct sembuf up = {0, +1, SEM_UNDO};

int main(int argc, char** argv)
{
	// Semaphores
	int forks_sem;
	int critical;

	// Creating semaphores
	forks_sem = semget(FORKKEY, N, 0666 | IPC_CREAT);
	if(forks_sem < 0)
	{
		fprintf(stderr, "Failed to create semaphore with key %d. Exiting.\n", FORKKEY);
		return 1;
	}
	critical = semget(CRITKEY, 1, 0666 | IPC_CREAT);
	if(critical < 0)
	{
		fprintf(stderr, "Failed to create semaphore with key %d. Exiting.\n", FORKKEY);
		return 1;
	}
	//Setting semval to 1
	if(semop(critical, &up, 1) < 0)
	{
		fprintf(stderr, "SEMOP ERROR\n");
	}

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
	
	// Printing summary
	if(semop(critical, &down, 1) < 0)
	{
		fprintf(stderr, "SEMOP ERROR\n");
	}
	for(int i=0; i<N; i++)
	{
		printf("Philosopher %d ate %d times\n", i, times_eaten[i]);
	}
	if(semop(critical, &up, 1) < 0)
	{
		fprintf(stderr, "SEMOP ERROR\n");
	}
	
	
	return 0;
}

void philosopher(int i)
{
	while(1)
	{
		think(i);
		hungry(i);
		grab_forks(i);
		eat(i);
		put_away_forks(i);
	}
}

void think(int i)
{
	int critical = semget(CRITKEY, 1, 0666);
	if(critical < 0)
	{
		fprintf(stderr, "Failed to create semaphore with key %d. Exiting.\n", FORKKEY);
	}
	if(semop(critical, &down, 1) < 0)
	{
		fprintf(stderr, "SEMOP ERROR\n");
	}
	philo_states[i] = THINKING;
	if(semop(critical, &up, 1) < 0)
	{
		fprintf(stderr, "SEMOP ERROR\n");
	}
	printf("Philosopher %d is thinking...\n", i);
	sleep(THINK);
}

void hungry(int i)
{
	int critical = semget(CRITKEY, 1, 0666);
	if(critical < 0)
	{
		fprintf(stderr, "Failed to create semaphore with key %d. Exiting.\n", FORKKEY);
	}
	if(semop(critical, &down, 1) < 0)
	{
		fprintf(stderr, "SEMOP ERROR\n");
	}
	philo_states[i] = HUNGRY;
	if(semop(critical, &up, 1) < 0)
	{
		fprintf(stderr, "SEMOP ERROR\n");
	}
	printf("Philosopher %d is hungry.\n", i);
}

void eat(int i)
{
	int critical = semget(CRITKEY, 1, 0666);
	if(critical < 0)
	{
		fprintf(stderr, "Failed to create semaphore with key %d. Exiting.\n", FORKKEY);
	}
	if(semop(critical, &down, 1) < 0)
	{
		fprintf(stderr, "SEMOP ERROR\n");
	}
	philo_states[i] = EATING;
	++times_eaten[i];
	printf("DDD TIMES %d %d\n", i, times_eaten[i]);
	if(semop(critical, &up, 1) < 0)
	{
		fprintf(stderr, "SEMOP ERROR\n");
	}
	printf("Philosopher %d is eating.\n", i);
	sleep(EAT);
}

void grab_forks(int left_fork_id)
{

}
void put_away_forks(int left_fork_id)
{

}
