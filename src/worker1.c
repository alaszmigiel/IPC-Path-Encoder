#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sem.h>
#include <dirent.h>
#include <signal.h>
#include <pthread.h>
#include <stdbool.h>
#include <errno.h>

#define shm_size 1024

static struct sembuf buf;
char* shm_ptr = NULL;
DIR* dir_ptr = NULL;
bool paused = false;

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

//zwalnianie pamieci wspoldzielonej i zamykanie katalogu
void release_resources(char* shm_ptr, DIR* dir_ptr) 
{
    if (shm_ptr != NULL) 
	{
        if (shmdt(shm_ptr) == -1) 
		{
            perror("shmdt");
        }
    }

    if (dir_ptr != NULL) 
	{
        if (closedir(dir_ptr) == -1) 
		{
            perror("closedir");
        }
    }
}

//funkcje obslugi sygnalow
//SIGUSR1 zamykanie programu
void signal_handler_SIGUSR1(int signo)
{
    kill(getpid() + 1, SIGTERM);
    kill(getpid() + 2, SIGTERM);
    kill(getppid(), SIGTERM);
    release_resources(shm_ptr, dir_ptr);
    exit(0);
}

//SIGTERM info o zamykaniu programu
void signal_handler_SIGTERM(int signo)
{
	release_resources(shm_ptr, dir_ptr);
    exit(0);
}

//SIGQUIT wstrzymanie programu
void signal_handler_SIGQUIT(int signo)
{
	if(!paused)
	{
        kill(getpid() + 1, SIGHUP);
        kill(getpid() + 2, SIGHUP);
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
		kill(getpid() + 1, SIGBUS);
        kill(getpid() + 2, SIGBUS);
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
		kill(getpid() + 1, SIGUSR2);
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
    
    int semid;
    int shmid;
    struct dirent* dir;
    char path[shm_size];

    //obsluga pamieci wspoldzielonej
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
    
    sleep(1);
    
    //pobranie sciezki do katalogu
    while (1) 
	{
        printf("Sciezka do katalogu:\n");
        fgets(path, sizeof(path), stdin);
        size_t lenght = strlen(path);
        //usuniecie znaku nowej linii
        if(lenght > 0 && path[lenght - 1] == '\n')
		{
			path[lenght-1] = '\0';
		}

        dir_ptr = opendir(path);


        if (dir_ptr) 
		{
            break; 
        }
        else 
		{
            perror("opendir");
            printf("Nie udalo sie odnalezc katalogu. Sprobuj ponownie.\n");
        }
    }
	
    //pobieranie sciezek
    while ((dir = readdir(dir_ptr)) != NULL) 
	{
        char result[strlen(path) + strlen(dir->d_name) + 2]; //dlugosc +2 na / i '\0'
	    strcpy(result, path);
        strcat(result, "/");
        strcat(result, dir->d_name);
        semaphore_wait(semid, 0);
        strcpy(shm_ptr, result);

        while (paused) { } //oczekiwanie na wznowienie 
    
        semaphore_signal(semid, 1);
    }

    semaphore_wait(semid, 0);
    strcpy(shm_ptr, "brak");
    semaphore_signal(semid, 1);

    release_resources(shm_ptr, dir_ptr);

    return 0;
}

