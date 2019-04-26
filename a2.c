#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "a2_helper.h"
#include <sys/sem.h>
#include <pthread.h>
#include <stdbool.h>

pthread_t *threads;							
int* values;

int sem_id;

void P(int sem_id, int sem_no)
{
	struct sembuf op = {sem_no,-1,0};
	semop(sem_id,&op,1);
}

void V(int sem_id, int sem_no)
{
	struct sembuf op = {sem_no,+1,0};
	semop(sem_id,&op,1);
}

void thread_function7(void* arg)
{
	int* th_id=(int*) arg;
	info(BEGIN,7,*th_id);

	if(*th_id==1)
	{
		values[2]=3;
		pthread_create(&threads[2],NULL,(void*)thread_function7,&values[2]);
		pthread_join(threads[2],NULL);	
	}
	info(END,7,*th_id);
}

int nb_threads=0;
bool th12_ended=false;

void thread_function3(void* arg)
{
	int* th_id=(int*) arg;
	P(sem_id,0);
	info(BEGIN,3,*th_id);

	P(sem_id,1);
	nb_threads++;
	V(sem_id,1);

	if(*th_id==12)
	{	
		P(sem_id,2);	
	
		th12_ended=true;
		V(sem_id,3);	
	}
	else if(*th_id==1 || *th_id==2 || *th_id==3)
	{
		if(!th12_ended)
		{
			P(sem_id,3);	
		}
	}
	if(nb_threads==4)
	{
		V(sem_id,2);
	}
	
	P(sem_id,1);
	nb_threads--;
	V(sem_id,1);

	info(END,3,*th_id);
	V(sem_id,0);
	
}

void thread_function6(void* arg)
{
	int* th_id=(int*) arg;
	info(BEGIN,6,*th_id);
	
	info(END,6,*th_id);
}

int main(int argc, char** argv){
    
    int P2,P3,P4,P5,P6,P7,P8;
    int state2, state3, state4, state5, state6, state7, state8;
   
    init();

    info(BEGIN, 1, 0);
    
    P2=fork();

    switch(P2)
    {
	case -1: perror("Cannot create a new child");
 		 exit(1);
        case 0: info(BEGIN, 2, 0);
		P3=fork();
		switch(P3)
		{
			case -1: perror("Cannot create a new child");
 				 exit(1);
			case 0: info(BEGIN,3,0);
				int i;
				threads=(pthread_t*)calloc(46,sizeof(pthread_t));
				values=(int*)calloc(46,sizeof(int));
				
				sem_id=semget(IPC_PRIVATE,4,IPC_CREAT | 0600);

	 			if (sem_id < 0) {
        				perror("Error creating the semaphore set");
        				exit(2);
    				}
				semctl(sem_id,0,SETVAL,4);
				semctl(sem_id,1,SETVAL,1);
				semctl(sem_id,2,SETVAL,1);
				semctl(sem_id,3,SETVAL,3);

				for(i=0;i<46;i++)
				{
					values[i]=i+1;
					pthread_create(&threads[i],NULL,(void*)thread_function3,&values[i]);
				
				}
				for(i=0;i<46;i++)
				{
					pthread_join(threads[i],NULL);	
				}
				semctl(sem_id,0,IPC_RMID,0);	
				semctl(sem_id,1,IPC_RMID,0);	
				semctl(sem_id,2,IPC_RMID,0);
				semctl(sem_id,3,IPC_RMID,0);
				free(values);
 				free(threads);
				break;
			default: P4=fork();
				 switch(P4)
				 {
				 	case -1: perror("Cannot create a new child");
 						 exit(1);
					case 0: info(BEGIN,4,0);
						P8=fork();
						switch(P8)
						{
							case -1: perror("Cannot create a new child");
 						 		 exit(1);
							case 0: info(BEGIN,8,0);
								break;
							default: waitpid(P8,&state8,0);
								 info(END,8,0);
								 break;
						}
						break;
					default: waitpid(P3,&state3,0);
						 info(END,3,0);
						 waitpid(P4,&state4,0);
						 info(END,4,0);
						 break;
				 }
				 break;
		}
		break;
	default:
		P5=fork();
		switch(P5)
		{
			case -1: perror("Cannot create a new child");
 				 exit(1);
			case 0: info(BEGIN,5,0);
				break;
			default: P6=fork();
				 switch(P6)
				 {
					case -1: perror("Cannot create a new child");
 						 exit(1);
					case 0: info(BEGIN,6,0);
						int i;
						threads=(pthread_t*)calloc(6,sizeof(pthread_t));
						values=(int*)calloc(6,sizeof(int));

						for(i=0;i<6;i++)
						{
							
							{
								values[i]=i+1;
								pthread_create(&threads[i],NULL,(void*)thread_function6,&values[i]);
							}	
						}
						for(i=0;i<6;i++)
						{
							//if(i!=2)
							{
								pthread_join(threads[i],NULL);	
							}
						}
							
						free(values);
	 					free(threads);

						P7=fork();
						switch(P7)
						{
							case -1: perror("Cannot create a new child");
 						 		 exit(1);
							case 0: info(BEGIN,7,0);
								
                                                                int i;
								threads=(pthread_t*)calloc(4,sizeof(pthread_t));
								values=(int*)calloc(4,sizeof(int));

								for(i=0;i<4;i++)
								{
									if(i!=2)
									{
										values[i]=i+1;
										pthread_create(&threads[i],NULL,(void*)thread_function7,&values[i]);
									}	
								}
								for(i=0;i<4;i++)
								{
									if(i!=2)
									{
										pthread_join(threads[i],NULL);	
									}
								}
								
								free(values);
	 							free(threads);
								break;
							default: waitpid(P7,&state7,0);
								 info(END,7,0);
								 break;
						}
						break;
					default: waitpid(P2,&state2,0);
						 info(END, 2, 0);
						 waitpid(P5,&state5,0);
						 info(END,5,0);
						 waitpid(P6,&state6,0);
						 info(END,6,0);
						 info(END, 1, 0);
						 break;
				 }
				 break;
		}
		break;
    }   

    return 0;
}
