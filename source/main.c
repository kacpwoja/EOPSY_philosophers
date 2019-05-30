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

// Semaphore operations
struct sembuf down = {0, -1, SEM_UNDO};
struct sembuf up = {0, +1, SEM_UNDO};

int main(int argc, char** argv)
{
	// Shared variables (pipes)
	int times_eaten[N][2];
	int philo_states[N][2];
	int return_status;
	for(int i=0; i<N; i++)
	{
		return_status = pipe(times_eaten[i]);
		if(return_status == -1)
		{
			fprintf(stderr, "Failed to create pipe %d,eat. Exiting.\n", i);
			return 1;
		}
		return_states = pipe(philo_states[N][2]);
		if(return_status == -1)
		{
			fprintf(stderr, "Failed to create pipe %d,state. Exiting.\n", i);
			return 1;
		}
	}
	
	// semun definition
	union semun {
		int val;
		struct semid_ds *buf;
		ushort *array;
	} arg;

	// Semaphores
	int forks_sem;
	int state_sem;

	// Creating semaphores
	forks_sem = semget(FORKKEY, N, 0640 | IPC_CREAT);
	if(forks_sem < 0)
	{
		fprintf(stderr, "Failed to create semaphore with key %d. Exiting.\n", FORKKEY);
		return 1;
	}
	state_sem = semget(CRITKEY, 1, 0640 | IPC_CREAT);
	if(state_sem < 0)
	{
		fprintf(stderr, "Failed to create semaphore with key %d. Exiting.\n", FORKKEY);
		return 1;
	}
	//Setting semval to 1
	arg.val = 1;
	if(semctl(state_sem, 0, SETVAL, arg)  < 0)
	{
		fprintf(stderr, "SEMCTL ERROR (state semaphore). Exiting.");
		return 1;
	}
	if(semctl(forks_sem, N, SETVAL, arg) < 0)
	{
		fprintf(stderr, "SEMCTL ERROR (forks semaphores). Exiting.");
		return 1;
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
	if(semop(state_sem, &down, 1) < 0)
	{
		fprintf(stderr, "SEMOP ERROR\n");
	}
	for(int i=0; i<N; i++)
	{
		printf("Philosopher %d ate %d times\n", i, times_eaten[i]);
	}
	if(semop(state_sem, &up, 1) < 0)
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
	int state_sem = semget(CRITKEY, 1, 0640);
	if(state_sem < 0)
	{
		fprintf(stderr, "Failed to create semaphore with key %d. Exiting.\n", FORKKEY);
	}
	if(semop(state_sem, &down, 1) < 0)
	{
		fprintf(stderr, "SEMOP ERROR\n");
	}
	philo_states[i] = THINKING;
	if(semop(state_sem, &up, 1) < 0)
	{
		fprintf(stderr, "SEMOP ERROR\n");
	}
	printf("Philosopher %d is thinking...\n", i);
	sleep(THINK);
}

void hungry(int i)
{
	int state_sem = semget(CRITKEY, 1, 0666);
	if(state_sem < 0)
	{
		fprintf(stderr, "Failed to create semaphore with key %d. Exiting.\n", FORKKEY);
	}
	if(semop(state_sem, &down, 1) < 0)
	{
		fprintf(stderr, "SEMOP ERROR\n");
	}
	philo_states[i] = HUNGRY;
	if(semop(state_sem, &up, 1) < 0)
	{
		fprintf(stderr, "SEMOP ERROR\n");
	}
	printf("Philosopher %d is hungry.\n", i);
}

void eat(int i)
{
	int state_sem = semget(CRITKEY, 1, 0666);
	if(state_sem < 0)
	{
		fprintf(stderr, "Failed to create semaphore with key %d. Exiting.\n", FORKKEY);
	}
	if(semop(state_sem, &down, 1) < 0)
	{
		fprintf(stderr, "SEMOP ERROR\n");
	}
	philo_states[i] = EATING;
	++times_eaten[i];
	// printf("DDD TIMES %d %d\n", i, times_eaten[i]);
	if(semop(state_sem, &up, 1) < 0)
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
