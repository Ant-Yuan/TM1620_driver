/*
* TM1620 LED driver V1.0
* yujianyuan@20170311
*
*/
#include "stm32f0xx_hal.h"
#include "TM1620.h"


//共阴字形码
uint8_t fontCode[]={	0x3F,0x06,0x5B,0x4F,0x66,	//0-4
											0x6D,0x7D,0x07,0x7F,0x6F,	//5-9
											0x77,0x7C,0x39,0x5E,0x79, //A-E
											0x71,0x73,0x3E,0x76,0x38,	//F,P,U,H,L
											0xFF											//test led
}; 


//DIN PA3 ,CLK PA4 ,STB/CS PA5
#define	DIO_HIGH 	GPIOA->BSRR = GPIO_PIN_3;
#define	DIO_LOW 	GPIOA->BRR = GPIO_PIN_3;

#define	CLK_HIGH 	GPIOA->BSRR = GPIO_PIN_4;
#define	CLK_LOW 	GPIOA->BRR = GPIO_PIN_4;

#define	STB_HIGH 	GPIOA->BSRR = GPIO_PIN_5;
#define	STB_LOW 	GPIOA->BRR = GPIO_PIN_5;


//TM1620_DATA寄存器
#define AUTO_ADDR_MODE		0x00	//地址自增模式
#define FIXED_ADDR_MODE		0x04	//固定地址模式

typedef enum {	TM1620_DISP_MODE	=	0x0,	//显示模式设置
								TM1620_DATA				=	0x1,	//数据命令设置
								TM1620_BRIGHT			= 0x2, 	//显示控制命令设置
								TM1620_ADDR				=	0x3 	//地址命令设置
}TM1620_CMD;


char DisplayArray[MAX_TUBE_NUM];

int TM1620_LowLevel_Init(void);
int TM1620_Write_8bit(uint8_t data);
void TM1620_Write(TM1620_CMD cmd,uint8_t data);
uint8_t chToFontcode(char ch);
void u16ToDisplayArray(uint16_t uin);
void TM1620_Init_Test(void);


                                                                                                                                 
int TM1620_Init(void)
{
	
	TM1620_LowLevel_Init();

	TM1620_Init_Test();
	
	HAL_Delay(500);
	
	return 0;
} 

int TM1620_LowLevel_Init(void)
{
	__HAL_RCC_GPIOA_CLK_ENABLE();
	GPIO_InitTypeDef GPIO_InitStruct;
	
	GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_3|GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	
	DIO_HIGH;
	STB_HIGH;
	CLK_HIGH;
	HAL_Delay(10);
	
	return 0;
}

void TM1620_Write(TM1620_CMD cmd,uint8_t data)
{
	switch(cmd)
	{
		case TM1620_DISP_MODE:
			STB_LOW;
			//显示模式6位8段,0b0000_0010
			TM1620_Write_8bit(data&0x03);
			STB_HIGH;
			//HAL_Delay(1);
			break;
		case TM1620_DATA:
			STB_LOW;
			//数据读写模式0b0100_0000
			TM1620_Write_8bit(0x40|(data&0x7));
			STB_HIGH;
			//HAL_Delay(1);
			break;
		case TM1620_BRIGHT:
			STB_LOW;
			//亮度设置0b1000_1xxx
			TM1620_Write_8bit(0x88|(0x7&data));
			STB_HIGH;
			//HAL_Delay(1);
			break;
		case TM1620_ADDR://显示地址
			STB_LOW;
			TM1620_Write_8bit(0xC0|(0xF&data));
			//HAL_Delay(1);
			break;
	}
}

int TM1620_Write_8bit(uint8_t data)
{
	//HAL_Delay(1);
	for(int i=0;i<8;i++)
	{
		CLK_LOW;
		if(data&0x1)
		{
			DIO_HIGH;
		}
		else
		{
			DIO_LOW;
		}
		data=data>>1;
		HAL_Delay(1);
		CLK_HIGH;
		//HAL_Delay(1);
	}
	return 0;
}

//在数码管上显示字符,该芯片最多6位数码管
void TM1620_Print(char* ch)
{
	uint8_t fcode[MAX_TUBE_NUM];
	
	for(int i=0;i<MAX_TUBE_NUM;i++)
	{
		fcode[i]=chToFontcode(ch[i]);
	}
	
	TM1620_Write(TM1620_DISP_MODE,0x02);//显示模式6位8段
	TM1620_Write(TM1620_DATA,AUTO_ADDR_MODE);			//地址自增模式
	
	//偶数为地址从0x00-0x0A分别为数码管的第1-6位
	//奇数地址显示无效
	TM1620_Write(TM1620_ADDR,0x00);			//起始地址0xC0
	
	for(int i=0;i<MAX_TUBE_NUM;i++)
	{
		TM1620_Write_8bit(fcode[i]);
		TM1620_Write_8bit(0x00);//因为奇数地址无效，所以补0
	}
	STB_HIGH;
	TM1620_Write(TM1620_BRIGHT,MAX_TUBE_BRIGHTNESS);			//亮度设置
}

//显示一个数字，长度最长为MAX_TUBE_NUM
void TM1620_Display(uint16_t din)
{
	u16ToDisplayArray(din);
	TM1620_Print(DisplayArray);
}


void TM1620_Init_Test(void)
{
	for(int i=0;i<MAX_TUBE_NUM;i++)
	{
		DisplayArray[i]='T';
	}
	TM1620_Print(DisplayArray);
	HAL_Delay(1000);
	u16ToDisplayArray(0);
	TM1620_Print(DisplayArray);
}

uint8_t chToFontcode(char ch)
{
	uint8_t fontcode=0;
	
	if(ch>='0'&&ch<='9')
	{
		fontcode	=	fontCode[ch-'0'];
	}
	else if(ch>='A'&&ch<='F')
	{
		fontcode 	=	fontCode[ch-'A'+10];
	}
	else if(ch=='P')
	{
		fontcode 	= fontCode[16];
	}
	else if(ch=='U')
	{
		fontcode 	= fontCode[17];
	}
	else if(ch=='H')
	{
		fontcode 	= fontCode[18];
	}
	else if(ch=='L')
	{
		fontcode 	= fontCode[19];
	}
	else if(ch=='T')
	{
		fontcode 	= fontCode[20];//test led全亮
	}
	else
	{
		fontcode = 0;//off
	}
	return fontcode;
}


void u16ToDisplayArray(uint16_t uin)
{
	uint16_t temp=uin;
	
	for(int i=(MAX_TUBE_NUM-1);i>=0;i--)
	{
		DisplayArray[i]=temp%10+'0';
		temp=temp/10;
	}
	
}



