/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "fatfs.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "stm32f769i_discovery_lcd.h"
#include "stm32f769i_discovery_ts.h"
#include "stm32f769i_discovery.h"
#include <stdbool.h>
#include "imgBlack.h"
//#include "imgRed.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define TEMP_REFRESH_PERIOD   1000    /* Internal temperature refresh period */
#define MAX_CONVERTED_VALUE   4095    /* Max converted value */
#define AMBIENT_TEMP            25    /* Ambient Temperature */
#define VSENS_AT_AMBIENT_TEMP  760    /* VSENSE value (mv) at ambient temperature */
#define AVG_SLOPE               25    /* Avg_Solpe multiply by 10 */
#define VREF                   3300
#define TAMQUADRADO				56.3
#define X				TS_State.touchX[0]
#define Y				TS_State.touchY[0]
#define RAIO					22
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

DMA2D_HandleTypeDef hdma2d;

DSI_HandleTypeDef hdsi;

LTDC_HandleTypeDef hltdc;

SD_HandleTypeDef hsd2;

TIM_HandleTypeDef htim6;
TIM_HandleTypeDef htim7;

SDRAM_HandleTypeDef hsdram1;

/* USER CODE BEGIN PV */

TS_StateTypeDef TS_State;

volatile uint32_t ConvertedValue;
volatile uint8_t tempCnt = 0;		//contador para alterar a temperatura a cada 2 segundos
volatile int timeTotal=0;			//contador do tempo total de jogo
volatile uint8_t seconds = 0;		//contador para converter o tempo total em segundos
volatile uint8_t minutes=0;			//contador para os minutos do tempo total
volatile uint8_t ctdown=20;			//contador do tempo de jogada
volatile uint8_t touchCircle=0;		//contador de toques da peça branca
volatile uint8_t touchFill=0;		//contador de toques da peça preta
volatile uint8_t ctJogadas=0;		//contador para o numero de jogadas
volatile uint8_t ctTimeOut=3;		//contador para o numero de timeouts
volatile uint8_t ctReset=0;			//contador para o umero de resets

//goal 13
volatile uint8_t redPiece=0;
volatile uint8_t blackPiece=0;

long int JTemp;						//variavel para guardar a temperatura
char string[50];
int reset=0;
int passouTempo=0;


//FLAGS
volatile uint8_t DBFLAG=0; 				//flag para debouncing
bool TEMPFLAG=false;					//flag para entrar na temperatura
volatile uint8_t PLAYERFLAG=0;			//flag para trocar jogador
volatile uint8_t TIMEFLAG=0;			//flag para contador decrescente
volatile uint8_t MENUFLAG=0;			//flag para entrar no menu
volatile uint8_t UPDATE_DISPLAY = 0;	//flag para update display
volatile uint8_t LETSPLAY=0;			//flag para iniciar o jogo
bool VERMELHAJOGOU=false;				//flag para saber se branca jogou numa celula
bool PRETAJOGOU=false;					//flag para saber se preta jogou numa celula

//goal 10
volatile int RESETTIME=0;					//flag para saber se foi carregado o ecra para reset do timer

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_DMA2D_Init(void);
static void MX_DSIHOST_DSI_Init(void);
static void MX_FMC_Init(void);
static void MX_LTDC_Init(void);
static void MX_SDMMC2_SD_Init(void);
static void MX_TIM6_Init(void);
static void MX_TIM7_Init(void);
/* USER CODE BEGIN PFP */
static void LCD_Config();
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){

	if(GPIO_Pin==GPIO_PIN_0){
		reset=1;
	}

	if(GPIO_Pin==GPIO_PIN_13)
	{
		if(DBFLAG == 0)
		{
			DBFLAG=1;
			UPDATE_DISPLAY = 1;
			BSP_TS_GetState(&TS_State);
		}
	}
}

//GOAL 1 //temperatura
void temperatura() {
	uint32_t ConvertedValue;
	char desc[100];

	if(tempCnt >= 2)
	{
		tempCnt = 0;

		HAL_StatusTypeDef status = HAL_ADC_PollForConversion(&hadc1, TEMP_REFRESH_PERIOD);
		if (status == HAL_OK) {
			ConvertedValue = HAL_ADC_GetValue(&hadc1); //get value
			JTemp = ((((ConvertedValue * VREF) / MAX_CONVERTED_VALUE)- VSENS_AT_AMBIENT_TEMP) * 10 / AVG_SLOPE) + AMBIENT_TEMP;

			/* Display the Temperature Value on the LCD */
			sprintf(desc, "Temp: %ldC", JTemp);
			//BSP_LCD_FillRect(BSP_LCD_GetXSize()-160, 0, (uint8_t *) desc, LEFT_MODE);
			BSP_LCD_SetBackColor(LCD_COLOR_YELLOW);
			BSP_LCD_DisplayStringAt(BSP_LCD_GetXSize()-160, 0, (uint8_t *) desc, LEFT_MODE);
			BSP_LCD_ClearStringLine(BSP_LCD_GetXSize()-160);
		}
	}
}

//GOAL6 //saveSD
void saveSD(){

	char toques[500];
	uint nbytes;

	sprintf(toques,"Peça vermelha: %d toques Peça preta: %d toques / Tempo decorrido %02d:%02d / Temperatura: %ldC / Nº de resets: %d",touchCircle, touchFill, minutes, seconds, JTemp, ctReset);
	/*sprintf(toques,"Peca branca: %d toques / ",touchCircle);
	sprintf(toques,"Peca preta: %d toques / ",touchFill);
	sprintf(toques,"Tempo decorrido %02d:%02d",minutes,seconds);*/

	if(f_mount(&SDFatFS, SDPath, 0)!= FR_OK)
		Error_Handler();
	if(f_open(&SDFile, "toques.txt", FA_WRITE | FA_OPEN_ALWAYS) != FR_OK)
		Error_Handler();
	else
		BSP_LED_Toggle(LED_GREEN);
	if(f_write(&SDFile, toques, strlen(toques), &nbytes) != FR_OK)
		Error_Handler();
	else{
		BSP_LED_Toggle(LED_GREEN);
		HAL_Delay(250);
	}

	f_close(&SDFile);
}

/*void readSD(){
	if(f_mount(&SDFatFS, SDPath, 0)!= FR_OK)
		Error_Handler();
	if(f_open(&SDFile, "toques.txt", FA_WRITE | FA_OPEN_ALWAYS) != FR_OK){
		Error_Handler();
		BSP_LED_Toggle(LED_GREEN);
		HAL_Delay(250);
	}
	if(f_read(&SDFile, toques, strlen(toques), &nbytes) != FR_OK){
		Error_Handler();
		BSP_LED_Toggle(LED_GREEN);
		HAL_Delay(250);
	}
	BSP_LCD_DisplayStringAtLine(1, (uint8_t*)toques);
	f_close(&SDFile);
}*/

void drawPiece(int i, int j){

	if(DBFLAG == 1){
		HAL_Delay(100);
		DBFLAG=0;

		if(PLAYERFLAG==0){
			BSP_LCD_SetTextColor(LCD_COLOR_RED);
			BSP_LCD_FillCircle(i+(TAMQUADRADO/2), j+(TAMQUADRADO/2), RAIO);
			PLAYERFLAG=1;
			touchCircle++;
			ctJogadas++;
			VERMELHAJOGOU=true;
			BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
			blackPiece++;
		}
		else{
			BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
			BSP_LCD_FillCircle(i+(TAMQUADRADO/2), j+(TAMQUADRADO/2), RAIO);
			PLAYERFLAG=0;
			touchFill++;
			ctJogadas++;
			PRETAJOGOU=true;
			BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
			redPiece++;
		}
		if(ctJogadas==11){
			saveSD();
		}
	}
}

//GOAL 3 //limites
void limites()
{
	int i=0, j=0;

	/*memset(string, '\0', 50);
	sprintf(string, "X= %d", X);
	BSP_LCD_ClearStringLine(4);
	BSP_LCD_DisplayStringAtLine(4, (uint8_t *)string);

	memset(string, '\0', 50);
	sprintf(string, "Y= %d", Y);
	BSP_LCD_ClearStringLine(5);
	BSP_LCD_DisplayStringAtLine(5, (uint8_t *)string);*/

	for(i=0; i<BSP_LCD_GetXSize()-360; i+=TAMQUADRADO)
	{
		for(j=30;j<BSP_LCD_GetYSize()-30;j+=TAMQUADRADO)
		{
			if(X<BSP_LCD_GetXSize()-360  && X >= 0 && Y>30 && Y<BSP_LCD_GetYSize())
			{
				if(X>i && X<(i+TAMQUADRADO) && Y>j && Y<(j+TAMQUADRADO))
				{
					drawPiece(i,j);
					break;
				}
			}
			else
				DBFLAG=0;
		}
	}
}

//GOAL 2 //tab
void tab(){

	int i=0, j=30;
	//para o goal 13
	//char player[30];
	char red[30];
	char black[30];
	char winner[30];

	for(i=0; i<BSP_LCD_GetXSize()-360; i+=TAMQUADRADO)
	{
		/*BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
		BSP_LCD_FillRect(i, 30, TAMQUADRADO, TAMQUADRADO);
		BSP_LCD_SetTextColor(LCD_COLOR_BLACK);*/
		BSP_LCD_DrawRect(i, 30, TAMQUADRADO, TAMQUADRADO);


		for(j=30;j<BSP_LCD_GetYSize()-30;j+=TAMQUADRADO){
			/*BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
			BSP_LCD_FillRect(i, j, TAMQUADRADO, TAMQUADRADO);
			BSP_LCD_SetTextColor(LCD_COLOR_BLACK);*/
			BSP_LCD_DrawRect(i, j, TAMQUADRADO, TAMQUADRADO);
		}
		if(i>168.9 && i<225.2 && j>142.6 && j<198.9)
			BSP_LCD_DrawCircle(i+(TAMQUADRADO/2), j+(TAMQUADRADO/2), RAIO);
	}

	//goal 13
	sprintf(red,"Num pecas RED: %d",redPiece);
	BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
	BSP_LCD_DisplayStringAt(BSP_LCD_GetXSize()-310, 120, (uint8_t *)red, LEFT_MODE);
	sprintf(black,"Num pecas BLACK: %d",blackPiece);
	BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
	BSP_LCD_DisplayStringAt(BSP_LCD_GetXSize()-330, 150, (uint8_t *)black, LEFT_MODE);

	if(redPiece>blackPiece){
		sprintf(winner,"RED is winning");
		BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
		BSP_LCD_DisplayStringAt(BSP_LCD_GetXSize()-290, 170, (uint8_t *)winner, LEFT_MODE);
		BSP_LCD_ClearStringLine(BSP_LCD_GetXSize()-290);
	}
	else if(redPiece<blackPiece){
		sprintf(winner,"BLACK is winning");
		BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
		BSP_LCD_DisplayStringAt(BSP_LCD_GetXSize()-290, 170, (uint8_t *)winner, LEFT_MODE);
		BSP_LCD_ClearStringLine(BSP_LCD_GetXSize()-290);
	}
	else if(redPiece==blackPiece){
		sprintf(winner,"Game is tied");
		BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
		BSP_LCD_DisplayStringAt(BSP_LCD_GetXSize()-290, 170, (uint8_t *)winner, LEFT_MODE);
		BSP_LCD_ClearStringLine(BSP_LCD_GetXSize()-290);
	}
}

//GOAL 4

//GOAL 5 //menu
void menu(){


	char escolha[50];
	BSP_LCD_Clear(LCD_COLOR_WHITE);
	MENUFLAG=1;

	sprintf(escolha,"PLAYER vs PLAYER");
	BSP_LCD_DrawRect(BSP_LCD_GetXSize()-565, BSP_LCD_GetYSize()-400, 350, 60);
	BSP_LCD_DisplayStringAt(BSP_LCD_GetXSize()-525, BSP_LCD_GetYSize()-375, (uint8_t *) escolha, LEFT_MODE);
	BSP_LCD_ClearStringLine(BSP_LCD_GetXSize()-525);

	sprintf(escolha, "PLAYER vs CPU");
	BSP_LCD_DrawRect(BSP_LCD_GetXSize()-540, BSP_LCD_GetYSize()-265, 300, 60);
	BSP_LCD_DisplayStringAt(BSP_LCD_GetXSize()-500, BSP_LCD_GetYSize()-240, (uint8_t *) escolha, LEFT_MODE);
	BSP_LCD_ClearStringLine(BSP_LCD_GetXSize()-500);


	while(DBFLAG != 1)	//Enquanto n tocar no lcd
	{
		HAL_Delay(100);
	}

	if(X>=BSP_LCD_GetXSize()-565 && X<=BSP_LCD_GetXSize()-25 && Y>=BSP_LCD_GetYSize()-400 && Y<=BSP_LCD_GetYSize()-340)
	{
		MENUFLAG=0;
		UPDATE_DISPLAY=1;
		//LCD_Config();
		//LETSPLAY=1;
		BSP_LCD_Clear(LCD_COLOR_WHITE);
		//joga();
	}
	/*if(X==BSP_LCD_GetXSize()-500 && Y==BSP_LCD_GetYSize()-240){
	 	MENUFLAG=1;
	 	UPDATE_DISPLAY=1;
		jogaAI();
	}*/

}


//GOAL 10 /////////////é preciso dar mais uns toques para voltar a mostrar o reset
void touchTime(){
	if(X>=BSP_LCD_GetXSize()-100 && X<=BSP_LCD_GetXSize() && Y>=31 && Y<=61){
		minutes=0;
		seconds=0;
		ctdown=0;
		ctTimeOut=3;
		RESETTIME=1;
	}
}

//GOAL7	//countdown
void countDown()
{
	char t[50];
	char to[50];
	char win[50];

	//touchTime();

	if(PLAYERFLAG==0){
		sprintf(t, "Countdown: %02ds", ctdown);
		sprintf(to,"Timeouts %d", ctTimeOut);
		if(ctdown>10){
			BSP_LCD_SetBackColor(LCD_COLOR_GREEN);
			BSP_LCD_DisplayStringAt(BSP_LCD_GetXSize()-290, 55, (uint8_t *)t, LEFT_MODE);
			BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
			BSP_LCD_DisplayStringAt(BSP_LCD_GetXSize()-290, 85, (uint8_t *)to, LEFT_MODE);
		}
		if(ctdown<=10 && ctdown>5){
			BSP_LCD_SetBackColor(LCD_COLOR_YELLOW);
			BSP_LCD_DisplayStringAt(BSP_LCD_GetXSize()-290, 55, (uint8_t *)t, LEFT_MODE);
			BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
			BSP_LCD_DisplayStringAt(BSP_LCD_GetXSize()-290, 85, (uint8_t *)to, LEFT_MODE);
		}
		if(ctdown<=5){
			BSP_LCD_SetBackColor(LCD_COLOR_RED);
			BSP_LCD_DisplayStringAt(BSP_LCD_GetXSize()-290, 55, (uint8_t *)t, LEFT_MODE);
			BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
			BSP_LCD_DisplayStringAt(BSP_LCD_GetXSize()-290, 85, (uint8_t *)to, LEFT_MODE);
		}
		if(ctdown==0){
			passouTempo++;
			ctdown=20;
			ctTimeOut--;
			PLAYERFLAG=1;
		}
		if(ctTimeOut==0){
			BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
			sprintf(win,"BLACK WINS");
			BSP_LCD_DisplayStringAt(BSP_LCD_GetXSize()-290, BSP_LCD_GetYSize()/2, (uint8_t *)win, LEFT_MODE);
			//BSP_LCD_Clear(LCD_COLOR_WHITE);
			BSP_LCD_DrawBitmap(BSP_LCD_GetXSize()-290,BSP_LCD_GetYSize()/2,(uint8_t*) stlogo);
			LETSPLAY=0;
			saveSD();
		}

	}

	if(PLAYERFLAG==1){
		sprintf(t, "Countdown: %02ds", ctdown);
		sprintf(to,"Timeouts %d", ctTimeOut);
		if(ctdown>10){
			BSP_LCD_SetBackColor(LCD_COLOR_GREEN);
			BSP_LCD_DisplayStringAt(BSP_LCD_GetXSize()-290, 55, (uint8_t *)t, LEFT_MODE);
			BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
			BSP_LCD_DisplayStringAt(BSP_LCD_GetXSize()-290, 85, (uint8_t *)to, LEFT_MODE);
		}
		if(ctdown<=10 && ctdown>5){
			BSP_LCD_SetBackColor(LCD_COLOR_YELLOW);
			BSP_LCD_DisplayStringAt(BSP_LCD_GetXSize()-290, 55, (uint8_t *)t, LEFT_MODE);
			BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
			BSP_LCD_DisplayStringAt(BSP_LCD_GetXSize()-290, 85, (uint8_t *)to, LEFT_MODE);
		}
		if(ctdown<=5){
			BSP_LCD_SetBackColor(LCD_COLOR_RED);
			BSP_LCD_DisplayStringAt(BSP_LCD_GetXSize()-290, 55, (uint8_t *)t, LEFT_MODE);
			BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
			BSP_LCD_DisplayStringAt(BSP_LCD_GetXSize()-290, 85, (uint8_t *)to, LEFT_MODE);
		}
		if(ctdown==0){
			ctdown=20;
			ctTimeOut--;
			PLAYERFLAG=0;
		}
		if(ctTimeOut==0){
			BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
			sprintf(win,"RED WINS");
			BSP_LCD_DisplayStringAt(BSP_LCD_GetXSize()-290, BSP_LCD_GetYSize()/2, (uint8_t *)win, LEFT_MODE);
			//BSP_LCD_Clear(LCD_COLOR_WHITE);
			//BSP_LCD_DrawBitmap(BSP_LCD_GetXSize()-290,BSP_LCD_GetYSize()/2,(uint8_t*) stlogo);
			LETSPLAY=0;
			saveSD();
		}

	}
	BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
}

//GOAL8	//tempoJogo
void tempoJogo(){

	char t[50];

	if(TIMEFLAG==0)
	{
		sprintf(t, "Total Time: %02d:%02d", minutes, seconds);
		BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
		BSP_LCD_DisplayStringAt(BSP_LCD_GetXSize()-290, 31, (uint8_t *)t, LEFT_MODE);

		if(seconds>59)
		{
			minutes++;
			seconds=0;
		}
		touchTime();
	}
}

//GOAL 9	//restart
void restart(){
	BSP_LCD_Clear(LCD_COLOR_WHITE);
	menu();

	ConvertedValue;
	tempCnt = 0;
	timeTotal=0;
	seconds = 0;
	minutes=0;
	ctdown=20;
	touchCircle=0;
	touchFill=0;
	ctJogadas=0;
	ctTimeOut=3;
	ctReset=0;

	reset=0;
	passouTempo=0;

	//FLAGS
	DBFLAG=0;
	TEMPFLAG=false;
	PLAYERFLAG=0;
	TIMEFLAG=0;
	MENUFLAG=0;
	UPDATE_DISPLAY = 0;
	//LETSPLAY=0;
	VERMELHAJOGOU=false;
	PRETAJOGOU=false;
}

//funcoes de apoio
void joga(){
	//BSP_LCD_Clear(LCD_COLOR_WHITE);
	temperatura();
	tab();
	tempoJogo();
	if(ctJogadas==11){
		saveSD();
		//restart();
		menu();
	}
	if(reset==1)
	  restart();
}

void jogaAI(){

}

void getTouch(){
	if(DBFLAG == 1)
		  {
			  DBFLAG = 0;
			  memset(string, '\0', 50);
			  sprintf(string, "X= %d", TS_State.touchX[0]);
			  BSP_LCD_ClearStringLine(4);
			  BSP_LCD_DisplayStringAtLine(4, (uint8_t *)string);

			  memset(string, '\0', 50);
			  sprintf(string, "Y= %d", TS_State.touchY[0]);
			  BSP_LCD_ClearStringLine(5);
			  BSP_LCD_DisplayStringAtLine(5, (uint8_t *)string);

		  }
}
/*void header(){
	BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
	BSP_LCD_SetBackColor(LCD_COLOR_BLUE);
	BSP_LCD_SetFont(&Font24);
	BSP_LCD_DisplayStringAt(0, 5, (uint8_t *)"REVERSI PROJECT", CENTER_MODE);
}*/

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */
  

  /* Enable I-Cache---------------------------------------------------------*/
  SCB_EnableICache();

  /* Enable D-Cache---------------------------------------------------------*/
  SCB_EnableDCache();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_DMA2D_Init();
  MX_DSIHOST_DSI_Init();
  MX_FMC_Init();
  MX_LTDC_Init();
  MX_SDMMC2_SD_Init();
  MX_TIM6_Init();
  MX_TIM7_Init();
  MX_FATFS_Init();
  /* USER CODE BEGIN 2 */
  LCD_Config();
  	//BSP_LED_Init(LED1);

  BSP_TS_Init(BSP_LCD_GetXSize(), BSP_LCD_GetYSize());
  BSP_TS_ITConfig();

  	//start do adc
  	HAL_ADC_Start(&hadc1);
  	//HAL_ADC_Start_IT(&hadc3);

  	HAL_TIM_Base_Start_IT(&htim6);
  	HAL_TIM_Base_Start_IT(&htim7);

  	//tab();	//TODO: trocar por menu
  	menu();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  /*if(MENUFLAG==0)
		  menu();*/
	  //joga();
	  temperatura();	//Flags internas: 2s update
	  //if(LETSPLAY==1){
	  tempoJogo();		//Counter 1 sec update
	  countDown();		//Counter 1 sec update
	  //}

	  if(UPDATE_DISPLAY == 1)
	  {
		  UPDATE_DISPLAY = 0;
		  tab();
		  limites();
	  }


	  if(reset==1){
	  	  restart();
	  	  ctReset++;
	  }


    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  //LETSPLAY=0;
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Configure the main internal regulator output voltage 
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 400;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 8;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Activate the Over-Drive mode 
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_6) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC|RCC_PERIPHCLK_SDMMC2
                              |RCC_PERIPHCLK_CLK48;
  PeriphClkInitStruct.PLLSAI.PLLSAIN = 192;
  PeriphClkInitStruct.PLLSAI.PLLSAIR = 2;
  PeriphClkInitStruct.PLLSAI.PLLSAIQ = 2;
  PeriphClkInitStruct.PLLSAI.PLLSAIP = RCC_PLLSAIP_DIV2;
  PeriphClkInitStruct.PLLSAIDivQ = 1;
  PeriphClkInitStruct.PLLSAIDivR = RCC_PLLSAIDIVR_2;
  PeriphClkInitStruct.Clk48ClockSelection = RCC_CLK48SOURCE_PLL;
  PeriphClkInitStruct.Sdmmc2ClockSelection = RCC_SDMMC2CLKSOURCE_CLK48;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */
  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion) 
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SEQ_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time. 
  */
  sConfig.Channel = ADC_CHANNEL_TEMPSENSOR;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_56CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief DMA2D Initialization Function
  * @param None
  * @retval None
  */
static void MX_DMA2D_Init(void)
{

  /* USER CODE BEGIN DMA2D_Init 0 */

  /* USER CODE END DMA2D_Init 0 */

  /* USER CODE BEGIN DMA2D_Init 1 */

  /* USER CODE END DMA2D_Init 1 */
  hdma2d.Instance = DMA2D;
  hdma2d.Init.Mode = DMA2D_M2M;
  hdma2d.Init.ColorMode = DMA2D_OUTPUT_ARGB8888;
  hdma2d.Init.OutputOffset = 0;
  hdma2d.LayerCfg[1].InputOffset = 0;
  hdma2d.LayerCfg[1].InputColorMode = DMA2D_INPUT_ARGB8888;
  hdma2d.LayerCfg[1].AlphaMode = DMA2D_NO_MODIF_ALPHA;
  hdma2d.LayerCfg[1].InputAlpha = 0;
  hdma2d.LayerCfg[1].AlphaInverted = DMA2D_REGULAR_ALPHA;
  hdma2d.LayerCfg[1].RedBlueSwap = DMA2D_RB_REGULAR;
  if (HAL_DMA2D_Init(&hdma2d) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_DMA2D_ConfigLayer(&hdma2d, 1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN DMA2D_Init 2 */

  /* USER CODE END DMA2D_Init 2 */

}

/**
  * @brief DSIHOST Initialization Function
  * @param None
  * @retval None
  */
static void MX_DSIHOST_DSI_Init(void)
{

  /* USER CODE BEGIN DSIHOST_Init 0 */

  /* USER CODE END DSIHOST_Init 0 */

  DSI_PLLInitTypeDef PLLInit = {0};
  DSI_HOST_TimeoutTypeDef HostTimeouts = {0};
  DSI_PHY_TimerTypeDef PhyTimings = {0};
  DSI_LPCmdTypeDef LPCmd = {0};
  DSI_CmdCfgTypeDef CmdCfg = {0};

  /* USER CODE BEGIN DSIHOST_Init 1 */

  /* USER CODE END DSIHOST_Init 1 */
  hdsi.Instance = DSI;
  hdsi.Init.AutomaticClockLaneControl = DSI_AUTO_CLK_LANE_CTRL_DISABLE;
  hdsi.Init.TXEscapeCkdiv = 4;
  hdsi.Init.NumberOfLanes = DSI_ONE_DATA_LANE;
  PLLInit.PLLNDIV = 20;
  PLLInit.PLLIDF = DSI_PLL_IN_DIV1;
  PLLInit.PLLODF = DSI_PLL_OUT_DIV1;
  if (HAL_DSI_Init(&hdsi, &PLLInit) != HAL_OK)
  {
    Error_Handler();
  }
  HostTimeouts.TimeoutCkdiv = 1;
  HostTimeouts.HighSpeedTransmissionTimeout = 0;
  HostTimeouts.LowPowerReceptionTimeout = 0;
  HostTimeouts.HighSpeedReadTimeout = 0;
  HostTimeouts.LowPowerReadTimeout = 0;
  HostTimeouts.HighSpeedWriteTimeout = 0;
  HostTimeouts.HighSpeedWritePrespMode = DSI_HS_PM_DISABLE;
  HostTimeouts.LowPowerWriteTimeout = 0;
  HostTimeouts.BTATimeout = 0;
  if (HAL_DSI_ConfigHostTimeouts(&hdsi, &HostTimeouts) != HAL_OK)
  {
    Error_Handler();
  }
  PhyTimings.ClockLaneHS2LPTime = 28;
  PhyTimings.ClockLaneLP2HSTime = 33;
  PhyTimings.DataLaneHS2LPTime = 15;
  PhyTimings.DataLaneLP2HSTime = 25;
  PhyTimings.DataLaneMaxReadTime = 0;
  PhyTimings.StopWaitTime = 0;
  if (HAL_DSI_ConfigPhyTimer(&hdsi, &PhyTimings) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_DSI_ConfigFlowControl(&hdsi, DSI_FLOW_CONTROL_BTA) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_DSI_SetLowPowerRXFilter(&hdsi, 10000) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_DSI_ConfigErrorMonitor(&hdsi, HAL_DSI_ERROR_NONE) != HAL_OK)
  {
    Error_Handler();
  }
  LPCmd.LPGenShortWriteNoP = DSI_LP_GSW0P_DISABLE;
  LPCmd.LPGenShortWriteOneP = DSI_LP_GSW1P_DISABLE;
  LPCmd.LPGenShortWriteTwoP = DSI_LP_GSW2P_DISABLE;
  LPCmd.LPGenShortReadNoP = DSI_LP_GSR0P_DISABLE;
  LPCmd.LPGenShortReadOneP = DSI_LP_GSR1P_DISABLE;
  LPCmd.LPGenShortReadTwoP = DSI_LP_GSR2P_DISABLE;
  LPCmd.LPGenLongWrite = DSI_LP_GLW_DISABLE;
  LPCmd.LPDcsShortWriteNoP = DSI_LP_DSW0P_DISABLE;
  LPCmd.LPDcsShortWriteOneP = DSI_LP_DSW1P_DISABLE;
  LPCmd.LPDcsShortReadNoP = DSI_LP_DSR0P_DISABLE;
  LPCmd.LPDcsLongWrite = DSI_LP_DLW_DISABLE;
  LPCmd.LPMaxReadPacket = DSI_LP_MRDP_DISABLE;
  LPCmd.AcknowledgeRequest = DSI_ACKNOWLEDGE_DISABLE;
  if (HAL_DSI_ConfigCommand(&hdsi, &LPCmd) != HAL_OK)
  {
    Error_Handler();
  }
  CmdCfg.VirtualChannelID = 0;
  CmdCfg.ColorCoding = DSI_RGB888;
  CmdCfg.CommandSize = 640;
  CmdCfg.TearingEffectSource = DSI_TE_EXTERNAL;
  CmdCfg.TearingEffectPolarity = DSI_TE_RISING_EDGE;
  CmdCfg.HSPolarity = DSI_HSYNC_ACTIVE_LOW;
  CmdCfg.VSPolarity = DSI_VSYNC_ACTIVE_LOW;
  CmdCfg.DEPolarity = DSI_DATA_ENABLE_ACTIVE_HIGH;
  CmdCfg.VSyncPol = DSI_VSYNC_FALLING;
  CmdCfg.AutomaticRefresh = DSI_AR_ENABLE;
  CmdCfg.TEAcknowledgeRequest = DSI_TE_ACKNOWLEDGE_DISABLE;
  if (HAL_DSI_ConfigAdaptedCommandMode(&hdsi, &CmdCfg) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_DSI_SetGenericVCID(&hdsi, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN DSIHOST_Init 2 */

  /* USER CODE END DSIHOST_Init 2 */

}

/**
  * @brief LTDC Initialization Function
  * @param None
  * @retval None
  */
static void MX_LTDC_Init(void)
{

  /* USER CODE BEGIN LTDC_Init 0 */

  /* USER CODE END LTDC_Init 0 */

  LTDC_LayerCfgTypeDef pLayerCfg = {0};
  LTDC_LayerCfgTypeDef pLayerCfg1 = {0};

  /* USER CODE BEGIN LTDC_Init 1 */

  /* USER CODE END LTDC_Init 1 */
  hltdc.Instance = LTDC;
  hltdc.Init.HSPolarity = LTDC_HSPOLARITY_AL;
  hltdc.Init.VSPolarity = LTDC_VSPOLARITY_AL;
  hltdc.Init.DEPolarity = LTDC_DEPOLARITY_AL;
  hltdc.Init.PCPolarity = LTDC_PCPOLARITY_IPC;
  hltdc.Init.HorizontalSync = 7;
  hltdc.Init.VerticalSync = 3;
  hltdc.Init.AccumulatedHBP = 14;
  hltdc.Init.AccumulatedVBP = 5;
  hltdc.Init.AccumulatedActiveW = 654;
  hltdc.Init.AccumulatedActiveH = 485;
  hltdc.Init.TotalWidth = 660;
  hltdc.Init.TotalHeigh = 487;
  hltdc.Init.Backcolor.Blue = 0;
  hltdc.Init.Backcolor.Green = 0;
  hltdc.Init.Backcolor.Red = 0;
  if (HAL_LTDC_Init(&hltdc) != HAL_OK)
  {
    Error_Handler();
  }
  pLayerCfg.WindowX0 = 0;
  pLayerCfg.WindowX1 = 0;
  pLayerCfg.WindowY0 = 0;
  pLayerCfg.WindowY1 = 0;
  pLayerCfg.PixelFormat = LTDC_PIXEL_FORMAT_ARGB8888;
  pLayerCfg.Alpha = 0;
  pLayerCfg.Alpha0 = 0;
  pLayerCfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_CA;
  pLayerCfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_CA;
  pLayerCfg.FBStartAdress = 0;
  pLayerCfg.ImageWidth = 0;
  pLayerCfg.ImageHeight = 0;
  pLayerCfg.Backcolor.Blue = 0;
  pLayerCfg.Backcolor.Green = 0;
  pLayerCfg.Backcolor.Red = 0;
  if (HAL_LTDC_ConfigLayer(&hltdc, &pLayerCfg, 0) != HAL_OK)
  {
    Error_Handler();
  }
  pLayerCfg1.WindowX0 = 0;
  pLayerCfg1.WindowX1 = 0;
  pLayerCfg1.WindowY0 = 0;
  pLayerCfg1.WindowY1 = 0;
  pLayerCfg1.PixelFormat = LTDC_PIXEL_FORMAT_ARGB8888;
  pLayerCfg1.Alpha = 0;
  pLayerCfg1.Alpha0 = 0;
  pLayerCfg1.BlendingFactor1 = LTDC_BLENDING_FACTOR1_CA;
  pLayerCfg1.BlendingFactor2 = LTDC_BLENDING_FACTOR2_CA;
  pLayerCfg1.FBStartAdress = 0;
  pLayerCfg1.ImageWidth = 0;
  pLayerCfg1.ImageHeight = 0;
  pLayerCfg1.Backcolor.Blue = 0;
  pLayerCfg1.Backcolor.Green = 0;
  pLayerCfg1.Backcolor.Red = 0;
  if (HAL_LTDC_ConfigLayer(&hltdc, &pLayerCfg1, 1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN LTDC_Init 2 */

  /* USER CODE END LTDC_Init 2 */

}

/**
  * @brief SDMMC2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SDMMC2_SD_Init(void)
{

  /* USER CODE BEGIN SDMMC2_Init 0 */

  /* USER CODE END SDMMC2_Init 0 */

  /* USER CODE BEGIN SDMMC2_Init 1 */

  /* USER CODE END SDMMC2_Init 1 */
  hsd2.Instance = SDMMC2;
  hsd2.Init.ClockEdge = SDMMC_CLOCK_EDGE_RISING;
  hsd2.Init.ClockBypass = SDMMC_CLOCK_BYPASS_DISABLE;
  hsd2.Init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_DISABLE;
  hsd2.Init.BusWide = SDMMC_BUS_WIDE_1B;
  hsd2.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE;
  hsd2.Init.ClockDiv = 0;
  /* USER CODE BEGIN SDMMC2_Init 2 */

  /* USER CODE END SDMMC2_Init 2 */

}

/**
  * @brief TIM6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM6_Init(void)
{

  /* USER CODE BEGIN TIM6_Init 0 */

  /* USER CODE END TIM6_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM6_Init 1 */

  /* USER CODE END TIM6_Init 1 */
  htim6.Instance = TIM6;
  htim6.Init.Prescaler = 9999;
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = 9999;
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM6_Init 2 */

  /* USER CODE END TIM6_Init 2 */

}

/**
  * @brief TIM7 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM7_Init(void)
{

  /* USER CODE BEGIN TIM7_Init 0 */

  /* USER CODE END TIM7_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM7_Init 1 */

  /* USER CODE END TIM7_Init 1 */
  htim7.Instance = TIM7;
  htim7.Init.Prescaler = 9999;
  htim7.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim7.Init.Period = 9999;
  htim7.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim7) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim7, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM7_Init 2 */

  /* USER CODE END TIM7_Init 2 */

}

/* FMC initialization function */
static void MX_FMC_Init(void)
{

  /* USER CODE BEGIN FMC_Init 0 */

  /* USER CODE END FMC_Init 0 */

  FMC_SDRAM_TimingTypeDef SdramTiming = {0};

  /* USER CODE BEGIN FMC_Init 1 */

  /* USER CODE END FMC_Init 1 */

  /** Perform the SDRAM1 memory initialization sequence
  */
  hsdram1.Instance = FMC_SDRAM_DEVICE;
  /* hsdram1.Init */
  hsdram1.Init.SDBank = FMC_SDRAM_BANK2;
  hsdram1.Init.ColumnBitsNumber = FMC_SDRAM_COLUMN_BITS_NUM_8;
  hsdram1.Init.RowBitsNumber = FMC_SDRAM_ROW_BITS_NUM_13;
  hsdram1.Init.MemoryDataWidth = FMC_SDRAM_MEM_BUS_WIDTH_32;
  hsdram1.Init.InternalBankNumber = FMC_SDRAM_INTERN_BANKS_NUM_4;
  hsdram1.Init.CASLatency = FMC_SDRAM_CAS_LATENCY_1;
  hsdram1.Init.WriteProtection = FMC_SDRAM_WRITE_PROTECTION_DISABLE;
  hsdram1.Init.SDClockPeriod = FMC_SDRAM_CLOCK_DISABLE;
  hsdram1.Init.ReadBurst = FMC_SDRAM_RBURST_DISABLE;
  hsdram1.Init.ReadPipeDelay = FMC_SDRAM_RPIPE_DELAY_0;
  /* SdramTiming */
  SdramTiming.LoadToActiveDelay = 16;
  SdramTiming.ExitSelfRefreshDelay = 16;
  SdramTiming.SelfRefreshTime = 16;
  SdramTiming.RowCycleDelay = 16;
  SdramTiming.WriteRecoveryTime = 16;
  SdramTiming.RPDelay = 16;
  SdramTiming.RCDDelay = 16;

  if (HAL_SDRAM_Init(&hsdram1, &SdramTiming) != HAL_OK)
  {
    Error_Handler( );
  }

  /* USER CODE BEGIN FMC_Init 2 */

  /* USER CODE END FMC_Init 2 */
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOI_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOJ_CLK_ENABLE();

  /*Configure GPIO pin : PI13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);

  /*Configure GPIO pin : PI15 */
  GPIO_InitStruct.Pin = GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);

  /*Configure GPIO pin : PA0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

}

/* USER CODE BEGIN 4 */
static void LCD_Config(void)
{
  uint32_t  lcd_status = LCD_OK;

  /* Initialize the LCD */
  lcd_status = BSP_LCD_Init();
  while(lcd_status != LCD_OK);

  BSP_LCD_LayerDefaultInit(0, LCD_FB_START_ADDRESS);

  /* Clear the LCD */
  BSP_LCD_Clear(LCD_COLOR_WHITE);

  /* Set LCD Example description */
  BSP_LCD_SetTextColor(LCD_COLOR_DARKBLUE);
  BSP_LCD_SetFont(&Font12);
  BSP_LCD_DisplayStringAt(50, BSP_LCD_GetYSize()- 15, (uint8_t *)"Copyright (c) Vitor Bernardino 2019", RIGHT_MODE);
  BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
  BSP_LCD_FillRect(0, 0, BSP_LCD_GetXSize(), 30);
  BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
  BSP_LCD_SetBackColor(LCD_COLOR_BLUE);
  BSP_LCD_SetFont(&Font24);
  BSP_LCD_DisplayStringAt(0, 5, (uint8_t *)"REVERSI PROJECT", CENTER_MODE);
  BSP_LCD_SetFont(&Font16);
  /*BSP_LCD_DisplayStringAt(0, 60, (uint8_t *)"This example shows how to measure the Junction", CENTER_MODE);
  BSP_LCD_DisplayStringAt(0, 75, (uint8_t *)"Temperature of the device via an Internal", CENTER_MODE);
  BSP_LCD_DisplayStringAt(0, 90, (uint8_t *)"Sensor and display the Value on the LCD", CENTER_MODE);*/

  BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
  BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
  BSP_LCD_SetFont(&Font24);
}


void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) { //comum para todos os timers

	if (htim->Instance == TIM6){
		tempCnt ++;
		timeTotal++;
		seconds++;
	}
	if (htim->Instance == TIM7){
		TIMEFLAG=0;
		ctdown--;
	}
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
	while(1){
		BSP_LED_Toggle(LED_RED);
		HAL_Delay(250);
	}
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
