/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "rtc.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#include "OLED.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
RTC_TimeTypeDef Get_time;
RTC_DateTypeDef Get_date;
RTC_AlarmTypeDef Get_alarm;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define DHT11_HIGH     HAL_GPIO_WritePin(GPIOE, DHT11_Pin,	GPIO_PIN_SET) //输出高电平
#define DHT11_LOW      HAL_GPIO_WritePin(GPIOE, DHT11_Pin, GPIO_PIN_RESET)//输出低电平
#define DHT11_IO_IN      HAL_GPIO_ReadPin(GPIOE, DHT11_Pin)//读取IO口电平
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint8_t temp=0;
uint8_t humi=0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void beep_start()
{
	HAL_GPIO_WritePin(GPIOG,GPIO_PIN_7,GPIO_PIN_SET);
}
void beep_stop()
{
	HAL_GPIO_WritePin(GPIOG,GPIO_PIN_7,GPIO_PIN_RESET);
}

void Calculate_week(int y,int m,int d)
{
	if(m==1||m==2)
	{
		m+=12;
		y--;
	}
	int week=(d+2*m+3*(m+1)/5+y+y/4-y/100+y/400)%7;
	switch(week)
	{
		case 0:
		{
			OLED_ShowString(0,4,(uint8_t*)"Monday   ",16);
			break;
		}
		case 1:
		{
			OLED_ShowString(0,4,(uint8_t*)"Tuesday  ",16);
			break;
		}
		case 2:
		{
			OLED_ShowString(0,4,(uint8_t*)"Wednesday",16);
			break;
		}
		case 3:
		{
			OLED_ShowString(0,4,(uint8_t*)"Thursday ",16);
			break;
		}
		case 4:
		{
			OLED_ShowString(0,4,(uint8_t*)"Friday   ",16);
			break;
		}
		case 5:
		{
			OLED_ShowString(0,4,(uint8_t*)"Saturday  ",16);
			break;
		}
		case 6:
		{
			OLED_ShowString(0,4,(uint8_t*)"Sunday   ",16);
			break;
		}
	}
}
void def_config()
{
	Get_alarm.AlarmTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
	Get_alarm.AlarmTime.StoreOperation = RTC_STOREOPERATION_RESET;
	Get_alarm.AlarmMask=RTC_ALARMMASK_DATEWEEKDAY;
	Get_alarm.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL;
	Get_alarm.Alarm = RTC_ALARM_A;
}
void Delay_us(int us)
{
	__HAL_TIM_SetCounter(&htim1,0);
	__HAL_TIM_ENABLE(&htim1);
	while(__HAL_TIM_GET_COUNTER(&htim1)<us);
	__HAL_TIM_DISABLE(&htim1);
}

void DHT11_OUT(void)
{
	GPIO_InitTypeDef  GPIO_InitStruct = {0};
 
	GPIO_InitStruct.Pin = GPIO_PIN_5;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
}

void DHT11_IN(void)
{
	GPIO_InitTypeDef  GPIO_InitStruct = {0};
 
	GPIO_InitStruct.Pin  = GPIO_PIN_5;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
}
void DHT11_Strat(void)
{
	DHT11_OUT();   //设置为输出模式
	DHT11_LOW;     //主机拉低总线
	HAL_Delay(20); //延迟必须大于18ms ； 
	DHT11_HIGH;    //主机拉高总线等待DHT11响应
	Delay_us(30);   
}
uint8_t DHT11_Check(void)
{
	uint8_t retry = 0 ;
	DHT11_IN();
	//采用while循环的方式检测响应信号
	while(DHT11_IO_IN && retry <100) // DHT11会拉低 40us ~80us
	{
		retry++;
		Delay_us(1);//1us
	}
	if(retry>=100) //判断当DHT11延迟超过80us时return 1 ， 说明响应失败
	{return  1;}
	else retry =  0 ;
		
	while(!DHT11_IO_IN && retry<100)// // DHT11拉低之后会拉高 40us ~80us
	{
		retry++;
		Delay_us(1);//1us
	}
		
	if(retry>=100)
	{return 1;}
	return 0 ;
}
uint8_t DHT11_Read_Bit(void)
{
	uint8_t retry = 0 ;
	while(DHT11_IO_IN && retry <100)//同上采用while循环的方式去采集数据
	{
		retry++;
		Delay_us(1);
	}
	retry = 0 ;
	while(!DHT11_IO_IN && retry<100)
	{
		retry++;
		Delay_us(1);
	}
 
	Delay_us(40);              //结束信号，延时40us 
	if(DHT11_IO_IN) return 1;  //结束信号后，总线会被拉高 则返回1表示读取成功
	else 
	return 0 ;
}
uint8_t DHT11_Read_Byte(void)
{
	uint8_t i , dat ;
	dat = 0 ;
	for(i=0; i<8; i++)
	{
		dat <<= 1; //通过左移存储数据
		dat |= DHT11_Read_Bit();
	}
	return dat ; 
}
uint8_t DHT11_Read_Data(uint8_t* temp , uint8_t* humi)
{
	uint8_t buf[5];        //储存五位数据
    uint8_t i;    
	DHT11_Strat();         //起始信号
	if(DHT11_Check() == 0) //响应信号
    {
		for(i=0; i<5; i++)
		{
			buf[i] = DHT11_Read_Byte();
		}
		if(buf[0]+buf[1]+buf[2]+buf[3] == buf[4]) //校验数据
		{
		  *humi = buf[0]; // 湿度
			*temp = buf[2]; // 温度
		}
	}else return 1;
	
   return 0 ;
}
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint8_t year_judge(int year)
{
	if(year%100!=0)
	{
		if(year%4==0)
			return 1;
		else
			return 0;
	}
	else
	{
		if(year%400==0)
			return 1;
		else
			return 0;
	}
}
uint8_t set=0;//0:时钟界面 1：时钟设置界面 2：闹钟设置界面 3：日期设置界面
uint8_t mode=0;//0:设置小时（年） 1：设置分钟（月） 2：设置秒（日）
uint8_t tim_up=0;
int year;
int month;
int date;
int Hours=0;
int Minutes=0;
int Seconds=0;
int alarm_h;
int alarm_m;
int alarm_s;
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if(GPIO_Pin==GPIO_PIN_0)
	{
		if(HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_0)==GPIO_PIN_SET)
		{
			HAL_NVIC_SetPriority(SysTick_IRQn,0,0);
			HAL_Delay(20);
			HAL_NVIC_SetPriority(SysTick_IRQn,15,0);
			if(HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_0)==GPIO_PIN_SET)
			{
				if(tim_up==0)
				{
					set+=1;
					if(set>3)
						set=0;
				}
				if(tim_up==1)
				{
					tim_up=0;
					beep_stop();
				}
				while(HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_0)==GPIO_PIN_SET);
			}
		}
	}
	
	if(GPIO_Pin==GPIO_PIN_2)
	{
		if(HAL_GPIO_ReadPin(GPIOG,GPIO_PIN_2)==GPIO_PIN_SET)
		{
			if(set!=0)
			{
				mode+=1;
				if(mode>2)
					mode=0;
				year=Get_date.Year;
				month=Get_date.Month;
				date=Get_date.Date;
				Hours=Get_time.Hours;
				Minutes=Get_time.Minutes;
				Seconds=Get_time.Seconds;
				alarm_h=Get_alarm.AlarmTime.Hours;
				alarm_m=Get_alarm.AlarmTime.Minutes;
				alarm_s=Get_alarm.AlarmTime.Seconds;
				while(HAL_GPIO_ReadPin(GPIOG,GPIO_PIN_2)==GPIO_PIN_SET);
			}
		}
	}
	if(GPIO_Pin==GPIO_PIN_3)
	{
		if(HAL_GPIO_ReadPin(GPIOG,GPIO_PIN_3)==GPIO_PIN_SET)
		{
			if(set==1)
			{
				if(mode==0)
				{
					Hours+=1;
					if(Hours>23)
						Hours=0;
				}
				if(mode==1)
				{
					Minutes+=1;
					if(Minutes>59)
						Minutes=0;
				}
				if(mode==2)
				{
					Seconds+=1;
					if(Seconds>59)
						Seconds=0;
				}
				Get_time.Hours=Hours;
				Get_time.Minutes=Minutes;
				Get_time.Seconds=Seconds;
				HAL_RTC_SetTime(&hrtc,&Get_time,FORMAT_BIN);
			}
			if(set==2)
			{
				if(mode==0)
				{
					alarm_h+=1;
					if(alarm_h>23)
						alarm_h=0;
				}
				if(mode==1)
				{
					alarm_m+=1;
					if(alarm_m>59)
						alarm_m=0;
				}
				if(mode==2)
				{
					alarm_s+=1;
					if(alarm_s>59)
						alarm_s=0;
				}
				Get_alarm.AlarmTime.Hours=alarm_h;
				Get_alarm.AlarmTime.Minutes=alarm_m;
				Get_alarm.AlarmTime.Seconds=alarm_s;
				def_config();
				HAL_RTC_SetAlarm_IT(&hrtc,&Get_alarm,FORMAT_BIN);
			}
			if(set==3)
			{
				if(mode==0)
				{
					year+=1;
				}
				if(mode==1)
				{
					month+=1;
					if(month>12)
						month=1;
				}
				if(mode==2)
				{
					date+=1;
					if(month==1||month==3||month==5||month==7||month==8||month==10||month==12)
					{
						if(date>31)
							date=1;
					}
					if(month==4||month==6||month==9||month==11)
					{
						if(date>30)
							date=1;
					}
					if(month==2)
					{
						if(year_judge(year))
						{
							if(date>29)
								date=1;
						}
						else
						{
							if(date>28)
								date=1;
						}
					}
				}
				Get_date.Year=year;
				Get_date.Month=month;
				Get_date.Date=date;
				HAL_RTC_SetDate(&hrtc,&Get_date,FORMAT_BIN);
			}
		}
	}
	if(GPIO_Pin==GPIO_PIN_4)
	{
		if(HAL_GPIO_ReadPin(GPIOG,GPIO_PIN_4)==GPIO_PIN_SET)
		{
			if(set==1)
			{
				if(mode==0)
				{
					Hours-=1;
					if(Hours<0)
						Hours=23;
				}
				if(mode==1)
				{
					Minutes-=1;
					if(Minutes<0)
						Minutes=59;
				}
				if(mode==2)
				{
					Seconds-=1;
					if(Seconds<0)
						Seconds=59;
				}
				Get_time.Hours=Hours;
				Get_time.Minutes=Minutes;
				Get_time.Seconds=Seconds;
				HAL_RTC_SetTime(&hrtc,&Get_time,FORMAT_BIN);
			}
			if(set==2)
			{
				if(mode==0)
				{
					alarm_h-=1;
					if(alarm_h<0)
						alarm_h=23;
				}
				if(mode==1)
				{
					alarm_m-=1;
					if(alarm_m<0)
						alarm_m=59;
				}
				if(mode==2)
				{
					alarm_s-=1;
					if(alarm_s<0)
						alarm_s=59;
				}
				Get_alarm.AlarmTime.Hours=alarm_h;
				Get_alarm.AlarmTime.Minutes=alarm_m;
				Get_alarm.AlarmTime.Seconds=alarm_s;
				def_config();
				HAL_RTC_SetAlarm_IT(&hrtc,&Get_alarm,FORMAT_BIN);
			}
			if(set==3)
			{
				if(mode==0)
				{
					year-=1;
					if(year<0)
						year=0;
				}
				if(mode==1)
				{
					month-=1;
					if(month<1)
						month=12;
				}
				if(mode==2)
				{
					date-=1;
					if(month==1||month==3||month==5||month==7||month==8||month==10||month==12)
					{
						if(date<1)
							date=31;
					}
					if(month==4||month==6||month==9||month==11)
					{
						if(date<1)
							date=30;
					}
					if(mode==2)
					{
						if(year_judge(year))
						{
							if(date<1)
								date=29;
						}
						else
						{
							if(date<1)
								date=28;
						}
					}
				}
				Get_date.Year=year;
				Get_date.Month=month;
				Get_date.Date=date;
				HAL_RTC_SetDate(&hrtc,&Get_date,FORMAT_BIN);
			}
		}
	}
	
}

uint8_t uart_cnt=0;
char rx_dat[2];
char rx_data[10];
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if(huart==&huart1)
	{
		rx_data[uart_cnt]=rx_dat[0];
		uart_cnt++;
		if(HAL_UART_Receive_IT(&huart1,(uint8_t*)rx_dat,1)!=HAL_OK)
		{
			huart1.RxState=HAL_UART_STATE_READY;
			__HAL_UNLOCK(&huart1);
		}
		if(rx_data[0]=='A')
		{
			if(uart_cnt>9)
			{
				if(rx_data[9]=='B')
				{
					if(rx_data[1]-48==0)
					{
						Hours=((rx_data[2]-48)*10+(rx_data[3]-48)*1);
						Minutes=((rx_data[4]-48)*10+(rx_data[5]-48)*1);
						Seconds=((rx_data[6]-48)*10+(rx_data[7]-48)*1);
						if(Hours<=23&&Hours>=0)
							Get_time.Hours=Hours;
						if(Minutes<=59&&Minutes>=0)
							Get_time.Minutes=Minutes;
						if(Seconds<=59&&Seconds>=0)
							Get_time.Seconds=Seconds;
						HAL_RTC_SetTime(&hrtc,&Get_time,FORMAT_BIN);
					}
					if(rx_data[1]-48==1)
					{
						alarm_h=((rx_data[2]-48)*10+(rx_data[3]-48)*1);
						alarm_m=((rx_data[4]-48)*10+(rx_data[5]-48)*1);
						alarm_s=((rx_data[6]-48)*10+(rx_data[7]-48)*1);
						if(alarm_h<=23&&alarm_h>=0)
							Get_alarm.AlarmTime.Hours=alarm_h;
						if(alarm_m<=59&&alarm_m>=0)
							Get_alarm.AlarmTime.Minutes=alarm_m;
						if(alarm_s<=59&&alarm_s>=0)
							Get_alarm.AlarmTime.Seconds=alarm_s;
						def_config();
						HAL_RTC_SetAlarm_IT(&hrtc,&Get_alarm,FORMAT_BIN);
					}
					if(rx_data[8]-48==0)
						__HAL_RTC_ALARM_DISABLE_IT(&hrtc, RTC_IT_ALRA);
					if(rx_data[8]-48==1)
						__HAL_RTC_ALARM_ENABLE_IT(&hrtc, RTC_IT_ALRA);
					uart_cnt=0;
				}
				if(rx_data[9]=='C')
				{
					year=((rx_data[1]-48)*1000+(rx_data[2]-48)*100+(rx_data[3]-48)*10+(rx_data[4]-48)*1)-2024;
					month=((rx_data[5]-48)*10+(rx_data[6]-48)*1);
					date=((rx_data[7]-48)*10+(rx_data[8]-48)*1);
					if(year>=0)
						Get_date.Year=year;
					if(month>=1&&month<=12)
						Get_date.Month=month;
					if(date>=1)
					{
						if(month==1||month==3||month==5||month==7||month==8||month==10||month==12)
						{
							if(date<=31)
								Get_date.Date=date;
						}
						if(month==4||month==6||month==9||month==11)
						{
							if(date<=30)
								Get_date.Date=date;
						}
						if(month==2)
						{
							if(year_judge(year+2024))
							{
								if(date<=29)
									Get_date.Date=date;
							}
							else
							{
								if(date<=28)
									Get_date.Date=date;
							}
						}
					}
					HAL_RTC_SetDate(&hrtc,&Get_date,FORMAT_BIN);
				}
				uart_cnt=0;
			}
		}
		else
			uart_cnt=0;
		HAL_UART_Receive_IT(&huart1,(uint8_t*)rx_dat,1);
	}
}

void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
	tim_up=1;
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
	uint8_t set_last=0;
	uint8_t tim_up_last=0;
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C2_Init();
  MX_RTC_Init();
  MX_TIM1_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
	OLED_Init();
	OLED_Clear();
	HAL_UART_Receive_IT(&huart1,(uint8_t*)rx_dat,1);
	__HAL_RCC_PWR_CLK_ENABLE();
	HAL_PWR_EnableBkUpAccess();
	DHT11_Strat();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
		HAL_RTC_GetTime(&hrtc,&Get_time,FORMAT_BIN);
		HAL_RTC_GetDate(&hrtc,&Get_date,FORMAT_BIN);
		if(set_last!=set)
		{
			OLED_Clear();
			set_last=set;
		}
		if(tim_up_last!=tim_up)
		{
			OLED_Clear();
			tim_up_last=tim_up;
		}
		if(set==0&&tim_up==0)
		{
			DHT11_Read_Data(&temp,&humi);
			OLED_ShowNum(0,0,Get_time.Hours,2,16);
			OLED_ShowString(16,0,(uint8_t*)":",16);
			OLED_ShowNum(24,0,Get_time.Minutes,2,16);
			OLED_ShowString(40,0,(uint8_t*)":",16);
			OLED_ShowNum(48,0,Get_time.Seconds,2,16);
			
			OLED_ShowNum(0,2,(Get_date.Year+2024),4,16);
			OLED_ShowString(32,2,(uint8_t*)"/",16);
			OLED_ShowNum(40,2,Get_date.Month,2,16);
			OLED_ShowString(56,2,(uint8_t*)"/",16);
			OLED_ShowNum(64,2,Get_date.Date,2,16);
			
			Calculate_week((Get_date.Year+2024),Get_date.Month,Get_date.Date);
			
			OLED_ShowString(0,6,(uint8_t*)"temp:",16);
			OLED_ShowNum(40,6,temp,2,16);
			OLED_ShowString(56,6,(uint8_t*)"humi:",16);
			OLED_ShowNum(96,6,humi,2,16);
		}
		if(set==1)
		{
			OLED_ShowString(0,0,(uint8_t*)"Time set",16);
			OLED_ShowNum(0,2,Hours,2,16);
			OLED_ShowString(16,2,(uint8_t*)":",16);
			OLED_ShowNum(24,2,Minutes,2,16);
			OLED_ShowString(40,2,(uint8_t*)":",16);
			OLED_ShowNum(48,2,Seconds,2,16);
		}
		if(set==2)
		{
			OLED_ShowString(0,0,(uint8_t*)"Alarm set",16);
			OLED_ShowNum(0,2,alarm_h,2,16);
			OLED_ShowString(16,2,(uint8_t*)":",16);
			OLED_ShowNum(24,2,alarm_m,2,16);
			OLED_ShowString(40,2,(uint8_t*)":",16);
			OLED_ShowNum(48,2,alarm_s,2,16);
		}
		if(set==3)
		{
			OLED_ShowString(0,0,(uint8_t*)"Date set",16);
			OLED_ShowNum(0,2,(year+2024),4,16);
			OLED_ShowString(32,2,(uint8_t*)"/",16);
			OLED_ShowNum(40,2,month,2,16);
			OLED_ShowString(56,2,(uint8_t*)"/",16);
			OLED_ShowNum(64,2,date,2,16);
		}
		if(set==1||set==2)
		{
			if(mode==0)
				OLED_ShowString(0,4,(uint8_t*)"h set",16);
			if(mode==1)
				OLED_ShowString(0,4,(uint8_t*)"m set",16);
			if(mode==2)
				OLED_ShowString(0,4,(uint8_t*)"s set",16);
		}
		if(set==3)
		{
			if(mode==0)
				OLED_ShowString(0,4,(uint8_t*)"y set",16);
			if(mode==1)
				OLED_ShowString(0,4,(uint8_t*)"m set",16);
			if(mode==2)
				OLED_ShowString(0,4,(uint8_t*)"d set",16);
		}
		if(set==0&&tim_up==1)
		{
			beep_start();
			HAL_Delay(200);
			beep_stop();
			HAL_Delay(200);
			OLED_ShowString(0,2,(uint8_t*)"Alarm Ringing...",16);
		}
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE|RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
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
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
