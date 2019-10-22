#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "a2_helper.h"
#include <sys/sem.h>
#include <pthread.h>
#include <stdbool.h>
#include <semaphore.h>
#include <fcntl.h>

pthread_t *threads;							
int* values;

int sem_id;

sem_t *sem1;
sem_t *sem2;

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

	if(*th_id==1)
	{
		info(BEGIN,7,*th_id);
		values[2]=3;
		pthread_create(&threads[2],NULL,(void*)thread_function7,&values[2]);
		pthread_join(threads[2],NULL);	
		info(END,7,*th_id);
	}
	else if(*th_id==4)
	{
		sem_wait(sem1);
		info(BEGIN,7,*th_id);
		info(END,7,*th_id);
		sem_post(sem1);
		sem_post(sem2);
	}
	else
	{
		info(BEGIN,7,*th_id);
		info(END,7,*th_id);
	}
}

int nb_threads=0;

void thread_function3(void* arg)
{
	int* th_id=(int*) arg;
	if(*th_id!=12)
	{
		P(sem_id,0);
	}
	info(BEGIN,3,*th_id);

	P(sem_id,1);
	nb_threads++;
	V(sem_id,1);

	if(nb_threads==4)
	{
		V(sem_id,2);
	}

	if(*th_id==12)
	{	
		P(sem_id,2);
		V(sem_id,3);
		info(END,3,*th_id);
		V(sem_id,2);
	}

	if(*th_id!=12)
	{
		P(sem_id,3);
		V(sem_id,3);
	}

	P(sem_id,1);
	nb_threads--;
	V(sem_id,1);

	if(*th_id!=12)
	{
		P(sem_id,2);
		info(END,3,*th_id);
		V(sem_id,0);
		V(sem_id,2);
	}
	
}

void thread_function6(void* arg)
{
	int* th_id=(int*) arg;
	
	if(*th_id==6)
	{
		info(BEGIN,6,*th_id);
		info(END,6,*th_id);
		sem_post(sem1);
	}
	else if(*th_id==4)
	{
		sem_wait(sem2);
		info(BEGIN,6,*th_id);
		info(END,6,*th_id);
		sem_post(sem2);
	}
	else
	{
		info(BEGIN,6,*th_id);
		info(END,6,*th_id);
	}
}

int main(int argc, char** argv){
    
    int P2,P3,P4,P5,P6,P7,P8;
    int state2, state3, state4, state5, state6, state7, state8;
   
    init();
	
    sem1 = sem_open("/sem1", O_CREAT, 0644, 1);
    sem2 = sem_open("/sem2", O_CREAT, 0644, 1);	

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
				semctl(sem_id,0,SETVAL,3);
				semctl(sem_id,1,SETVAL,1);
				semctl(sem_id,2,SETVAL,1);
				semctl(sem_id,3,SETVAL,1);
				
				P(sem_id,2);
				P(sem_id,3);

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

				info(END,3,0);
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
								info(END,8,0);
								break;
							default: waitpid(P8,&state8,0);
								 info(END,4,0);
								 break;
						}
						break;
					default: waitpid(P3,&state3,0);
						 waitpid(P4,&state4,0);
						 info(END, 2, 0);
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
				info(END,5,0);
				break;
			default: P6=fork();
				 switch(P6)
				 {
					case -1: perror("Cannot create a new child");
 						 exit(1);
					case 0: info(BEGIN,6,0);
						P7=fork();
						switch(P7)
						{
							case -1: perror("Cannot create a new child");
 						 		 exit(1);
							case 0: info(BEGIN,7,0);

                                                                int i;
								threads=(pthread_t*)calloc(4,sizeof(pthread_t));
								values=(int*)calloc(4,sizeof(int));

   					       			sem_wait(sem1);

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
								
								sem_close(sem1);
								sem_close(sem2);
								free(values);
	 							free(threads);
								info(END,7,0);
								break;
							default: 
								 threads=(pthread_t*)calloc(6,sizeof(pthread_t));
								 values=(int*)calloc(6,sizeof(int));

								 sem_wait(sem2);

								 for(i=0;i<6;i++)
								 {
									values[i]=i+1;
									pthread_create(&threads[i],NULL,(void*)thread_function6,&values[i]);
								 }
								 for(i=0;i<6;i++)
								 {
									pthread_join(threads[i],NULL);	
								 }
								 sem_close(sem1);
								 sem_close(sem2);	
								 free(values);
	 							 free(threads);

								 waitpid(P7,&state7,0);
								 info(END,6,0);
								 break;
						}
						break;
					default: waitpid(P2,&state2,0);
						 waitpid(P5,&state5,0);
						 waitpid(P6,&state6,0);
						 info(END, 1, 0);
						 break;
				 }
				 break;
		}
		break;
    }   
    
    sem_close(sem1);
    sem_close(sem2);
    sem_unlink("/sem1");
    sem_unlink("/sem2");

    return 0;
}
