#include "main.h"
#include "cmsis_os.h"
#include "gpio.h"
#include "string.h"
#include "rng.h"
#include "stdio.h"
#include <time.h>
//----------------------
void SystemClock_Config(void);
extern UART_HandleTypeDef huart2;
//=========================================
int state_config = 0;
volatile int period_sensor_1 = 200;
volatile int period_sensor_2 = 300;
volatile int period_sensor_3 = 600;

#define TAILLE_PIPE_RECEPTION_ANALYSE 20
osMessageQueueId_t Pipe_Reception_Analyse;
osSemaphoreId_t Semaphore_Queue;

enum {TENSION=1, COURANT=2,TEMPERATURE=3}T_TYPE_MESURE;
typedef struct{
	uint8_t Type; // nature de valeur
	uint32_t Value; // permet de stocker une valeur rnd
	time_t Timestamp;
}T_DATA;

// ---------
osThreadAttr_t Config_Thread_Sensor_1={
	.name="Thread_Sensor_1",
	.priority=osPriorityHigh, // le niveau est 8 (voir cmsis_os2.h)
	.stack_size=128*4 // Pile de 128 mots de 32 bits
};
osThreadAttr_t Config_Thread_Sensor_2={
	.name="Thread_Sensor_2",
	.priority=osPriorityHigh, // le niveau est 8 (voir cmsis_os2.h)
	.stack_size=128*4 // Pile de 128 mots de 32 bits
};
osThreadAttr_t Config_Thread_Sensor_3={
	.name="Thread_Sensor_3",
	.priority=osPriorityHigh, // le niveau est 8 (voir cmsis_os2.h)
	.stack_size=128*4 // Pile de 128 mots de 32 bits
};
void Ecriture_Queue(T_DATA Data){
	// ENtréé en section critique
	int Etat= osSemaphoreAcquire (Semaphore_Queue, 0);
	osMessageQueuePut (Pipe_Reception_Analyse,(void*)&Data, 0, osWaitForever);
	if (Etat !=osOK) printf("[Debug] Erreur Queue\n") ;
	osSemaphoreRelease(Semaphore_Queue);
	// Sortie de section critique
}
void Fonction_Thread_Sensor_1(void* P_Info){
	HAL_ADC_Start(&hadc1);
	HAL_ADC_PollForConversion(&hadc1, 1);
	val_sensor_1 = HAL_ADC_GetValue(&hadc1);
	T_DATA Data={.Type=1};
	while(1) {
		HAL_ADC_Start(&hadc1);
		// Poll ADC1 Perihperal & TimeOut = 1mSec
		HAL_ADC_PollForConversion(&hadc1, 1);
		Data.Timestamp = time(NULL);
		Data.Value = val_sensor_1;
		Ecriture_Queue(Data);
	}
	osThreadTerminate(NULL);
}
void Fonction_Thread_Sensor_2(void* P_Info){
	T_DATA Data={.Type=2};
	while(1){
		HAL_RNG_GenerateRandomNumber(&hrng, &Data.Value);
		Data.Value %=12;
		Data.Timestamp = time(NULL);
		Ecriture_Queue(Data);
	}
	osThreadTerminate(NULL);
}
void Fonction_Thread_Sensor_3(void* P_Info){
	T_DATA Data={.Type=3};
	while(1){
		HAL_RNG_GenerateRandomNumber(&hrng, &Data.Value);
		Data.Value %=500;
		Data.Timestamp = time(NULL);
		Ecriture_Queue(Data);
	}
	osThreadTerminate(NULL);
}
//=========================================
int main(){
	HAL_Init();
	SystemClock_Config();
	MX_GPIO_Init();
	MX_ADC1_Init();
	MX_RNG_Init();
	osKernelInitialize();
	Pipe_Reception_Analyse= osMessageQueueNew (TAILLE_PIPE_RECEPTION_ANALYSE, sizeof(uint8_t),NULL);
	Semaphore_Queue = osSemaphoreNew (1,0,NULL);
	osThreadNew(Fonction_Thread_Sensor_1, NULL, &Config_Thread_Sensor_1);
	osThreadNew(Fonction_Thread_Sensor_2, NULL, &Config_Thread_Sensor_2);
	osThreadNew(Fonction_Thread_Sensor_3, NULL, &Config_Thread_Sensor_3);
	osKernelStart();
	while(1);
}
