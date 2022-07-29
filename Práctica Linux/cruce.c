/*
  |---------------------------------|
  |Juan José López Gómez   			|
  |Enrique Mesonero Ronco  			|
  |---------------------------------|
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/msg.h>

#include "cruce.h"

#define LIM_INF1 13
#define LIM_INF2 16
#define LIM_INF3 0
#define LIM_INF4 25

#if defined(__GNU_LIBRARY__) && !defined(_SEM_SEMUN_UNDEFINED)
       //Al estar definida en <sys/sem.h> en el linux de clase no hace falta la union
#else
	//Parte para Solaris de Encina	
       union semun {
               int val;                    /* valor para SETVAL */
               struct semid_ds *buf;       /* buffer para IPC_STAT, IPC_SET */
               unsigned short int *array;  /* array para GETALL, SETALL */
               struct seminfo *__buf;      /* buffer para IPC_INFO */
       };
#endif

char * shaddr;
pid_t ppid;

void espera();
void finalizar();

int guardar(int , int ,int *, int *);

void Wait(int,int);
void Signal(int,int);

void Ciclo_Semaforico(int,int);
void zona_coche(int,int);
void zona_peaton(int,int);
	
typedef struct mensaje
{
	long tipo;
}mensaje;

int main (int argc, char*argv[])
{
	ppid=getpid();
	union semun s;
	int nPro,nSpeed,id, tipo, flag=0,i;
	int semid;
	int *shid, *msgid;
	mensaje m;

	if((shid=malloc(sizeof(int)))==NULL)
	{
		perror("Error en la reserva de memoria");
		exit(7);
	}
	
	if((msgid=malloc(sizeof(int)))==NULL)
	{
		perror("Error en la reserva de memoria");
		exit(7);
	}		
	
	struct sigaction salir;
	salir.sa_handler=espera;
	sigfillset(&salir.sa_mask);
	salir.sa_flags=0;
	
	if(sigaction(SIGINT, &salir,NULL)==-1)
		{
			perror("Error al capturar la senial");
			exit(6);
		}
	
	struct sigaction ignore;	
	ignore.sa_handler = SIG_IGN;
	ignore.sa_flags = 0;
	if(sigaction(SIGCHLD, &ignore, NULL)==-1)
		{
			perror("Error al capturar la senial");
			exit(6);
		}

	if(argc!=3)
		{
			printf("Error en el numero de entradas por la linea de ordenes\n");
			return 1;
		}
	else
		{
			nPro=atoi(argv[1]);
			nSpeed=atoi(argv[2]);
			if(nPro<=0 || nSpeed<0)
				{
					printf("Error en los parametros\n");
					return 2;
				}
			else
			{
				if((semid=semget(IPC_PRIVATE,20,IPC_CREAT | 0600))==-1)
					{
						perror("Error en la creacion del semaforo");
						raise(SIGINT);
					}

				guardar(0,semid,0,0);
				
				if((*shid=shmget(IPC_PRIVATE,1000,IPC_CREAT | 0600))==-1)
					{
						perror("Error en la creacion de la zona de memoria compartida");
						raise(SIGINT);
					}

				guardar(1,0,shid,0);
				
				if((shaddr=shmat(*shid,0,0))==(char*)-1)
					{
						perror("Error en la asignacion del puntero");
						raise(SIGINT);
					}
					
				if((*msgid=msgget(IPC_PRIVATE,IPC_CREAT | 0600))==-1)
					{
						perror("Error en la creacion del buzon");
						raise(SIGINT);
					}

				guardar(2,0,0,msgid);
				
				if(CRUCE_inicio(nSpeed,nPro,semid,shaddr)==-1)
					{
						perror("Error en la funcion cruce_inicio");
						raise(SIGINT);
					}
					
				s.val=nPro-2;
				if(semctl(semid,2,SETVAL,&s)==-1)
					{
						perror("Error en dar valor al semaforo");
						raise(SIGINT);
					}
					
				for(i=3; i<20;i++)
				{
					if(i==3 || i==6 || i==9 || i==10 || i==11 || i==12 || i==13 || i==14)
						s.val=1;
					else
						s.val=0;
						
					if(semctl(semid,i,SETVAL,&s)==-1)
						{
							perror("Error en dar valor al semaforo");
							raise(SIGINT);
					}		
				}
				
				id=fork();
				switch(id)
				{
					case -1:
						{
							perror("Error al crear el hijo manejador del semaforo");
							raise(SIGINT);
						}
					case 0:
						{
							salir.sa_handler=finalizar; 
							if(sigaction(SIGINT, &salir,NULL)==-1)
								{
									perror("Error al capturar la senial");
									raise(SIGINT);
								}
							Ciclo_Semaforico(*msgid,semid);
						}
				}
					
					while(1)
					{
						if(!flag)
							{	
								for(i=1;i<1600;i++)
									{
										m.tipo=i;
										if(msgsnd(*msgid,&m,sizeof(m)-sizeof(long),0)==-1)
											raise(SIGINT);
									}
									flag=1;
							}
							
						Wait(semid,2);
						tipo=CRUCE_nuevo_proceso();
						id=fork();
						switch(id)
							{
								case -1:
									{
										perror("Error en el fork");
										raise (SIGINT);
									}
								case 0:
									{
										salir.sa_handler=finalizar;
										if(sigaction(SIGINT, &salir,NULL)==-1)
										{
											perror("Error al capturar la senial");
											raise(SIGINT);
										}
										if(!tipo)
											{
												zona_coche(semid,*msgid);
											}
										else
											{
												zona_peaton(semid,*msgid);
											}	
									}
							}			
					}
			}
	}
}
	
void espera()
{
	if(ppid==getpid())
		{
			int status,semid,*shid,*msgid;
			if((shid=malloc(sizeof(int)))==NULL)
			{
				perror("Error en la reserva de memoria");
				exit(7);
			}

			if((msgid=malloc(sizeof(int)))==NULL)
			{
				perror("Error en la reserva de memoria");
				exit(7);
			}

			semid=guardar(3,0,shid,msgid);

			if(CRUCE_fin()==-1)
				perror("Error en la funcion cruce_fin");
				
			if(semctl(semid,0,IPC_RMID)==-1)
				perror("\nError en la eliminación del semaforo");
				
			if(msgctl(*msgid,IPC_RMID,0)==-1)
				perror("\nError en la eliminación del buzon");
				

			if((shmdt(shaddr))==-1)		
				perror("\nError al liberar la memoria compartida");
				
			if(shmctl(*shid,IPC_RMID,0)==-1)
				perror("\nError en la eliminación de la zona de memoria compartida");
				
			exit(0);
		}

	finalizar();
}

void finalizar()
{
	exit(0);
}

int guardar(int control, int semid, int *shid, int * msgid)
{
	static int Semid;
	static int Shid;
	static int Msgid;
	
	if(control==0)
		Semid=semid;

	else if(control==1)
		Shid=*shid;
		
	else if(control==2)		
		Msgid=*msgid;
	
	else if(control==3)
		{
			*shid=Shid;
			*msgid=Msgid;
			return Semid;
		}

	return 0;
}


void Ciclo_Semaforico(int msgid,int semid)
{
	char buffer[100];
	int i;
	
	//PreCiclo
	if(CRUCE_pon_semAforo(SEM_C1, VERDE) == -1)
		raise (SIGINT);

	if(CRUCE_pon_semAforo(SEM_C2, ROJO) == -1)
		raise (SIGINT);

	if(CRUCE_pon_semAforo(SEM_P1, ROJO) == -1)
		raise (SIGINT);	

	if(CRUCE_pon_semAforo(SEM_P2, VERDE) == -1)
		raise (SIGINT);

	while(1) 
	{
		//InterFase 1->2
		if(CRUCE_pon_semAforo(SEM_C1, AMARILLO)==-1)
				raise (SIGINT);
				
		Wait(semid,6);
		for(i=0;i<2;i++)
			pausa();
		Signal(semid,6);

		//FASE 2
		if(CRUCE_pon_semAforo(SEM_C2,VERDE) ==-1 || CRUCE_pon_semAforo(SEM_C1, ROJO)==-1 || CRUCE_pon_semAforo(SEM_P2, ROJO)==-1)
			raise(SIGINT);

		Signal(semid,4);
		for(i=0;i<9;i++)
			pausa();
		Wait(semid,4);
		
		//InterFase 2->3
		if(CRUCE_pon_semAforo(SEM_C2, AMARILLO)==-1)
			raise(SIGINT);
			
		Wait(semid,6);
		for(i=0;i<2;i++)
			pausa();
		Signal(semid,6);

		//FASE 3
		if(CRUCE_pon_semAforo(SEM_C2, ROJO)==-1 || CRUCE_pon_semAforo(SEM_P1, VERDE)==-1)
			raise(SIGINT);

		Wait(semid,12);
		Signal(semid,7);
		for(i=0;i<12;i++)
			pausa();
		Wait(semid,7);
		Signal(semid,12);

		//FASE 1
		if(CRUCE_pon_semAforo(SEM_C1, VERDE)==-1 || CRUCE_pon_semAforo(SEM_P1, ROJO)==-1 || CRUCE_pon_semAforo(SEM_P2, VERDE)==-1)			
				raise (SIGINT);
		
		Signal(semid,5);
		Signal(semid,8);			
	
		for(i=0;i<6;i++)
			pausa();
		Wait(semid,8);
		Wait(semid,5);
				
	}
}

void zona_coche(int semid, int msgid)
{
	int i;
	struct posiciOn p;
	struct mensaje m;
	p=CRUCE_inicio_coche();
	if(p.x==-1 ||p.y==-1)
		{
			perror("Error en la funcion Cruce_inicio_coche");
			raise(SIGINT);
		}
	if(p.x==-3)
	{
			
		while(p.x<33)
		{
			if(p.x==-3)
				Wait(semid,9);
		
			if(p.x<=6)
				{
					if(p.x==-3)
						{
							if(msgrcv(msgid,&m,sizeof(m)-sizeof(long),3,0)==-1)
								raise(SIGINT);
						}

					if(p.x==-1)
						{
							if(msgrcv(msgid,&m,sizeof(m)-sizeof(long),5,0)==-1)
								raise(SIGINT);
						}

					if(p.x==1)
						{
							if(msgrcv(msgid,&m,sizeof(m)-sizeof(long),7,0)==-1)
								raise(SIGINT);
							if(msgrcv(msgid,&m,sizeof(m)-sizeof(long),1,0)==-1)
								raise(SIGINT);
						}

					if(p.x==3)
						{
							if(msgrcv(msgid,&m,sizeof(m)-sizeof(long),9,0)==-1)
								raise(SIGINT);
							if(msgrcv(msgid,&m,sizeof(m)-sizeof(long),1,0)==-1)
								raise(SIGINT);
						}

					if(p.x==5)
						{
							if(msgrcv(msgid,&m,sizeof(m)-sizeof(long),11,0)==-1)
								raise(SIGINT);
							if(msgrcv(msgid,&m,sizeof(m)-sizeof(long),3,0)==-1)
								raise(SIGINT);
						}
				}

			else
				{
					if(msgrcv(msgid,&m,sizeof(m)-sizeof(long),p.x+6,0)==-1)
						raise(SIGINT);
					if(msgrcv(msgid,&m,sizeof(m)-sizeof(long),p.x-2,0)==-1)
						raise(SIGINT);
				}
						
			if(p.x==13)
				{
					Signal(semid,9);
					Wait(semid,4);
					Wait(semid,14);
					p=CRUCE_avanzar_coche(p);
					if(pausa_coche()==-1)
						{
							perror("Error en la funcion pausa_coche");
							raise(SIGINT);
						}

					m.tipo=9;
					if(msgsnd(msgid,&m,sizeof(m)-sizeof(long),0)==-1)
						raise(SIGINT);

					m.tipo=17;
					if(msgsnd(msgid,&m,sizeof(m)-sizeof(long),0)==-1)
						raise(SIGINT);

					Signal(semid,4);
				}
			else if(p.x==15)
				{
					Wait(semid,6);

					p=CRUCE_avanzar_coche(p);
					if(pausa_coche()==-1)
						{
							perror("Error en la funcion pausa_coche");
							raise(SIGINT);
						}

				Signal(semid,6);
				m.tipo=11;
				if(msgsnd(msgid,&m,sizeof(m)-sizeof(long),0)==-1)
					raise(SIGINT);
				m.tipo=19;
				if(msgsnd(msgid,&m,sizeof(m)-sizeof(long),0)==-1)
					raise(SIGINT);
			}else if(p.x==23)
			{
				Wait(semid,3);

				p=CRUCE_avanzar_coche(p);
				if(pausa_coche()==-1)
				{
						perror("Error en la funcion pausa_coche");
						raise(SIGINT);
				}
				m.tipo=19;
				if(msgsnd(msgid,&m,sizeof(m)-sizeof(long),0)==-1)
					raise(SIGINT);
				m.tipo=27;
				if(msgsnd(msgid,&m,sizeof(m)-sizeof(long),0)==-1)
					raise(SIGINT);
			}else if(p.x==27)
			{
				Wait(semid,6);
				p=CRUCE_avanzar_coche(p);
				if(pausa_coche()==-1)
				{
						perror("Error en la funcion pausa_coche");
						raise(SIGINT);
				}
				m.tipo=23;
				if(msgsnd(msgid,&m,sizeof(m)-sizeof(long),0)==-1)
					raise(SIGINT);
				m.tipo=31;
				if(msgsnd(msgid,&m,sizeof(m)-sizeof(long),0)==-1)
					raise(SIGINT);
			}else{
			
			if(p.x<=6)
			{
				if(p.x==-1)
				{
				m.tipo=3;
				if(msgsnd(msgid,&m,sizeof(m)-sizeof(long),0)==-1)
					raise(SIGINT);
				}
				if(p.x==1)
				{
				m.tipo=1;
				if(msgsnd(msgid,&m,sizeof(m)-sizeof(long),0)==-1)
					raise(SIGINT);
				m.tipo=5;
				if(msgsnd(msgid,&m,sizeof(m)-sizeof(long),0)==-1)
					raise(SIGINT);
				}
				if(p.x==3)
				{
				m.tipo=1;
				if(msgsnd(msgid,&m,sizeof(m)-sizeof(long),0)==-1)
					raise(SIGINT);
				m.tipo=7;
				if(msgsnd(msgid,&m,sizeof(m)-sizeof(long),0)==-1)
					raise(SIGINT);
				}
				if(p.x==5)
				{
				m.tipo=1;
				if(msgsnd(msgid,&m,sizeof(m)-sizeof(long),0)==-1)
					raise(SIGINT);
				m.tipo=9;
				if(msgsnd(msgid,&m,sizeof(m)-sizeof(long),0)==-1)
					raise(SIGINT);
				}

			}else if(p.x!=13 || p.x!=15 || p.x!=27 || p.x!=23){
			
				m.tipo=p.x-4;
				if(msgsnd(msgid,&m,sizeof(m)-sizeof(long),0)==-1)
					raise(SIGINT);
				m.tipo=p.x+4;
				if(msgsnd(msgid,&m,sizeof(m)-sizeof(long),0)==-1)
					raise(SIGINT);
			}
			p=CRUCE_avanzar_coche(p);
			if(pausa_coche()==-1)
			{
				perror("Error en la funcion pausa_coche");
				raise(SIGINT);
			}

			}
			
		}

		while(p.y>0)
			{
				if(p.y==10)
					{
						Signal(semid,14);
						m.tipo=29;
						if(msgsnd(msgid,&m,sizeof(m)-sizeof(long),MSG_NOERROR)==-1)
							raise(SIGINT);
					}

				if(msgrcv(msgid,&m, sizeof(m)-sizeof(long),p.y+35,MSG_NOERROR)==-1)
					raise(SIGINT);

				if(msgrcv(msgid,&m, sizeof(m)-sizeof(long),p.y+40,MSG_NOERROR)==-1)
					raise(SIGINT);

				if(p.y==13)
				{
					Signal(semid,6);
					Wait(semid,12);

					p=CRUCE_avanzar_coche(p);
					if(pausa_coche()==-1)
						{
							perror("Error en la funcion pausa_coche");
							raise(SIGINT);
						}				
				}

				else
					{
						p=CRUCE_avanzar_coche(p);
						if(pausa_coche()==-1)
							{
								perror("Error en la funcion pausa_coche");
								raise(SIGINT);
						}
					}
				if(p.y==12)
					{
						m.tipo=45;
						if(msgsnd(msgid,&m,sizeof(m)-sizeof(long),0)==-1)
							raise(SIGINT);

						m.tipo=50;
						if(msgsnd(msgid,&m,sizeof(m)-sizeof(long),0)==-1)
							raise(SIGINT);
					}

				m.tipo=p.y+34;
				if(msgsnd(msgid,&m,sizeof(m)-sizeof(long),0)==-1)
					raise(SIGINT);

				m.tipo=p.y+39;
				if(msgsnd(msgid,&m,sizeof(m)-sizeof(long),0)==-1)
					raise(SIGINT);

			}

		for(i=55; i<=61; i++)
			{
				m.tipo=i;
				if(msgsnd(msgid,&m,sizeof(m)-sizeof(long),0)==-1)
					raise(SIGINT);
			}

		if(CRUCE_fin_coche()==-1)
			raise(SIGINT);

		Signal(semid,12);
		Signal(semid,3);
		Signal(semid,2);
		raise(SIGINT);
	}

	else if(p.x==33)
	{ 
		
		while(p.y>0)
		{
			if(p.y==1)
				Wait(semid,10);
			
			if(msgrcv(msgid,&m, sizeof(m)-sizeof(long),p.y+35,0)==-1)
				raise(SIGINT);

			if(msgrcv(msgid,&m, sizeof(m)-sizeof(long),p.y+40,0)==-1)
				raise(SIGINT);

			if(p.y==6)
				{
					Wait(semid,5);

					p=CRUCE_avanzar_coche(p);
					if(pausa_coche()==-1)
						{
							perror("Error en la funcion pausa_coche");
							raise(SIGINT);
						}
					
					m.tipo=p.y+34;
					if(msgsnd(msgid,&m,sizeof(m)-sizeof(long),0)==-1)
						raise(SIGINT);

					m.tipo=p.y+39;
					if(msgsnd(msgid,&m,sizeof(m)-sizeof(long),0)==-1)
						raise(SIGINT);	

					Signal(semid,5);
				}
			else if(p.y==7)
				{
					Wait(semid,3);
					Wait(semid,6);

					p=CRUCE_avanzar_coche(p);
					if(pausa_coche()==-1)
						{
							perror("Error en la funcion pausa_coche");
							raise(SIGINT);
						}

					m.tipo=p.y+34;
					if(msgsnd(msgid,&m,sizeof(m)-sizeof(long),0)==-1)
						raise(SIGINT);
					m.tipo=p.y+39;
					if(msgsnd(msgid,&m,sizeof(m)-sizeof(long),0)==-1)
						raise(SIGINT);	

					Signal(semid,6);
				}

			else if(p.y==13)
				{
					Signal(semid,10);
					Signal(semid,6);
					Wait(semid,12);

					p=CRUCE_avanzar_coche(p);
					if(pausa_coche()==-1)
						{
							perror("Error en la funcion pausa_coche");
							raise(SIGINT);
						}
				}

			else
				{
					p=CRUCE_avanzar_coche(p);
					if(pausa_coche()==-1)
						{
							perror("Error en la funcion pausa_coche");
							raise(SIGINT);
						}
				}

			m.tipo=p.y+34;
			if(msgsnd(msgid,&m,sizeof(m)-sizeof(long),0)==-1)
				raise(SIGINT);

			m.tipo=p.y+39;
			if(msgsnd(msgid,&m,sizeof(m)-sizeof(long),0)==-1)
				raise(SIGINT);	

		}

		for(i=54; i<=61; i++)
			{
				m.tipo=i;
				if(msgsnd(msgid,&m,sizeof(m)-sizeof(long),0)==-1)
					raise(SIGINT);
			}

		if(CRUCE_fin_coche()==-1)
			raise(SIGINT);

		Signal(semid,12);
		Signal(semid,3);
		Signal(semid,2);
		raise(SIGINT);
	}
	else
		{
			Signal(semid,2);
			exit(0);
		}	
}

void zona_peaton(int semid, int msgid)
{
	mensaje m,r;
	struct posiciOn nac, sig, ant;
	
	Wait(semid,11);

	sig=CRUCE_inicio_peatOn_ext(&nac);
	
	while((nac.x == LIM_INF3 && nac.y >=LIM_INF1 && nac.y <= LIM_INF2) || (nac.y == LIM_INF2 && nac.x >=LIM_INF3 && nac.y <= LIM_INF4))
	{
		r.tipo = 100+sig.x*21+sig.y;
		if(msgrcv(msgid,&r,sizeof(r)-sizeof(long),r.tipo,MSG_NOERROR)==-1)
			raise(SIGINT);

			ant=nac;
			nac=sig;
			sig=CRUCE_avanzar_peatOn(sig);
			if(pausa()==-1)
				raise(SIGINT);

		m.tipo=100+ant.x*21+ant.y;
		if(msgsnd(msgid,&m, sizeof(m)-sizeof(long),IPC_NOWAIT)==-1)
			raise(SIGINT);
	}
	Signal(semid,11);
	
	while(sig.y>=0)
	{

		if(sig.x==30 && nac.y>=13 && nac.y<=15)
			{
				Wait(semid,7);
				Wait(semid,14);
				while(sig.x<41)
				{
					r.tipo=100+sig.x*21+sig.y;
					if(msgrcv(msgid,&r,sizeof(r)-sizeof(long),r.tipo,MSG_NOERROR)==-1)
						raise(SIGINT);

					ant=nac;
					nac=sig;

					sig=CRUCE_avanzar_peatOn(sig);
					if(pausa()==-1)
						raise(SIGINT);

					m.tipo=100+ant.x*21+ant.y;
					if(msgsnd(msgid,&m, sizeof(m)-sizeof(long),IPC_NOWAIT)==-1)
						raise(SIGINT);
				}
				Signal(semid,14);
				Signal(semid,7);
			}
		else if(sig.y==11 && nac.x>=21 && nac.x<=27)
			{	
				Wait(semid,8);
				Wait(semid,13);
				while(sig.y>6)
				{
					r.tipo=100+sig.x*21+sig.y;
					if(msgrcv(msgid,&r,sizeof(r)-sizeof(long),r.tipo,MSG_NOERROR)==-1)
						raise(SIGINT);

					ant=nac;
					nac=sig;

					sig=CRUCE_avanzar_peatOn(sig);
					if(pausa()==-1)
						raise(SIGINT);

					m.tipo=100+ant.x*21+ant.y;
					if(msgsnd(msgid,&m, sizeof(m)-sizeof(long),IPC_NOWAIT)==-1)
						raise(SIGINT);
				}
				Signal(semid,13);
				Signal(semid,8);
			}
			
		r.tipo=100+sig.x*21+sig.y;
		if(msgrcv(msgid,&r,sizeof(r)-sizeof(long),r.tipo,MSG_NOERROR)==-1)
			raise(SIGINT);

		ant=nac;
		nac=sig;
		sig=CRUCE_avanzar_peatOn(sig);
		if(pausa()==-1)
			raise(SIGINT);

		m.tipo=100+ant.x*21+ant.y;
		if(msgsnd(msgid,&m, sizeof(m)-sizeof(long),IPC_NOWAIT)==-1)
			raise(SIGINT);			
	}

	if(CRUCE_fin_peatOn()==-1)
		raise(SIGINT);
		
	m.tipo=100+nac.x*21+nac.y;
	if(msgsnd(msgid,&m, sizeof(m)-sizeof(long),IPC_NOWAIT)==-1)
		raise(SIGINT);
		
	Signal(semid,2);
	exit(0);
}

void Wait (int semid, int nsem)
{
	struct sembuf s;
	s.sem_num=nsem;
	s.sem_op=-1;
  	s.sem_flg=0;
  	if(semop(semid,&s,1)==-1)
    	{
      		perror("Operación WAIT en semáforo");
      		raise(SIGINT);
    	}
}

void Signal (int semid, int nsem)
{
	struct sembuf s;
	s.sem_num=nsem;
	s.sem_op=1;
  	s.sem_flg=0;
  	if(semop(semid,&s,1)==-1)
    	{
      		perror("Operación Signal en semáforo");
      		raise(SIGINT);
    	}   		
}
