#include "main.h"
#include "adc.h"
#include "cmsis_os.h"
#include "gpio.h"
#include "rng.h"
#include "rtc.h"
#include "string.h"
#include "stdio.h"
#include <stdlib.h>
#include "tim.h"
#include "usart.h"

#define TAILLE_PIPE_RECEPTION_ANALYSE 200
#define FLAG_SEND_DATA 1
#define FLAG_WATCH_QUEUE 2
#define FLAG_BUZZER 3
#define INDEX_TIME 3
#define INDEX_TYPE 16
#define INDEX_VALUE 20
#define MESSAGE_RECEIVE_SIZE 30

volatile int period_sensor_1 = 200;
volatile int period_sensor_2 = 300;
volatile int period_sensor_3 = 600;
volatile int period_send_data = 8000;
char receive_buffer[MESSAGE_RECEIVE_SIZE];
int state_config = 0;
volatile int* PERIOD[4];
const unsigned long DEBOUNCE_TIME = 300; // 0.3 sec
unsigned long BTN_TIME = 0;

osMessageQueueId_t Pipe_Reception_Analyse;
osThreadId_t Thread_Send_Data;
osThreadId_t Thread_Watch_Queue;
osThreadId_t Thread_Buzzer;
osThreadId_t Thread_Config;

// volatile char* json_message = "{1:0000000000,2:0,3:0000}";
enum {TENSION = 1, COURANT = 2,TEMPERATURE = 3} T_TYPE_MESURE;

GPIO_TypeDef *PORT[] = {LED_SENSOR_1_GPIO_Port,	LED_SENSOR_2_GPIO_Port,	LED_SENSOR_3_GPIO_Port,	LED_FREQ_SEND_DATA_GPIO_Port};
uint16_t PIN[] = {LED_SENSOR_1_Pin,	LED_SENSOR_2_Pin, LED_SENSOR_3_Pin, LED_FREQ_SEND_DATA_Pin};

#pragma pack(1)
typedef struct{
	uint8_t Type; // nature de valeur
	RTC_DateTypeDef Date; // permet de stocker une valeur rnd
	RTC_TimeTypeDef Hour;
	uint32_t Value;
} T_DATA;
#pragma pack()

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
	.name="Thread_Sensor_Send",
	.priority=osPriorityLow, // le niveau est 8 (voir cmsis_os2.h)
	.stack_size=256*4 // Pile de 128 mots de 32 bits
};
osThreadAttr_t Config_Thread_Watch_Queue={
	.name="Thread_Watch_Queue",
	.priority=osPriorityHigh, // le niveau est 8 (voir cmsis_os2.h)
	.stack_size=128*4 // Pile de 128 mots de 32 bits
};
osThreadAttr_t Config_Thread_Buzzer={
	.name="Thread_Buzzer",
	.priority=osPriorityLow, // le niveau est 8 (voir cmsis_os2.h)
	.stack_size=128*4 // Pile de 128 mots de 32 bits
};


// analog read
void Fonction_Thread_Sensor_1(void* P_Info){
	T_DATA Data={.Type=1};
	while(1) {
		HAL_ADC_Start(&hadc1);
		HAL_ADC_PollForConversion(&hadc1, 1);
		uint16_t val_sensor_1 = HAL_ADC_GetValue(&hadc1);
		Data.Value = val_sensor_1;
		HAL_RTC_GetDate(&hrtc, &Data.Date, RTC_FORMAT_BIN);
		HAL_RTC_GetTime(&hrtc, &Data.Hour, RTC_FORMAT_BIN);
		osMessageQueuePut(Pipe_Reception_Analyse, (void*)&Data, 0, osWaitForever);
		osThreadFlagsSet(Thread_Watch_Queue, FLAG_WATCH_QUEUE);
		osDelay(period_sensor_3);
	}
	osThreadTerminate(NULL);
}
// RNG
void Fonction_Thread_Sensor_2(void* P_Info){
	T_DATA Data={.Type=2};
	while(1){
		HAL_RNG_GenerateRandomNumber(&hrng, &Data.Value);
		Data.Value %= 12;
		HAL_RTC_GetDate(&hrtc, &Data.Date, RTC_FORMAT_BIN);
		HAL_RTC_GetTime(&hrtc, &Data.Hour, RTC_FORMAT_BIN);
		osMessageQueuePut(Pipe_Reception_Analyse, (void*)&Data, 0, osWaitForever);
		osThreadFlagsSet(Thread_Watch_Queue, FLAG_WATCH_QUEUE);
		osDelay(period_sensor_1);
	}
	osThreadTerminate(NULL);
}
// RNG
void Fonction_Thread_Sensor_3(void* P_Info){
	T_DATA Data={.Type=3};
	while(1){
		HAL_RNG_GenerateRandomNumber(&hrng, &Data.Value);
		Data.Value %= 500;
		HAL_RTC_GetDate(&hrtc, &Data.Date, RTC_FORMAT_BIN);
		HAL_RTC_GetTime(&hrtc, &Data.Hour, RTC_FORMAT_BIN);
		osMessageQueuePut(Pipe_Reception_Analyse, (void*)&Data, 0, osWaitForever);
		osThreadFlagsSet(Thread_Watch_Queue, FLAG_WATCH_QUEUE);
		osDelay(period_sensor_2);
	}
	osThreadTerminate(NULL);
}

void Fonction_Thread_Send(void* P_Info){
	T_DATA Data;
	// char* json_message = "{1:0000000000,2:0,3:0000}";
	while(1){
		osThreadFlagsWait(FLAG_SEND_DATA, osFlagsWaitAll, period_send_data);
		int i = osMessageQueueGetCount(Pipe_Reception_Analyse);
		while(i--){
			osMessageQueueGet(Pipe_Reception_Analyse, &Data, NULL, osWaitForever);
			/* char * buffer[sizeof(Data.Value)];

			char * vide[10];
			itoa(Data.Value, buffer, 10);
			// printf("Received data : %s\n\r", buffer);
			// memcpy(json_message + sizeof(char) * INDEX_TIME, itoa(Data.Timestamp), sizeof(Data.Timestamp));
			// memcpy(json_message, buffer, sizeof(Data.Value));
			// memcpy(json_message + sizeof(char) * INDEX_TYPE, itoa(Data.Type), sizeof(Data.Type));
			// send via UART*/

			printf("{1:%d/%d/%d-%d:%d:%d,2:%d,3:%d}", Data.Date.Date, Data.Date.Month, Data.Date.Year, Data.Hour.Hours, Data.Hour.Minutes, Data.Hour.Seconds, Data.Type, Data.Value);
		}
		osThreadFlagsSet(Thread_Buzzer, FLAG_BUZZER);
	}
	osThreadTerminate(NULL);
}

void Fonction_Thread_Watch_Queue(void* P_Info){
	while (1){
		osThreadFlagsWait(FLAG_WATCH_QUEUE, osFlagsWaitAll, HAL_MAX_DELAY);
		if (osMessageQueueGetSpace(Pipe_Reception_Analyse) == 0){
			osThreadFlagsSet(Thread_Send_Data, FLAG_SEND_DATA);
		}
	}
}

// RNG
void Fonction_Thread_Buzzer(void* P_Info){
	while (1){
		osThreadFlagsWait(FLAG_BUZZER, osFlagsWaitAll, HAL_MAX_DELAY);
		TIM1->CCR1 = 10;
		HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
		osDelay(200);
		HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
	}
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
	char subbuff_5[5];
	char subbuff_6[6];
	memcpy(subbuff_5, &receive_buffer[3], 4);
	subbuff_5[4] = '\0';
	period_sensor_1 = atoi(subbuff_5);
	memcpy(subbuff_5, &receive_buffer[10], 4);
	subbuff_5[4] = '\0';
	period_sensor_2 = atoi(subbuff_5);
	memcpy(subbuff_5, &receive_buffer[17], 4);
	subbuff_5[4] = '\0';
	period_sensor_3 = atoi(subbuff_5);
	memcpy(subbuff_6, &receive_buffer[24], 5);
	subbuff_6[5] = '\0';
	period_send_data = atoi(subbuff_6);
	HAL_UART_Receive_IT(&huart2, (void*)&receive_buffer, MESSAGE_RECEIVE_SIZE);
}

void handler_btn_select(){
	// Testing purpose
	// printf("Bouton select, state config : %d\n\r", mess, state_config);*/
	if(state_config != 0){
		PORT[state_config-1]->ODR &= ~PIN[state_config-1];
	}
	state_config++;
	if(state_config > 4){
		state_config = 0;
	}
	if(state_config != 0){
		PORT[state_config-1]->ODR |= PIN[state_config-1];
	}
}

void handler_btn_plus(){
	// Testing purpose
	// printf("Bonton plus, state config : %d, période : %d\n\r", state_config, *(PERIOD[state_config-1]));
	if (state_config != 4 && *(PERIOD[state_config-1]) < 2000) *(PERIOD[state_config-1]) += 100;
	else if (state_config == 4 && *(PERIOD[state_config-1]) < 10000) *(PERIOD[state_config-1]) += 1000;
}

void handler_btn_minus(){
	// Testing purpose
	// printf("Bonton minus, state config : %d, période : %d\n\r", state_config, *(PERIOD[state_config-1]));
	if (state_config != 4 && *(PERIOD[state_config-1]) > 100) *(PERIOD[state_config-1]) -= 100;
	else if (state_config == 4 && *(PERIOD[state_config-1]) > 1000) *(PERIOD[state_config-1]) -= 1000;
}

void HAL_GPIO_EXTI_Callback(uint16_t P_Pin){
	unsigned long L_Temps_Actuel = HAL_GetTick();
	if ((L_Temps_Actuel - BTN_TIME) > DEBOUNCE_TIME) { // anti-rebonds
		BTN_TIME = L_Temps_Actuel;
		if (P_Pin == BTN_SEND_DATA_Pin){
			osThreadFlagsSet(Thread_Send_Data, FLAG_SEND_DATA);
		}
		else if (P_Pin == BTN_SELECT_Pin){
			handler_btn_select();
		}
		else if (P_Pin == BTN_PLUS_Pin && state_config != 0){
			handler_btn_plus();
		}
		else if (P_Pin == BTN_MINUS_Pin && state_config != 0){
			handler_btn_minus();
		}
	}
}

int _write(int P_Flux, char* P_Message, int P_Taille) {
	HAL_StatusTypeDef Etat = HAL_UART_Transmit(&huart2, (uint8_t *) P_Message, P_Taille, HAL_MAX_DELAY);
	if (Etat == HAL_OK) return P_Taille;
	else return -1;
}
//---------------------------
int _read(int P_Flux, char* P_Message, int P_Taille) {
	HAL_StatusTypeDef Etat=HAL_UART_Receive(&huart2, (uint8_t *) P_Message, 1, HAL_MAX_DELAY);
	if (Etat == HAL_OK) return 1;
	else return -1;
}

void SystemClock_Config(void);
//--------------------
extern UART_HandleTypeDef huart2;

int main(){
	PERIOD[0] = &period_sensor_1;
	PERIOD[1] = &period_sensor_2;
	PERIOD[2] = &period_sensor_3;
	PERIOD[3] = &period_send_data;
	HAL_Init();
	SystemClock_Config();
	MX_USART2_UART_Init();
	MX_RNG_Init();
	MX_GPIO_Init();
	MX_RTC_Init();
	MX_ADC1_Init();
	MX_TIM1_Init();
	osKernelInitialize();
	HAL_UART_Receive_IT(&huart2, (void*)&receive_buffer, MESSAGE_RECEIVE_SIZE);
	Pipe_Reception_Analyse = osMessageQueueNew(TAILLE_PIPE_RECEPTION_ANALYSE, sizeof(T_DATA), NULL);
	osThreadNew(Fonction_Thread_Sensor_1, NULL, &Config_Thread_Sensor_1);
	osThreadNew(Fonction_Thread_Sensor_2, NULL, &Config_Thread_Sensor_2);
	osThreadNew(Fonction_Thread_Sensor_3, NULL, &Config_Thread_Sensor_3);
	Thread_Buzzer = osThreadNew(Fonction_Thread_Buzzer, NULL, &Config_Thread_Buzzer);
	Thread_Watch_Queue = osThreadNew(Fonction_Thread_Watch_Queue, NULL, &Config_Thread_Watch_Queue);
	Thread_Send_Data = osThreadNew(Fonction_Thread_Send, NULL, &Config_Thread_Send);
	osKernelStart();
	while(1);
}
