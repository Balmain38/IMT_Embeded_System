#include "main.h"
#include "adc.h"
#include "cmsis_os.h"
#include "gpio.h"
#include "string.h"
#include "rng.h"
#include "stdio.h"
#include "usart.h"
#include "tim.h"
#include <time.h>
//----------------------
void SystemClock_Config(void);
extern UART_HandleTypeDef huart2;
//=========================================
int state_config = 0;
volatile int period_sensor_1 = 200;
volatile int period_sensor_2 = 300;
volatile int period_sensor_3 = 600;
volatile int period_send = 10000;
volatile int* PERIOD[4];

osMessageQueueId_t Pipe_Reception_Analyse;
osThreadId_t Thread_Send_Data;

#define TAILLE_PIPE_RECEPTION_ANALYSE 20
#define FLAG_SEND_DATA 1
#define INDEX_TIME 3
#define INDEX_TYPE 16
#define INDEX_VALUE 20

char* json_message = "{1:0000000000,2:0,3:0000}";
enum {TENSION = 1, COURANT = 2,TEMPERATURE = 3} T_TYPE_MESURE;

typedef struct{
	uint8_t Type; // nature de valeur
	uint32_t Value; // permet de stocker une valeur rnd
	uint32_t Timestamp;
} T_DATA;

uint16_t val_sensor_1;

GPIO_TypeDef *PORT[] = {LED_SENSOR_1_GPIO_Port,	LED_SENSOR_2_GPIO_Port,	LED_SENSOR_3_GPIO_Port,	LED_FREQ_SEND_DATA_GPIO_Port};
uint16_t PIN[] = {LED_SENSOR_1_Pin,	LED_SENSOR_2_Pin, LED_SENSOR_3_Pin, LED_FREQ_SEND_DATA_Pin};

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
osThreadAttr_t Config_Thread_Send={
	.name="Thread_Sensor_3",
	.priority=osPriorityLow, // le niveau est 8 (voir cmsis_os2.h)
	.stack_size=128*4 // Pile de 128 mots de 32 bits
};
osThreadAttr_t Config_Thread_Config_phy={
	.name="Thread_Config_phy",
	.priority=osPriorityNormal, // le niveau est 8 (voir cmsis_os2.h)
	.stack_size=128*4 // Pile de 128 mots de 32 bits
};
osThreadAttr_t Config_Thread_Config_uart={
	.name="Thread_Config_uart",
	.priority=osPriorityNormal, // le niveau est 8 (voir cmsis_os2.h)
	.stack_size=128*4 // Pile de 128 mots de 32 bits
};


int _write(int P_Flux, char* P_Message, int P_Taille) {
	HAL_StatusTypeDef Etat = HAL_UART_Transmit(&huart2, (uint8_t *) P_Message, P_Taille, HAL_MAX_DELAY);
	if (Etat == HAL_OK){ return P_Taille;}
	else{return -1;}
}

void Fonction_Thread_Sensor_1(void* P_Info){
	HAL_ADC_Start(&hadc1);
	HAL_ADC_PollForConversion(&hadc1, 1);
	val_sensor_1 = HAL_ADC_GetValue(&hadc1);
	printf('Ici la street %d\n\r', val_sensor_1);
	T_DATA Data={.Type=1};
	while(1) {
		HAL_ADC_Start(&hadc1);
		// Poll ADC1 Perihperal & TimeOut = 1mSec
		HAL_ADC_PollForConversion(&hadc1, 1);
		Data.Timestamp = (uint32_t)(time(NULL));
		Data.Value = val_sensor_1;
		osMessageQueuePut (Pipe_Reception_Analyse,(void*)&Data, 0, osWaitForever);
		osDelay(period_sensor_1);
	}
	osThreadTerminate(NULL);
}

void Fonction_Thread_Sensor_2(void* P_Info){
	T_DATA Data={.Type=2};
	while(1){
		HAL_RNG_GenerateRandomNumber(&hrng, &Data.Value);
		Data.Value %= 12;
		Data.Timestamp = (uint32_t)(time(NULL));
		osMessageQueuePut (Pipe_Reception_Analyse,(void*)&Data, 0, osWaitForever);
		osDelay(period_sensor_2);
	}
	osThreadTerminate(NULL);
}

void Fonction_Thread_Sensor_3(void* P_Info){
	T_DATA Data={.Type=3};
	while(1){
		HAL_RNG_GenerateRandomNumber(&hrng, &Data.Value);
		Data.Value %= 500;
		Data.Timestamp = (uint32_t)(time(NULL));
		osMessageQueuePut (Pipe_Reception_Analyse,(void*)&Data, 0, osWaitForever);
		osDelay(period_sensor_3);
	}
	osThreadTerminate(NULL);
}

void Fonction_Thread_Send(void* P_Info){
	T_DATA Data;
	while(1){
		osThreadFlagsWait (FLAG_SEND_DATA, osFlagsWaitAll, HAL_MAX_DELAY);
		int i;
		osMessageQueueGetCount(&i);
		while(i--){
			if (osMessageQueueGet(Pipe_Reception_Analyse,(void*) &Data, 0, osWaitForever) == osOK){
				memcpy(json_message + sizeof(char) * INDEX_TIME, itoa(Data.Timestamp), sizeof(Data.Timestamp));
				memcpy(json_message + sizeof(char) * INDEX_VALUE, itoa(Data.Value), sizeof(Data.Value));
				memcpy(json_message + sizeof(char) * INDEX_TYPE, itoa(Data.Type), sizeof(Data.Type));
				// send via UART
				printf("%s", json_message);
			}
		}
	}
	osThreadTerminate(NULL);
}


// Intéruption Timer pour Send Data
void Callback_TIM_5(TIM_HandleTypeDef* P_Timer){
	if (P_Timer == &htim5) {
		osThreadFlagsSet(Thread_Send_Data,FLAG_SEND_DATA);
	}
}

// Intéruption Bouton Bleu pour Send Data
void HAL_GPIO_EXTI_Callback(uint16_t P_Pin){
	if (P_Pin == BTN_SEND_DATA_Pin){
		osThreadFlagsSet (Thread_Send_Data,FLAG_SEND_DATA);
	}
	else if (P_Pin == BTN_SELECT_Pin){
		if(state_config != 0){
			PORT[state_config]->ODR &= ~PIN[state_config];
		}
		state_config ++;
		if(state_config > 4){state_config = 0;}
		if(state_config != 0){
			PORT[state_config]->ODR |= PIN[state_config];
		}
	}
	else if (P_Pin == BTN_PLUS_Pin && state_config != 0){
		if (state_config != 4 && *(PERIOD[state_config]) < 2000) PERIOD[state_config] += 100;
		else if (state_config == 4 && *(PERIOD[state_config]) < 10000) PERIOD[state_config] += 1000;
	}
	else if (P_Pin == BTN_MINUS_Pin && state_config != 0){
		if (state_config != 4 && *(PERIOD[state_config]) > 100) PERIOD[state_config] -= 100;
		else if (state_config == 4 && *(PERIOD[state_config]) > 1000) PERIOD[state_config] -= 1000;
	}
}

int main(){
	PERIOD[0] = &period_sensor_1;
	PERIOD[1] = &period_sensor_2;
	PERIOD[2] = &period_sensor_3;
	PERIOD[3] = &period_send;
	HAL_Init();
	SystemClock_Config();
	MX_GPIO_Init();
	MX_ADC1_Init();
	MX_RNG_Init();
	MX_USART2_UART_Init();
	osKernelInitialize();
	Pipe_Reception_Analyse= osMessageQueueNew (TAILLE_PIPE_RECEPTION_ANALYSE, sizeof(uint8_t),NULL);
	osThreadNew(Fonction_Thread_Sensor_1, NULL, &Config_Thread_Sensor_1);
	osThreadNew(Fonction_Thread_Sensor_2, NULL, &Config_Thread_Sensor_2);
	osThreadNew(Fonction_Thread_Sensor_3, NULL, &Config_Thread_Sensor_3);
	Thread_Send_Data = osThreadNew(Fonction_Thread_Send, NULL, &Config_Thread_Send);
	osKernelStart();
	while(1);
}
