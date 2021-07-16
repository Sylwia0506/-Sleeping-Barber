#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include <getopt.h>
#include <unistd.h>
#define MAX 1000000

struct Clientlist
{
    int numb_client;
    struct Clientlist *next;
};



sem_t clients; 
sem_t barber; 
   
int time_client;     
int time_barbera;

int customer_on_the_armchair=-1; 

void randwait(int x) {
    usleep((rand()%x) * (rand()%MAX) +MAX );
}

struct Clientlist *not_accepted_client = NULL;
struct Clientlist *waiting_clients = NULL;

void writeMessage(struct Clientlist* list, char* message)
{
	struct Clientlist *pom = list;
    printf("%s ", message);
    while(pom!=NULL)
    {
        printf("%d ",pom->numb_client);
        pom = pom->next;
    }
    printf("\n");
}

void Customers_not_admitted(int cl)
{
    struct Clientlist *temp = (struct Clientlist*)malloc(sizeof(struct Clientlist));
    temp->numb_client = cl;
    temp->next = not_accepted_client;
    not_accepted_client = temp;
	
	writeMessage(not_accepted_client,"Client didn't get into the barbershop");
}

void Clients_waiting(int cl)
{
    struct Clientlist *temp = (struct Clientlist*)malloc(sizeof(struct Clientlist));
    temp->numb_client = cl;
    temp->next = waiting_clients;
    waiting_clients = temp;

	writeMessage(waiting_clients, "The customer is waiting:");
}

void delete_client(int cl)
{
    struct Clientlist *temp = waiting_clients;
    struct Clientlist *pop = waiting_clients;
    while (temp!=NULL)
    {
        if(temp->numb_client==cl)
        {
            if(temp->numb_client==waiting_clients->numb_client)
            {
                waiting_clients=waiting_clients->next;
                free(temp);
            }
            else
            {
                pop->next=temp->next;
                free(temp);
            }
            break;
        }
        pop=temp;
        temp=temp->next;
    }
	writeMessage(waiting_clients,"The customer is waiting:");
}

int customers_did_not_enter=0;
int available_seats_waiting_room=10;  
int free_chairs=10; 

bool debug=false; 
pthread_mutex_t armchair; 
pthread_mutex_t waiting_room; 

void clientCameToWaitingRoom(int numb_client)
{
	free_chairs--;
    printf("Res:%d WRomm: %d/%d [in: %d]  - a seat was taken in the waiting room\n",customers_did_not_enter, available_seats_waiting_room-free_chairs, available_seats_waiting_room, customer_on_the_armchair);
    if(debug==true)
    {
        Clients_waiting(numb_client);
    }
    if(sem_post(&clients)==-1)// sygnal dla fryzjera ze klient jest w poczekalni
	{
		perror("sem_post: &clients");
	} 
    if(pthread_mutex_unlock(&waiting_room)!=0)// odblokowanie poczekalni
	{                                        
		perror("pthread_mutex_unlock() error");                                       
		exit(1);                                                                    
	}   
}
void clientGoToBarberArmchair(int numb_client)
{
	if(pthread_mutex_lock(&armchair)!=0)//klient siada na fotelu, więc blokuje fotel
	{                                        
		perror("pthread_mutex_lock() error");                                       
		exit(1);                                                                    
	}    
    customer_on_the_armchair=numb_client;  
    printf("Res:%d WRomm: %d/%d [in: %d]  -  The barber is cutting hair\n",customers_did_not_enter, available_seats_waiting_room-free_chairs, available_seats_waiting_room,customer_on_the_armchair);
    if(debug==true)
    {
        delete_client(numb_client); //jak ostrzyżony to usuwam
    }
}

void clientResignFromTheVisit(int numb_client)
{
	if(pthread_mutex_unlock(&waiting_room)!=0) //odblokowanie poczekalni
	{                                        
		perror("pthread_mutex_unlock() error");                                       
		exit(1);                                                                    
	}  
    customers_did_not_enter++;
    printf("Res:%d WRomm: %d/%d [in: %d]  -  the client did not enter\n",customers_did_not_enter, available_seats_waiting_room-free_chairs, available_seats_waiting_room, customer_on_the_armchair);
    if(debug==true)
    {
        Customers_not_admitted(numb_client);
    }
}

void *Client(void *numb_client)
{
    randwait(8);
    if(pthread_mutex_lock(&waiting_room)!=0)// blokowanie fotela w poczekalni
	{                                        
		perror("pthread_mutex_lock() error");                                       
		exit(1);                                                                    
	}  
    if(free_chairs>0)
    {
		clientCameToWaitingRoom(*(int *)numb_client);
		if(sem_wait(&barber)==-1) //czekam , aż fryzjer skończy strzyc osobę
		{
			perror("sem_wait: &barber");
		} 
		clientGoToBarberArmchair(*(int *)numb_client);
    }
    else
    {
		clientResignFromTheVisit(*(int *)numb_client);
    }
}

bool end_work=false; 

void barberTakeClient()
{
	if(pthread_mutex_lock(&waiting_room)!=0)
	{                                        
		perror("pthread_mutex_lock() error");                                       
		exit(1);                                                                    
	}
	free_chairs++;
	if(pthread_mutex_unlock(&waiting_room)!=0)
	{                                        
		perror("pthread_mutex_unlock() error");                                       
		exit(1);                                                                    
	}
	if(sem_post(&barber)==-1) //fryzjer wysyła sygnał że moze ścinać
	{
		perror("sem_post: &barber");
	}
}

void clientHaircut()
{
	randwait(2);  
	printf("Res:%d WRomm: %d/%d [in: %d]  -  The barber has finished cutting hair.\n",customers_did_not_enter, available_seats_waiting_room-free_chairs, available_seats_waiting_room,customer_on_the_armchair);
}

void *Barber()
{
    while(!end_work)
    {
		if(!end_work)
		{
			if(sem_wait(&clients)==-1)// fryzjer śpi, aż go klient obudzi
			{
			perror("sem_wait: &clients");
			} 
			barberTakeClient();	//fryzjer bierze klienta	
			clientHaircut();

			if(pthread_mutex_unlock(&armchair)!=0)
			{                                      
   				 perror("pthread_mutex_unlock() error");                                     
   				 exit(2);                                                                    
  			}                                    
		}
		else 
			printf("Barber sleep \n"); 
    }
    printf("The barber is going home for the day.\n");
}

void initMutexAndSemaphore()
{
	if(sem_init(&clients,0,0)!=0)
	{
		perror("sem_init error");
		exit(EXIT_FAILURE);
	}
    if(sem_init(&barber,0,0)!=0)
	{
		perror("sem_init error");
		exit(EXIT_FAILURE);
	}

    if(pthread_mutex_init(&armchair, NULL)==-1)
	{
		perror("mutex_init error");                                                 
        exit(2); 
	}
    if(pthread_mutex_init(&waiting_room, NULL)==-1)
	{
		perror("mutex_init error");                                                 
        exit(2); 
	}
}

void destroyMutexAndSemaphore()
{
	pthread_mutex_destroy(&armchair);
    pthread_mutex_destroy(&waiting_room);
    sem_destroy(&clients);
    sem_destroy(&barber);
}

void deleteList(struct Clientlist* list)
{
	while (list != NULL)
	{
		struct Clientlist* tmp = list;
		list = list->next;
		free(tmp);
	}

}

int main(int argc, char *argv[])
{
	pthread_t barber_thread;
	pthread_t *clients_threads;
	int *t_clients;
	int number_of_clients=25;
    int choice = 0;
    srand(time(NULL));

    static struct option param[] =
    {
        {"Client", required_argument, NULL, 'c'},
        {"Armchair", required_argument, NULL, 'a'},
        {"debug", no_argument, NULL, 'd'}
    };
	
    while((choice = getopt_long(argc, argv, "c:a:d",param,NULL)) != -1)
    {
        switch(choice)
        {
			case 'c': 
				number_of_clients=atoi(optarg);
				break;
			case 'a': 
				free_chairs=atoi(optarg);
				available_seats_waiting_room=atoi(optarg);
				break;
			case 'd':
				debug=true;
				break;
			default: /* '?' */
            	fprintf(stderr, "Usage: %s [-option] [val]\n",argv[0]);
            	exit(EXIT_FAILURE);
        }
    }

    clients_threads = malloc(sizeof(pthread_t)*number_of_clients);
    t_clients=malloc(sizeof(int)*number_of_clients);

    int i;
    for (i=0; i<number_of_clients; i++)
    {
        t_clients[i] = i;
    }

    initMutexAndSemaphore();

    if(pthread_create(&barber_thread, NULL, Barber, NULL)!=0)
	{
		perror("create Thread");
        pthread_exit(NULL);
	}
	
    for (i=0; i<number_of_clients; ++i)
    {
        if(pthread_create(&clients_threads[i], NULL, Client, (void *)&t_clients[i])!=0)
		{
			 perror("create Thread");
             pthread_exit(NULL);
		}
    }

    for (i=0; i<number_of_clients; i++)
    {
        if(pthread_join(clients_threads[i],NULL)!=0)
		{
		perror("error joining thread:");
		}
    }
	end_work=true;
	if(pthread_join(barber_thread,NULL)!=0)
	{
		perror("error joining thread:");
	}

	destroyMutexAndSemaphore();
    deleteList(waiting_clients);
    deleteList(not_accepted_client);

    return 0;
}
