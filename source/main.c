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

void grab_forks(int left_fork_id);
void put_away_forks(int left_fork_id);
void test(int left_fork_id);

// Shared variables (pipe IDs)
int times_eaten[N][2];
int philo_states[N][2];

// Semaphore operations
struct sembuf down = {0, -1, SEM_UNDO};
struct sembuf up = {0, +1, SEM_UNDO};

int main(int argc, char** argv)
{
	// Getting pipe IDs
	int return_status;
	for(int i=0; i<N; i++)
	{
		return_status = pipe(times_eaten[i]);
		if(return_status == -1)
		{
			fprintf(stderr, "Failed to create pipe %d,eat. Exiting.\n", i);
			return 1;
		}
		// Initiate to 0
		write(times_eaten[i][1], 0, sizeof(int));

		return_status = pipe(philo_states[N]);
		if(return_status == -1)
		{
			fprintf(stderr, "Failed to create pipe %d,state. Exiting.\n", i);
			return 1;
		}
		// Initiate to THINKING (0)
		write(philo_states[i][1], (int*)THINKING, sizeof(int));
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
	// Semaphore init
	arg.val = 1;
	if(semctl(state_sem, 0, SETVAL, arg)  < 0)
	{
		fprintf(stderr, "SEMCTL ERROR (state semaphore). Exiting.");
		return 1;
	}
	ushort zeros[N];
	for(int i=0; i<N; i++)
		zeros[i] = 0;
	arg.array = zeros;
	if(semctl(forks_sem, N, SETALL, arg) < 0)
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
	if(semop(state_sem, &down, 1) < 0)
	{
		fprintf(stderr, "SEMOP ERROR\n");
	}
	for(int i=0; i<N; i++)
	{
		int eaten;
		read(times_eaten[i][0], &eaten, sizeof(int));
		printf("Philosopher %d ate %d times\n", i, eaten);
	}
	if(semop(state_sem, &up, 1) < 0)
	{
		fprintf(stderr, "SEMOP ERROR\n");
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
	forks_sem = semget(FORKKEY, N, 0640);
	if(forks_sem < 0)
	{
		fprintf(stderr, "Failed to get semaphore with key %d. Exiting.\n", FORKKEY);
	}
	state_sem = semget(CRITKEY, 1, 0640);
	if(state_sem < 0)
	{
		fprintf(stderr, "Failed to get semaphore with key %d. Exiting.\n", FORKKEY);
	}

	// Enter critical section
	if(semop(state_sem, &down, 1) < 0)
	{
		fprintf(stderr, "Semop error\n");
	}

	// Become hungry
	write(philo_states[i][1], (int*)HUNGRY, sizeof(int));
	printf("Philosopher %d is hungry and trying to pick up forks.\n", i);
	
	// Test for forks
	test(i);

	// Leave critical section
	if(semop(state_sem, &up, 1) < 0)
	{
		fprintf(stderr, "Semop error\n");
	}

	// Take the forks
	struct sembuf i_down = {i, -1, SEM_UNDO};
	if(semop(forks_sem, &i_down, 1) < 0)
	{
		fprintf(stderr, "Semop error\n");
	}
}
void put_away_forks(int left_fork_id)
{
	int i = left_fork_id;
	// Semaphores
	int forks_sem;
	int state_sem;

	// Creating semaphores
	forks_sem = semget(FORKKEY, N, 0640);
	if(forks_sem < 0)
	{
		fprintf(stderr, "Failed to get semaphore with key %d. Exiting.\n", FORKKEY);
	}
	state_sem = semget(CRITKEY, 1, 0640);
	if(state_sem < 0)
	{
		fprintf(stderr, "Failed to get semaphore with key %d. Exiting.\n", FORKKEY);
	}

	// Enter critical section
	if(semop(state_sem, &down, 1) < 0)
	{
		fprintf(stderr, "Semop error\n");
	}

	// Finish eating - increase count
	printf("Philosopher %d has finished his meal. He is putting down the forks\n", i);
	int eaten;
	read(times_eaten[i][0], &eaten, sizeof(int));
	++eaten;
	write(times_eaten[i][1], &eaten, sizeof(int));

	// Start thinking
	write(philo_states[i][1], (int*)THINKING, sizeof(int));
	
	// Let neighbours eat
	test(LEFT);
	test(RIGHT);

	// Leave critical section
	if(semop(state_sem, &up, 1) < 0)
	{
		fprintf(stderr, "Semop error\n");
	}
}

void test(int left_fork_id)
{
	int i = left_fork_id;
	// Semaphores
	int forks_sem;
	int state_sem;

	// Creating semaphores
	forks_sem = semget(FORKKEY, N, 0640);
	if(forks_sem < 0)
	{
		fprintf(stderr, "Failed to get semaphore with key %d. Exiting.\n", FORKKEY);
	}
	state_sem = semget(CRITKEY, 1, 0640);
	if(state_sem < 0)
	{
		fprintf(stderr, "Failed to get semaphore with key %d. Exiting.\n", FORKKEY);
	}

	// Read states (already in critical section)
	int i_state;
	int l_state;
	int r_state;

	read(philo_states[i][0], &i_state, sizeof(int));
	read(philo_states[LEFT][0], &l_state, sizeof(int));
	read(philo_states[RIGHT][0], &r_state, sizeof(int));

	// Check neighbours
	if(i_state == HUNGRY && l_state != EATING && r_state != EATING)
	{
		// Begin eating and allow to take forks
		write(philo_states[i][1], (int*)EATING, sizeof(int));
		
		struct sembuf i_up = {i, +1, SEM_UNDO};
		if(semop(forks_sem, &i_up, 1) < 0)
		{
			fprintf(stderr, "Semop error\n");
		}
	}
}
