#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <signal.h>
#include <errno.h>

#define shm_size 1024

#define PROCESS_COUNT 3
pid_t pid[PROCESS_COUNT];
pid_t ppid;

int shmid, semid;
char* shm_ptr = NULL;


//zwalnianie pamieci wspoldzielonej i semaforow
void release_resources(int semid, char* shm_ptr) 
{
	
	if (shm_ptr != NULL) 
	{
        if (shmdt(shm_ptr) == -1) 
		{
            perror("shmdt");
        }
    }
    
     if (semctl(semid, 0, IPC_STAT, NULL) != -1) 
	 {
        if (semctl(semid, 0, IPC_RMID) == -1) 
		{
            perror("semctl");
            exit(1);
        }
    }
}

//SIGTERM info o zamykaniu program
void signal_handler_SIGTERM(int signo)
{
    /*int i;
    for (i = 0; i < PROCESS_NUM; ++i) 
	{
        wait(NULL);
    }
    */
    sleep(1);
    printf("Zwalnianie zasobow\n");
    sleep(1);
    release_resources(shmid, shm_ptr);
    printf("Koniec programu\n");
    exit(0);
}

int main() 
{
	signal(SIGTERM, signal_handler_SIGTERM);
    
    printf("SYGNALY:\n");
    printf("SIGUSR1 - zakonczenie dzialania aplikacji\n");
    printf("SIGQUIT - wstrzymanie dzialania aplikacji\n");
    printf("SIGCONT - wznowienie dzialania aplikacji\n");
    printf("SIGUSR2 - wlaczenie/wylaczenie kodowania\n");
    sleep(2);
    
    key_t key;
    int i;
    ppid = getpid();
    printf("Proces macierzysty PID: %i\n", ppid);
	
	FILE* key_file = fopen("key", "w");
    if (key_file == NULL) {
        perror("Tworzenie pliku 'key'");
        exit(1);
    }
    fclose(key_file);

    //generowanie klucza
    if ((key = ftok("key", 'K')) == -1) 
	{
        perror("ftok");
        exit(1);
    }

    //obsluga pamieci wspoldzielonej
    if ((shmid = shmget(key, 2 * shm_size, 0666 | IPC_CREAT)) == -1) 
	{
        perror("shmget");
        exit(1);
    }

    if ((shm_ptr = shmat(shmid, NULL, 0)) == (void*)-1) 
	{
        perror("shmat");
        exit(1);
    }
    
    //inicjalizacja semaforow
    semid = semget(key, 3, 0666 | IPC_CREAT);
    if (semid == -1) 
	{
        perror("semget");
        exit(1);
    }
    if (semctl(semid, 0, SETVAL, 1) == -1) 
	{
        perror("semctl p1");
        exit(1);
    }
    if (semctl(semid, 1, SETVAL, 0) == -1) 
	{
        perror("semctl p2");
        exit(1);
    }
    if (semctl(semid, 2, SETVAL, 0) == -1) 
	{
        perror("semctl p3");
        exit(1);
    }

    int size = snprintf(NULL, 0, "%d", key);
    char *shm_key = (char *)malloc(size + 1);
    if (shm_key == NULL) 
	{
        perror("malloc");
        exit(1);
	}
    snprintf(shm_key, size + 1, "%d", key);

    //tworzenie procesow potomnych
    pid[0] = fork();
    if (pid[0] < 0) 
	{
        perror("fork failed");
        exit(1);
    }
    if (pid[0] == 0) 
	{
        execl("./worker1", "worker1", shm_key, NULL);
        perror("Uruchomienie worker 1");
        exit(1);
    }

    pid[1] = fork();
    if (pid[1] < 0) 
	{
        perror("fork failed");
        exit(1);
    }
    if (pid[1] == 0) 
	{
        execl("./worker2", "worker2", shm_key, NULL);
        perror("Uruchomienie worker 2");
        exit(1);
    }

    pid[2] = fork();
    if (pid[2] < 0) 
	{
        perror("fork failed");
        exit(1);
    }
    if (pid[2] == 0) 
	{
        execl("./worker3", "worker3", shm_key, NULL);
        perror("Uruchomienie worker 3");
        exit(1);
    }

    for (i = 0; i < PROCESS_COUNT; ++i) 
	{
        printf("Proces potomny %d PID: %i\n", i+1, pid[i]);
    }
    
    //oczekiwanie na zakonczenie dzialania procesow potomnych
    for (i = 0; i < PROCESS_COUNT; ++i) 
	{
        waitpid(pid[i], NULL, 0);
    }

    release_resources(shmid, shm_ptr);
    
    return 0;
}

