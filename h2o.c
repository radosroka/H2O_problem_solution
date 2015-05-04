//Author : Radovan Sroka 1BIB
//Second project to Operating systems
//Problem : Building H2o

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <semaphore.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <signal.h>
#include <time.h>
#include <stdarg.h> 

FILE * output;

enum {
	N, GH, GO, B
};
long params[4];

//******************************************//

#define SEM_COUNT 10

sem_t *semaphores;
int sem_id;

enum {
	FILE_MUTEX, SHARED_MUTEX, T1, T2, OX, H, TA, BOND_LOCK, LOCK, ABC
};

//*******************************************//

#define SHARED_COUNTERS 6

enum{
	OPER_COUNTER, B3_COUNTER, OX_COUNTER, H_COUNTER, BOND_COUNTER, BOND_END
};

long *shared_counters;
int shared_id;

//******************************************//

void init(){					// iniciazition function for semaphores, shared memory and others variables
	srandom(time(0));
	setbuf(stdout, NULL);

	if ((output = fopen("h2o.out", "w")) == NULL){
		perror("h2o.out");
		exit(2);
	}

	if (( sem_id = shmget(IPC_PRIVATE, sizeof(sem_t) * SEM_COUNT, IPC_CREAT | 0666)) == -1){ 
		perror("Alocation of semaphores");
		goto file;
	}

	if (( semaphores = (sem_t *) shmat(sem_id, NULL, 0)) == NULL){ 
		perror("Attach shared memory");
		goto file;
	}

	if (( shared_id = shmget(IPC_PRIVATE, sizeof(long) * SHARED_COUNTERS, IPC_CREAT | 0666)) == -1){ 
		perror("Alocation of shared memory");
		goto sem;
	}

	if (( shared_counters = (long *) shmat(shared_id, NULL, 0)) == NULL){ 
		perror("Attach shared memory");
		goto sem;
	}

	if ( sem_init(&semaphores[FILE_MUTEX], 1, 1) == -1){
		perror("semaphore_init");
		goto shm;
	}    

	if ( sem_init(&semaphores[SHARED_MUTEX], 1, 1) == -1){
		perror("semaphore_init");
		goto f_m;
	}

	if ( sem_init(&semaphores[T1], 1, 0) == -1){
		perror("semaphore_init");
		goto sem_shmutex;
	}    

	if ( sem_init(&semaphores[T2], 1, 0) == -1){
		perror("semaphore_init");
		goto sem_turn1;
	}

	if ( sem_init(&semaphores[OX], 1, 0) == -1){
		perror("semaphore_init");
		goto sem_turn2;
	}    

	if ( sem_init(&semaphores[H], 1, 0) == -1){
		perror("semaphore_init");
		goto sem_ox;
	}

	if ( sem_init(&semaphores[TA], 1, 0) == -1){
		perror("semaphore_init");
		goto sem_hq;
	}   

	if ( sem_init(&semaphores[BOND_LOCK], 1, 3) == -1){
		perror("semaphore_init");
		goto sem_ta;
	}   

	if ( sem_init(&semaphores[LOCK], 1, 1) == -1){
		perror("semaphore_init");
		goto sem_BOND_LOCK;
	}  

	if ( sem_init(&semaphores[ABC], 1, 1) == -1){
		perror("semaphore_init");
		goto sem_lock;
	} 

	for (int i = 0; i < SHARED_COUNTERS; i++){
		shared_counters[i] = 0;
	}
	shared_counters[OPER_COUNTER] = 1;

	return;  
	sem_lock:			sem_destroy(&semaphores[LOCK]);

	sem_BOND_LOCK:		sem_destroy(&semaphores[BOND_LOCK]);

	sem_ta:			sem_destroy(&semaphores[TA]);

	sem_hq:			sem_destroy(&semaphores[H]);

	sem_ox:		sem_destroy(&semaphores[OX]);

	sem_turn2:		sem_destroy(&semaphores[T2]);

	sem_turn1:		sem_destroy(&semaphores[T1]);

	sem_shmutex:	sem_destroy(&semaphores[SHARED_MUTEX]);

	f_m: 	sem_destroy(&semaphores[FILE_MUTEX]);

	shm:	shmctl( shared_id, IPC_RMID, NULL);

	sem:	shmctl(sem_id, IPC_RMID, NULL);

	file:	fclose(output);

	exit(2);
}

void deinit(){				//dinicialization, frees all resources

	for (int i = 0; i < SEM_COUNT; i++){
		sem_destroy(&semaphores[i]);
	}
	shmctl( shared_id, IPC_RMID, NULL);
	shmctl(sem_id, IPC_RMID, NULL);
	fclose(output);
}

void print_mutex_to_file(long oper_counter, const char * fmt, ...){   //mutual exclusion when something is writing to file
	va_list arguments;
	va_start (arguments, fmt);
	sem_wait(&semaphores[FILE_MUTEX]);
	fflush(0);
	fprintf(output, "%ld\t: ", oper_counter);
	vfprintf(output, fmt, arguments);
	fflush(0);
	sem_post(&semaphores[FILE_MUTEX]);
	va_end(arguments);
}


void barrier3(){    // barrier fo 3 precesses

	sem_wait(&semaphores[SHARED_MUTEX]);
	shared_counters[B3_COUNTER]++;
	if (shared_counters[B3_COUNTER] == 3){
		sem_post(&semaphores[T1]);
		sem_post(&semaphores[T1]);
		sem_post(&semaphores[T1]);
		}

	sem_post(&semaphores[SHARED_MUTEX]);
	sem_wait(&semaphores[T1]);

	sem_wait(&semaphores[SHARED_MUTEX]);
	shared_counters[B3_COUNTER]--;
	if (shared_counters[B3_COUNTER] == 0){
		sem_post(&semaphores[T2]);
		sem_post(&semaphores[T2]);
		sem_post(&semaphores[T2]);
	}
	sem_post(&semaphores[SHARED_MUTEX]);
	sem_wait(&semaphores[T2]);
}

void barrier_for_all(){   //barrier for all processes (end waiting)

	sem_wait(&semaphores[SHARED_MUTEX]);
	shared_counters[BOND_COUNTER]++;
		if (shared_counters[BOND_COUNTER] == 3*params[N])
			sem_post(&semaphores[TA]);
	sem_post(&semaphores[SHARED_MUTEX]);

	sem_wait(&semaphores[TA]);
	sem_post(&semaphores[TA]);	
}


void bond(char name, long number){ //bonded function
	
	barrier3();

	sem_wait(&semaphores[SHARED_MUTEX]);
	print_mutex_to_file(shared_counters[OPER_COUNTER]++, "%c %ld\t: begin bonding\n", name, number);
	sem_post(&semaphores[SHARED_MUTEX]);

	usleep(rand()%(params[B]+1));
	
	barrier3();

	sem_wait(&semaphores[SHARED_MUTEX]);
	print_mutex_to_file(shared_counters[OPER_COUNTER]++, "%c %ld\t: bonded\n", name, number);
	sem_post(&semaphores[SHARED_MUTEX]);

	barrier3();
	sem_post(&semaphores[BOND_LOCK]);
}


void Oxygen(long number){	//body of oxygen process


	sem_wait(&semaphores[LOCK]);	
	sem_wait(&semaphores[BOND_LOCK]);

	sem_wait(&semaphores[SHARED_MUTEX]);

	print_mutex_to_file(shared_counters[OPER_COUNTER]++, "O %ld\t: started\n", number);

	if(shared_counters[H_COUNTER] > 1 && shared_counters[OX_COUNTER] >= 0){
		//sem_wait(&semaphores[LOCK]);
		print_mutex_to_file(shared_counters[OPER_COUNTER]++, "O %ld\t: ready\n", number);
		shared_counters[H_COUNTER] -= 2;
		sem_post(&semaphores[H]);
		sem_post(&semaphores[H]);
		sem_post(&semaphores[SHARED_MUTEX]);
		//sem_post(&semaphores[LOCK]);
	}
	else{
		print_mutex_to_file(shared_counters[OPER_COUNTER]++, "O %ld\t: waiting\n", number);
		sem_post(&semaphores[LOCK]);
		sem_post(&semaphores[BOND_LOCK]);
		shared_counters[OX_COUNTER]++;
		sem_post(&semaphores[SHARED_MUTEX]);
		sem_wait(&semaphores[OX]);
		//sem_wait(&semaphores[BOND_LOCK]);
	}
	
	bond('O', number);

	sem_wait(&semaphores[SHARED_MUTEX]);
	shared_counters[BOND_END]++;
	if (shared_counters[BOND_END] == 3){
		shared_counters[BOND_END] = 0;
		sem_post(&semaphores[LOCK]);
	}
	sem_post(&semaphores[SHARED_MUTEX]);	

	barrier_for_all();

	sem_wait(&semaphores[SHARED_MUTEX]);
	print_mutex_to_file(shared_counters[OPER_COUNTER]++, "O %ld\t: finished\n", number);
	sem_post(&semaphores[SHARED_MUTEX]);
}

void Hydrogen(long number){	  //boddy for hydrogen process

	
	sem_wait(&semaphores[LOCK]);	
	sem_wait(&semaphores[BOND_LOCK]);
	
	
	sem_wait(&semaphores[SHARED_MUTEX]);
	
	print_mutex_to_file(shared_counters[OPER_COUNTER]++, "H %ld\t: started\n", number);

	if(shared_counters[H_COUNTER] > 0 && shared_counters[OX_COUNTER] > 0){
		print_mutex_to_file(shared_counters[OPER_COUNTER]++, "H %ld\t: ready\n", number);
		shared_counters[H_COUNTER]--;
		shared_counters[OX_COUNTER]--;
		sem_post(&semaphores[H]);
		sem_post(&semaphores[OX]);
		sem_post(&semaphores[SHARED_MUTEX]);
		//sem_post(&semaphores[LOCK]);	
	}
	else{
		print_mutex_to_file(shared_counters[OPER_COUNTER]++, "H %ld\t: waiting\n", number);
		sem_post(&semaphores[LOCK]);
		sem_post(&semaphores[BOND_LOCK]);
		shared_counters[H_COUNTER]++;
		sem_post(&semaphores[SHARED_MUTEX]);
		sem_wait(&semaphores[H]);
		//sem_wait(&semaphores[BOND_LOCK]);
	}

	bond('H', number);

	sem_wait(&semaphores[SHARED_MUTEX]);
	shared_counters[BOND_END]++;
	if (shared_counters[BOND_END] == 3){
		shared_counters[BOND_END] = 0;
		sem_post(&semaphores[LOCK]);
	}
	sem_post(&semaphores[SHARED_MUTEX]);

	barrier_for_all();

	sem_wait(&semaphores[SHARED_MUTEX]);
	print_mutex_to_file(shared_counters[OPER_COUNTER]++, "H %ld\t: finished\n", number);
	sem_post(&semaphores[SHARED_MUTEX]);
}

void GenOxygen(){      //function for generate oxygen processes
	
	long oxygen_counter = 1;

	pid_t children[params[N]];

	usleep(rand()%(params[GO]+1));

	for (long i = 0; i < params[N]; i++) {
	pid_t pid = fork();
	if (pid) {					//parent
		children[i] = pid;
		oxygen_counter++;
		if (i == params[N]-1){
			for (long j = 0; j < params[N]; j++){
				waitpid(children[j], NULL, 0);
			}
		}	
		else continue;		
	} else if (pid == 0) {		//child
		Oxygen(oxygen_counter);
		break;
	} else {					//error
		perror("Create_process");
		deinit();
		exit(1);
		}
	}
}

void GenHydrogen(){		//function for generate hydrogen processes

	long hydrogen_counter = 1;

	usleep(rand()%(params[GH]+1));

	pid_t children[2*params[N]];
	for (long i = 0; i < 2*params[N]; i++) {
	pid_t pid = fork();
	if (pid) {					//parent
		children[i] = pid;
		hydrogen_counter++;
		if (i == 2*params[N]-1) {
			for (long j = 0; j < 2*params[N]; j++){
				waitpid(children[j], NULL, 0);
			}
		}			
		else continue;
	} else if (pid == 0) {		//child
		Hydrogen(hydrogen_counter);
		break;
	} else {					//error
		perror("Create_process");
		deinit();
		exit(1);
		}
	}	
}

int main(int argc, char const *argv[])
{
	
	if(argc != 5){			// arguments parsing
		fprintf(stderr, "%s\n", "Bad numbers of parameters");
		exit(1);
	}

	for (int i = 0; i < 4; i++){
		char * endptr = NULL;
		params[i] = strtol(argv[i+1], &endptr, 10);
		if (*endptr != '\0'){
			fprintf(stderr, "%s\n", "Parameters must be numbers between 0 - 5000");
			exit(1);
		}
		if (i == 0 && params[i] <= 0){
			fprintf(stderr, "%s\n", "First parameter must >=1");
			exit(1);
		}
		if (i != 0 && (params[i] < 0 || params[i] > 5000)){
			fprintf(stderr, "%d. %s\n", i, "parameter must be between 0 - 5000");
			exit(1);
		}
	}

	init();

	pid_t children[2];

	for (int i = 0; i < 2; i++) {
		pid_t pid = fork();
	if (pid) {					//parent
		children[i] = pid;
		if (i != 1)continue;
		else {			
			waitpid(children[0], NULL, 0);
			waitpid(children[1], NULL, 0);
		}
	} else if (pid == 0) {		//child
		if (i == 0)GenOxygen();
		if (i == 1)GenHydrogen();
		break;
	} else {					//error
		perror("Create_process");
		deinit();
		exit(1);
	}
}
deinit();
return 0;
}

