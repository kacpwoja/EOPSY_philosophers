#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <stdlib.h>
#include <sys/shm.h>

// Number of Philosophers
#define N 5
// Time it takes to think and eat (in seconds)
#define THINK 2
#define EAT 1
// Total runtime of the program in seconds
#define RUNTIME 30

// Semaphore Keys
#define FORKKEY 0x1111
#define CRITKEY 0x1234
// Memory Key
#define SHMKEY 0x1998

// Defines for readibilty
#define LEFT (i-1+N)%N
#define RIGHT (i+1)%N
#define THINKING 0
#define HUNGRY 1
#define EATING 2

void grab_forks(int left_fork_id);
void put_away_forks(int left_fork_id);
void test(int left_fork_id);

// Shared memory
struct shared_memory {
	int times_eaten[N];
	int philo_states[N];
};

// Semaphore operations
struct sembuf down = {0, -1, SEM_UNDO};
struct sembuf up = {0, +1, SEM_UNDO};

int main(int argc, char** argv)
{
	// Get shared memory
	int shmid = shmget(SHMKEY, sizeof(struct shared_memory), 0660 | IPC_CREAT);
	if(shmid < 0)
	{
		perror("SHMGET error\n");
		exit(1);
	}
	// Attach
	struct shared_memory* shared;
	shared = shmat(shmid, NULL, 0);
	if(shared == (void*) -1)
	{
		perror("SHMAT error\n");
		exit(1);
	}
	// Initialize to 0 and THINKING
	for(int i=0; i<N; i++)
	{
		shared->times_eaten[i] = 0;
		shared->philo_states[i] = THINKING;
	}

	// semun definition
	union semun {
		int val;
		ushort *array;
	} arg;

	// Semaphores
	int forks_sem;
	int state_sem;

	// Creating semaphores
	forks_sem = semget(FORKKEY, N, 0660 | IPC_CREAT);
	if(forks_sem < 0)
	{
		perror("SEMGET error");
		exit(1);
	}
	state_sem = semget(CRITKEY, 1, 0660 | IPC_CREAT);
	if(state_sem < 0)
	{
		perror("SEMGET error");
		exit(1);
	}
	// Semaphore init
	arg.val = 1;
	if(semctl(state_sem, 0, SETVAL, arg)  < 0)
	{
		perror("SEMCTL ERROR (state semaphore). Exiting.");
		exit(1);
	}
	ushort zeros[N];
	for(int i=0; i<N; i++)
		zeros[i] = 0;
	arg.array = zeros;
	if(semctl(forks_sem, N, SETALL, arg) < 0)
	{
		perror("SEMCTL ERROR (forks semaphores). Exiting.");
		exit(1);
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
			perror("Fork failed. Terminating processes and exiting.\n");
			for( int j=0; j<philo_count; j++)
			{
				kill(philo_pids[j], SIGTERM);
			}
			exit(1);
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
			while(1)
			{
				// Thinking
				printf("Philosopher %d is thinking...\n", i);
				sleep(THINK);
				// Hungry, trying to eat
				grab_forks(i);
				// Got forks, beginning to eat
				printf("Philosopher %d is eating.\n", i);
				sleep(EAT);
				// Eating finished, putting forks back and thinking
				put_away_forks(i);
			}
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
	/*
	if(semop(state_sem, &down, 1) < 0)
	{
		perror("SEMOP ERROR\n");
		exit(1);
	}
	*/
	for(int i=0; i<N; i++)
	{
		printf("Philosopher %d ate %d times\n", i, shared->times_eaten[i]);
	}
	/*
	if(semop(state_sem, &up, 1) < 0)
	{
		perror("SEMOP ERROR\n");
		exit(1);
	}
	*/

	// Detach memory
	if(shmdt(shared) < 0)
	{
		perror("SHMDT error\n");
		exit(1);
	}
	// Remove memory
	if(shmctl(shmid, IPC_RMID, 0) < 0)
	{
		perror("SHMCTL error\n");
		exit(1);
	}
	
	return 0;
}

void grab_forks(int left_fork_id)
{
	int i = left_fork_id;
	// Semaphores
	int forks_sem;
	int state_sem;

	// Getting semaphores
	forks_sem = semget(FORKKEY, N, 0660);
	if(forks_sem < 0)
	{
		perror("SEMGET error");
		exit(1);
	}
	state_sem = semget(CRITKEY, 1, 0660);
	if(state_sem < 0)
	{
		perror("SEMGET error");
		exit(1);	
	}
	// Get shared memory
	int shmid = shmget(SHMKEY, sizeof(struct shared_memory), 0660);
	if(shmid < 0)
	{
		perror("SHMGET error\n");
		exit(1);
	}
	// Attach
	struct shared_memory* shared;
	shared = shmat(shmid, NULL, 0);
	if(shared == (void*) -1)
	{
		perror("SHMAT error\n");
		exit(1);
	}

	// Enter critical section
	if(semop(state_sem, &down, 1) < 0)
	{
		perror("Semop error\n");
		exit(1);
	}

	// Become hungry
	shared->philo_states[i] = HUNGRY;
	printf("Philosopher %d is hungry and trying to pick up forks.\n", i);
	
	// Test for forks
	test(i);

	// Leave critical section
	if(semop(state_sem, &up, 1) < 0)
	{
		perror("Semop error\n");
		exit(1);
	}
	// Take the forks
	struct sembuf i_down = {i, -1, SEM_UNDO};
	if(semop(forks_sem, &i_down, 1) < 0)
	{
		perror("Semop error\n");
		exit(1);
	}

	// Detach memory
	if(shmdt(shared) < 0)
	{
		perror("SHMDT error\n");
		exit(1);
	}
	return;
}
void put_away_forks(int left_fork_id)
{
	int i = left_fork_id;
	// Semaphores
	int forks_sem;
	int state_sem;

	// Creating semaphores
	forks_sem = semget(FORKKEY, N, 0660);
	if(forks_sem < 0)
	{
		perror("SEMGET error");
		exit(1);
	}
	state_sem = semget(CRITKEY, 1, 0660);
	if(state_sem < 0)
	{
		perror("SEMGET error");
		exit(1);
	}
	// Get shared memory
	int shmid = shmget(SHMKEY, sizeof(struct shared_memory), 0660);
	if(shmid < 0)
	{
		perror("SHMGET error\n");
		exit(1);
	}
	// Attach
	struct shared_memory* shared;
	shared = shmat(shmid, NULL, 0);
	if(shared == (void*) -1)
	{
		perror("SHMAT error\n");
		exit(1);
	}

	// Enter critical section
	if(semop(state_sem, &down, 1) < 0)
	{
		perror("Semop error\n");
		exit(1);
	}

	// Finish eating - increase count
	printf("Philosopher %d has finished his meal. He is putting down the forks\n", i);
	++shared->times_eaten[i];

	// Start thinking
	shared->philo_states[i] = THINKING;
	
	// Let neighbours eat
	test(LEFT);
	test(RIGHT);

	// Leave critical section
	if(semop(state_sem, &up, 1) < 0)
	{
		perror("Semop error\n");
		exit(1);
	}

	// Detach memory
	if(shmdt(shared) < 0)
	{
		perror("SHMDT error\n");
		exit(1);
	}
	return;
}

void test(int left_fork_id)
{
	int i = left_fork_id;
	// Semaphores
	int forks_sem;
	int state_sem;

	// Creating semaphores
	forks_sem = semget(FORKKEY, N, 0660);
	if(forks_sem < 0)
	{
		perror("SEMGET error");
		exit(1);
	}
	state_sem = semget(CRITKEY, 1, 0660);
	if(state_sem < 0)
	{
		perror("SEMGET error");
		exit(1);
	}
	// Get shared memory
	int shmid = shmget(SHMKEY, sizeof(struct shared_memory), 0660);
	if(shmid < 0)
	{
		perror("SHMGET error\n");
		exit(1);
	}
	// Attach
	struct shared_memory* shared;
	shared = shmat(shmid, NULL, 0);
	if(shared == (void*) -1)
	{
		perror("SHMAT error\n");
		exit(1);
	}
	// Check neighbours (already in critical section)
	if(shared->philo_states[i] == HUNGRY &&
	   shared->philo_states[LEFT] != EATING &&
	   shared->philo_states[RIGHT] != EATING)
	{
		// Begin eating and allow to take forks
		shared->philo_states[i] = EATING;
		struct sembuf i_up = {i, +1, SEM_UNDO};
		if(semop(forks_sem, &i_up, 1) < 0)
		{
			perror("Semop error\n");
			exit(1);
		}
	}

	// Detach memory
	if(shmdt(shared) < 0)
	{
		perror("SHMDT error\n");
		exit(1);
	}
	return;
}
