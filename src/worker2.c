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
#include <pthread.h>
#include <stdbool.h>
#include <errno.h>

#define shm_size 1024

static struct sembuf buf;
char* shm_ptr = NULL;
bool paused = false;
bool encoding = true;

//funkcje obslugi semaforow
void semaphore_signal(int semid, int semnum) 
{
    buf.sem_num = semnum;
    buf.sem_op = 1;
    buf.sem_flg = 0;
    if (semop(semid, &buf, 1) == -1) 
	{
        perror("Podnoszenie semafora");
        exit(1);
    }
}

void semaphore_wait(int semid, int semnum) 
{
    buf.sem_num = semnum;
    buf.sem_op = -1;
    buf.sem_flg = 0;
    while (semop(semid, &buf, 1)) 
	{
        if (errno == EINTR) 
		{
            continue; 
        } 
		else 
		{
            perror("Opuszczenie semafora");
            exit(1);
        }
    }
}

//zwalnianie pamieci wspoldzielonej 
void release_resources(char* shm_ptr) 
{
   if (shm_ptr != NULL) 
   {
        if (shmdt(shm_ptr) == -1) 
		{
            perror("shmdt");
        }
    }
}

//zmiana ascii na szesnastkowy
void string_to_hex(const char* input, char* output) 
{
    if (input == NULL || output == NULL)
    {
    	perror("Pointer NULL");
    	return;
	}

    size_t lenght = strlen(input); 

    size_t i, j = 0;
    
    for (i = 0; i < lenght; ++i) 
	{
		//konwersja na szesnastkowy
        sprintf(output + j, "%02x", (unsigned char)input[i]); 
        j += 2; 
    }
}

//funkcje obslugi sygnalow
//SIGUSR1 zamykanie programu
void signal_handler_SIGUSR1(int signo)
{
    kill(getpid() - 1, SIGTERM);
    kill(getpid() + 1, SIGTERM);
    kill(getppid(), SIGTERM);
    release_resources(shm_ptr);
    exit(0);
}

//SIGTERM info o zamykaniu programu
void signal_handler_SIGTERM(int signo)
{
	release_resources(shm_ptr);
    exit(0);
}

//SIGQUIT wstrzymanie programu
void signal_handler_SIGQUIT(int signo)
{
	if(!paused)
	{
		kill(getpid() - 1, SIGHUP);
        kill(getpid() + 1, SIGHUP);
        paused = true;
	}
}

//SIGHUP info o wstrzymaniu programu
void signal_handler_SIGHUP(int signo)
{
	paused = true;
}

//SIGCONT wznowienie programu
void signal_handler_SIGCONT(int signo)
{
	if(paused)
	{
		kill(getpid() - 1, SIGBUS);
        kill(getpid() + 1, SIGBUS);
        paused = false;
	}
}

//SIGBUS info o wznowieniu programu
void signal_handler_SIGBUS(int signo)
{
	paused = false;
}

//SIGUSR2 wlaczenie/wylaczenie kodowania
void signal_handler_SIGUSR2(int signo)
{
	if(!paused)
	{
		if(encoding)
        {
    	    encoding=false;
	    }
	    else
	    {
		    encoding=true;
	    }
	}
}

//watek obslugujacy sygnaly
void *Thread(void *t)
{
    while (1)
    {
        signal(SIGUSR1, signal_handler_SIGUSR1);
        signal(SIGTERM, signal_handler_SIGTERM);
        signal(SIGQUIT, signal_handler_SIGQUIT);
        signal(SIGHUP, signal_handler_SIGHUP);
        signal(SIGCONT, signal_handler_SIGCONT);
        signal(SIGBUS, signal_handler_SIGBUS);
        signal(SIGUSR2, signal_handler_SIGUSR2);
    }
    return 0;
}
    
int main(int argc, char* argv[]) 
{
    if (argc < 2) 
	{
        perror("argc");
        exit(1);
    }
    key_t key = atoi(argv[1]);
    
    pthread_t t;
    pthread_create(&t, NULL, Thread, NULL);
    
    int shmid, semid;
    char path[shm_size];
    char hex[2 * shm_size];

    //obsluga pamieci wpoldzielonej
    if ((shmid = shmget(key, 2 * shm_size, 0666)) == -1) 
	{
        perror("shmget");
        exit(1);
    }

    if ((shm_ptr = shmat(shmid, NULL, 0)) == (void*)-1) 
	{
        perror("shmat");
        exit(1);
    }

    //obsluga semaforow
    semid = semget(key, 3, 0666);
    if (semid == -1) 
	{
        perror("semget");
        exit(1);
    }

    //wypis sciezek
    while (1) 
	{
        semaphore_wait(semid, 1);
        printf("drugi");
        strcpy(path, shm_ptr);

        if (strcmp(path, "brak") == 0) 
		{
            strcpy(shm_ptr, path);
            semaphore_signal(semid, 2);
            break;
        }

        memset(hex, 0, sizeof(hex));
        string_to_hex(path, hex);

        sleep(2);
        
        if(encoding)
        {
        	printf("%s -:- %s\n", path, hex);
            strcpy(shm_ptr, hex);
		}
        else
        {
        	printf("%s -:- %s\n", path, path);
            strcpy(shm_ptr, path);
		}

        while (paused) { } //oczekiwanie na wznowienie 
        
        semaphore_signal(semid, 2);
    }
    
    release_resources(shm_ptr);
    
    return 0;
}
