#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <string.h>
#include <locale.h>
#include "cruce2.h"

#define VERDADERO 1
#define FALSO 0
#define MAXMP 1150
#define MAXMC 100
#define LIM_INF1 13
#define LIM_INF2 16
#define LIM_INF3 0
#define LIM_INF4 25

int (*C_Inicio) (int, int);
int (*C_PonSem) (int, int);
struct posiciOn (*C_Inicio_Peaton) (void);
struct posiciOn (*C_Inicio_Coche) (void);
struct posiciOn (*C_ACoche) (struct posiciOn);
struct posiciOn (*C_APeaton) (struct posiciOn);
void (*Pon_error) (const char *);

FARPROC CRUCE_fin, C_GestorInicio, C_NuevoPro, C_FCoche, C_FPeaton, Pausa, P_Coche, Refresh;

DWORD WINAPI Ciclo_sem(LPVOID );
DWORD WINAPI Zona_Peaton(LPVOID );
DWORD WINAPI Zona_Coche(LPVOID );

typedef struct datos{
	HANDLE SnPro, S_P1, S_P2;
	HANDLE M_NacP, M_P1, M_P2, M_Nac_CH, M_Nac_CV, M_C;
	HANDLE S_C1, S_C2, S_P1_C;
	HANDLE MP[MAXMP];
	HANDLE MC[MAXMC];
}datos;

int main (int argc, char * argv[]){
	datos *d;
	int nPro,nV, id, i;
	int varHilos;
	HINSTANCE libreria;
	setlocale(LC_CTYPE,"SPANISH");
	if(argc!=3)
		{
			printf("El número de argumentos es incorrecto.\n");
			return 1;
		}
	else
		{
			nPro=atoi(argv[1]);
			nV=atoi(argv[2]);
			if(nPro<=0 || nPro>49 || nV<0)
				{
					printf("Error en el valor de los párametros.\n");
					return 2;
				}
				
			if((libreria = LoadLibrary("cruce2.dll"))==NULL)
				{
					PERROR("No se ha podido cargar la libreria\n");
					return 3;
				} 
			else
			{
				varHilos=nPro-2;
				if((C_Inicio= (int (*) (int,int)) GetProcAddress(libreria,"CRUCE_inicio"))==NULL) 
					{
						printf("Error en mapear CRUCE_inicio\n");
						return 4;
					}
				
				if((CRUCE_fin=GetProcAddress(libreria,"CRUCE_fin"))==NULL) 
					{
						printf("Error en mapear CRUCE_fin\n");
						return 4;
					}
				
				if((C_GestorInicio=GetProcAddress(libreria,"CRUCE_gestor_inicio"))==NULL) 
					{
						printf("Error en mapear CRUCE_gestor_inicio\n");
						return 4;
					}
				
				if((C_PonSem= (int (*) (int,int)) GetProcAddress(libreria,"CRUCE_pon_semAforo"))==NULL) 
					{
						printf("Error en mapear CRUCE_pon_semAforo\n");
						return 4;
					}
				
				if((C_NuevoPro=GetProcAddress(libreria,"CRUCE_nuevo_proceso"))==NULL) 
					{
						printf("Error en mapear CRUCE_nuevo_proceso\n");
						return 4;
					}
				
				if(( C_Inicio_Coche=(struct posiciOn (*) (void) ) GetProcAddress(libreria,"CRUCE_inicio_coche"))==NULL) 
					{
						printf("Error en mapear CRUCE_inicio_coche\n");
						return 4;
					}
				
				if((C_ACoche=(struct posiciOn (*) (struct posiciOn) )GetProcAddress(libreria,"CRUCE_avanzar_coche"))==NULL) 
					{
						printf("Error en mapear CRUCE_avanzar_coche\n");
						return 4;
					}
				
				if((C_FCoche=GetProcAddress(libreria,"CRUCE_fin_coche"))==NULL) 
					{
						printf("Error en mapear CRUCE_fin_coche\n");
						return 4;
					}
				
				if((C_Inicio_Peaton=(struct posiciOn (*) (void) )GetProcAddress(libreria,"CRUCE_nuevo_inicio_peatOn"))==NULL) 
					{
						printf("Error en mapear CRUCE_nuevo_inicio_peatOn\n");
						return 4;
					}
				
				if((C_APeaton=(struct posiciOn (*) (struct posiciOn) ) GetProcAddress(libreria,"CRUCE_avanzar_peatOn"))==NULL) 
					{
						printf("Error en mapear CRUCE_avanzar_peatOn\n");
						return 4;
					}
				
				if((C_FPeaton=GetProcAddress(libreria,"CRUCE_fin_peatOn"))==NULL) 
					{
						printf("Error en mapear CRUCE_fin_peatOn\n");
						return 4;
					}
				
				if((Pausa=GetProcAddress(libreria,"pausa"))==NULL) 
					{
						printf("Error en mapear pausa\n");
						return 4;
					}
				
				if((P_Coche=GetProcAddress(libreria,"pausa_coche"))==NULL) 
					{
						printf("Error en mapear pausa_coche\n");
						return 4;
					}
				
				if((Refresh=GetProcAddress(libreria,"refrescar"))==NULL) 
					{
						printf("Error en mapear refrescar\n");
						return 4;
					}
				
				if((Pon_error=(void (*) (const char*))GetProcAddress(libreria,"pon_error"))==NULL) 
					{
						printf("Error en mapear pon_error\n");
						return 4;
					}
				if((d=(datos *)malloc(sizeof(datos)))==NULL)
					{
						free(d);
						FreeLibrary(libreria);
						return 5;
					}
				if(C_Inicio(nV,nPro)==-1)
					{
							PERROR("Error en la funcioón Cruce Inicio");
							return 6;
					}
					
				d->SnPro=CreateSemaphore(NULL,nPro-2,nPro-2,NULL);	
				d->S_P1=CreateSemaphore(NULL,0,1,NULL);
				d->S_P2=CreateSemaphore(NULL,0,1,NULL);
				d->S_C1=CreateSemaphore(NULL,0,1,NULL);
				d->S_C2=CreateSemaphore(NULL,0,1,NULL);
				d->S_P1_C=CreateSemaphore(NULL,1,1,NULL);
				d->M_NacP=CreateMutex(NULL,0,NULL);
				d->M_P1=CreateMutex(NULL,0,NULL);
				d->M_P2=CreateMutex(NULL,0,NULL);
				d->M_Nac_CH=CreateMutex(NULL,0,NULL);
				d->M_Nac_CV=CreateMutex(NULL,0,NULL);
				d->M_C=CreateMutex(NULL,0,NULL);
				for(i=0; i<MAXMP;i++)
					d->MP[i]=CreateMutex(NULL,0,NULL);
				for(i=0;i<MAXMC;i++)
					d->MC[i]=CreateMutex(NULL,0,NULL);	
				
				//Crear hilo para el ciclo semáforico
				if(CreateThread(NULL, 0, Ciclo_sem , d, 0,NULL)==NULL)
					{
						PERROR("Error al crear el hilo del ciclo Semáforico");
						exit(7);
					}
				while(1)
				{
					if((WaitForSingleObject(d->SnPro,INFINITE))==WAIT_FAILED)
						{
							PERROR("Error en el wait de control de procesos");
							free(d);
							FreeLibrary(libreria);
							exit(11);
						}
					id=C_NuevoPro();
					if(id==PEAToN)
					{
						if(CreateThread(NULL, 0, Zona_Peaton, d, 0, NULL)==NULL)
							{
								PERROR("Error al crear el hilo para un peaton");
								exit(7);
							}
					}
					else
					{
						if(CreateThread(NULL, 0, Zona_Coche, d, 0, NULL)==NULL)
							{
								PERROR("Error al crear el hilo para un Coche");
								exit(7);
							}	
					}
				}		
		}
	}
	CRUCE_fin();
	FreeLibrary(libreria);
	return 0;	
}


DWORD WINAPI Ciclo_sem(LPVOID parametros )
{
	int i;
	datos * d;
	d=(datos*) parametros;
	
	if(C_GestorInicio()==-1)
		exit(6);
		
	//PreCiclo
	if(C_PonSem(SEM_C1, VERDE) == -1)
		exit(6);

	if(C_PonSem(SEM_C2, ROJO) == -1)
		exit(6);

	if(C_PonSem(SEM_P1, ROJO) == -1)
		exit(6);

	if(C_PonSem(SEM_P2, VERDE) == -1)
		exit(6);
		
	while(1) 
	{
		//InterFase 1->2
		if(C_PonSem(SEM_C1, AMARILLO)==-1)
			exit(6);
				
		for(i=0;i<2;i++)
			Pausa();
		
		//FASE 2
		if(C_PonSem(SEM_C2,VERDE) ==-1 || C_PonSem(SEM_C1, ROJO)==-1 || C_PonSem(SEM_P2, ROJO)==-1)
			exit(6);
			
		if(!(ReleaseSemaphore(d->S_C2,1,NULL)))
			exit(10);		
		for(i=0;i<9;i++)
			Pausa();
		if((WaitForSingleObject(d->S_C2,INFINITE))==WAIT_FAILED)
			exit(11);
			
		//InterFase 2->3
		if(C_PonSem(SEM_C2, AMARILLO)==-1)
			exit(6);
			
		for(i=0;i<2;i++)
			Pausa();
		
		//FASE 3
		if(C_PonSem(SEM_C2, ROJO)==-1 || C_PonSem(SEM_P1, VERDE)==-1)
		exit(6);
		
		if(!(ReleaseSemaphore(d->S_P1,1,NULL)))
			exit(10);
		if((WaitForSingleObject(d->S_P1_C,INFINITE))==WAIT_FAILED)
			exit(11);
				
		for(i=0;i<12;i++)
			Pausa();
			
		if(!(ReleaseSemaphore(d->S_P1_C,1,NULL)))
			exit(10);	
		if((WaitForSingleObject(d->S_P1,INFINITE))==WAIT_FAILED)
			exit(11);
			
		//FASE 1
		if(C_PonSem(SEM_C1, VERDE)==-1 || C_PonSem(SEM_P1, ROJO)==-1 || C_PonSem(SEM_P2, VERDE)==-1)			
			exit(6);
			
		if(!(ReleaseSemaphore(d->S_C1,1,NULL)))
			exit(10);
		if(!(ReleaseSemaphore(d->S_P2,1,NULL)))
			exit(10);
			
		for(i=0;i<6;i++)
			Pausa();
				
		if((WaitForSingleObject(d->S_P2,INFINITE))==WAIT_FAILED)
			exit(11);
		if((WaitForSingleObject(d->S_C1,INFINITE))==WAIT_FAILED)
			exit(11);
	}
}

DWORD WINAPI Zona_Peaton(LPVOID parametros )
{
	int num,flag=1;
	datos * d;
	d=(datos*) parametros;
	struct posiciOn sig, ant, act;

	sig=C_Inicio_Peaton();
	
	if((WaitForSingleObject(d->M_NacP,INFINITE))==WAIT_FAILED)
		exit(11);
	while((sig.x == LIM_INF3 && sig.y >=LIM_INF1 && sig.y <= LIM_INF2) || (sig.y == LIM_INF2 && sig.x >=LIM_INF3 && sig.y <= LIM_INF4))
	{
		if((WaitForSingleObject(d->MP[sig.x*21+sig.y],INFINITE))==WAIT_FAILED)
			exit(13);
			
		ant=sig;
		sig=C_APeaton(sig);
		if(Pausa()==-1)
			PERROR("Error en la pausa");
		if(flag)
			flag=0;
		else	
			if(!ReleaseMutex(d->MP[act.x*21+act.y]))
				exit(12);
		act=ant;
	}
	if(!ReleaseMutex(d->M_NacP))
		exit(10);
		
	while(sig.y>0 && sig.x>0 && sig.y<21 && sig.x<50)
	{
		if(sig.x==30 && ant.y>=13 && ant.y<=15)
			{	
				if((WaitForSingleObject(d->S_P1,INFINITE))==WAIT_FAILED)
					exit(11);
				if((WaitForSingleObject(d->M_P1,INFINITE))==WAIT_FAILED)
					exit(13);
				while(sig.x<41)
				{
					if((WaitForSingleObject(d->MP[sig.x*21+sig.y],INFINITE))==WAIT_FAILED)
						exit(13);
							
					ant=sig;
					sig=C_APeaton(sig);
					if(Pausa()==-1)
						PERROR("Error en la pausa");
			
					if(!ReleaseMutex(d->MP[act.x*21+act.y]))
						exit(12);
					act=ant;
				}
				if(!ReleaseMutex(d->M_P1))
					exit(12);
				if(!(ReleaseSemaphore(d->S_P1,1,NULL)))
					exit(10);
			}
		else if(sig.y==11 && ant.x>=21 && ant.x<=27)
			{
				if((WaitForSingleObject(d->S_P2,INFINITE))==WAIT_FAILED)
					exit(11);
				if((WaitForSingleObject(d->M_P2,INFINITE))==WAIT_FAILED)
					exit(13);
				
				while(sig.y>6)
				{
					if((WaitForSingleObject(d->MP[sig.x*21+sig.y],INFINITE))==WAIT_FAILED)
						exit(13);
						
					ant=sig;
					sig=C_APeaton(sig);
					if(Pausa()==-1)
						PERROR("Error en la pausa");
						
				if(!ReleaseMutex(d->MP[act.x*21+act.y]))
						exit(12);
					act=ant;
				}
				if(!ReleaseMutex(d->M_P2))
					exit(12);
				if(!(ReleaseSemaphore(d->S_P2,1,NULL)))
					exit(10);
			}
			else
			{
				if((WaitForSingleObject(d->MP[sig.x*21+sig.y],INFINITE))==WAIT_FAILED)
					exit(13);
					
				ant=sig;
				sig=C_APeaton(sig);
				if(Pausa()==-1)
					PERROR("Error en la pausa");
					
			if(!ReleaseMutex(d->MP[act.x*21+act.y]))
						exit(12);
					act=ant;
			}
	}
		
	if(C_FPeaton()==-1)
		exit(6);
		
	if(!(ReleaseSemaphore(d->SnPro,1,NULL)))
		exit(10);
		
	CloseHandle(GetCurrentThread());
	TerminateThread(GetCurrentThread(),0);
}

DWORD WINAPI Zona_Coche(LPVOID parametros )
{
	datos * d;
	d=(datos*) parametros;
	struct posiciOn p, ant;
	int Carril;
	
	p=C_Inicio_Coche();
	if(p.x==-3)
	{
		if((WaitForSingleObject(d->M_Nac_CH,INFINITE))==WAIT_FAILED)
			exit(13);
		Carril=VERDADERO;
	}
	else
	{
		if((WaitForSingleObject(d->M_Nac_CV,INFINITE))==WAIT_FAILED)
			exit(13);
		Carril=FALSO;
	}
	while(p.y>=0)
	{	
		if(p.x==13)
		{
			if((WaitForSingleObject(d->S_C2,INFINITE))==WAIT_FAILED)
				exit(11);
			if((WaitForSingleObject(d->M_P2,INFINITE))==WAIT_FAILED)
				exit(13);
			if(!(ReleaseMutex(d->M_Nac_CH)))
				exit(12);
			if((WaitForSingleObject(d->M_C,INFINITE))==WAIT_FAILED)
				exit(13);
		}
		if(p.x==15)
			if(!(ReleaseSemaphore(d->S_C2,1,NULL)))
				exit(9);  
		if(p.x==31 && Carril)
		{
			if(!ReleaseMutex(d->M_P2))		
				exit(12);
			Carril=FALSO;
		}
		if(p.y==13)
		{
			if((WaitForSingleObject(d->M_P1,INFINITE))==WAIT_FAILED)
				exit(13);
			if((WaitForSingleObject(d->S_P1_C,INFINITE))==WAIT_FAILED)
				exit(11);	
		}	
		if(p.y==20)
		{
			if(!ReleaseMutex(d->M_P1))
				exit(12);
			if(!ReleaseMutex(d->M_C))
				exit(12);
			if(!(ReleaseSemaphore(d->S_P1_C,1,NULL)))
				exit(10);
		}
		if(p.y==6)
			if((WaitForSingleObject(d->S_C1,INFINITE))==WAIT_FAILED)
				exit(11);
		if(p.y==7)
		{
			if(!(ReleaseMutex(d->M_Nac_CV)))
				exit(12);
			if((WaitForSingleObject(d->M_C,INFINITE))==WAIT_FAILED)
				exit(13);	
		}
		if(p.y==9)
			if(!(ReleaseSemaphore(d->S_C1,1,NULL)))
				exit(16);
		if(Carril)  //carril izq
		{
			if((WaitForSingleObject(d->MC[p.x+4],INFINITE))==WAIT_FAILED)
				exit(13);	
		}
		else 	//carril vertical
		{
			if((WaitForSingleObject(d->MC[p.y+40],INFINITE))==WAIT_FAILED)
				exit(13);	
		}	
		ant=p;
		p=C_ACoche(p);
		if(P_Coche()==-1)
			exit(14);
		if(Carril)
		{
			if((WaitForSingleObject(d->MC[p.x+4],INFINITE))==WAIT_FAILED)
				exit(13);
			if(!(ReleaseMutex(d->MC[ant.x+4])))
				exit(12);
		}
		else
		{
			if((WaitForSingleObject(d->MC[p.y+40],INFINITE))==WAIT_FAILED)
				exit(13);
			if(!(ReleaseMutex(d->MC[ant.y+40])))
				exit(12);
		}	
	}	
	if(C_FCoche()==-1)
		exit(6);		
	if(!(ReleaseSemaphore(d->SnPro,1,NULL)))
		exit(10);
	CloseHandle(GetCurrentThread());
	TerminateThread(GetCurrentThread(),0);
}


