#include "lcd1602.h"

void LCD1602_DataToggle(char s)
{
	for(int i=0; i<8; i++){
		if((s>>i)&1){ //check bit
			switch(i){
				case 0: HAL_GPIO_WritePin(D0_GPIO_Port, D0_Pin, GPIO_PIN_SET);break;
				case 1: HAL_GPIO_WritePin(D1_GPIO_Port, D1_Pin, GPIO_PIN_SET);break;
				case 2: HAL_GPIO_WritePin(D2_GPIO_Port, D2_Pin, GPIO_PIN_SET);break;
				case 3: HAL_GPIO_WritePin(D3_GPIO_Port, D3_Pin, GPIO_PIN_SET);break;
				case 4: HAL_GPIO_WritePin(D4_GPIO_Port, D4_Pin, GPIO_PIN_SET);break;
				case 5: HAL_GPIO_WritePin(D5_GPIO_Port, D5_Pin, GPIO_PIN_SET);break;
				case 6: HAL_GPIO_WritePin(D6_GPIO_Port, D6_Pin, GPIO_PIN_SET);break;
				case 7: HAL_GPIO_WritePin(D7_GPIO_Port, D7_Pin, GPIO_PIN_SET);break;
				default:break;
			}
		}else{
			switch(i){
				case 0: HAL_GPIO_WritePin(D0_GPIO_Port, D0_Pin, GPIO_PIN_RESET);break;
				case 1: HAL_GPIO_WritePin(D1_GPIO_Port, D1_Pin, GPIO_PIN_RESET);break;
				case 2: HAL_GPIO_WritePin(D2_GPIO_Port, D2_Pin, GPIO_PIN_RESET);break;
				case 3: HAL_GPIO_WritePin(D3_GPIO_Port, D3_Pin, GPIO_PIN_RESET);break;
				case 4: HAL_GPIO_WritePin(D4_GPIO_Port, D4_Pin, GPIO_PIN_RESET);break;
				case 5: HAL_GPIO_WritePin(D5_GPIO_Port, D5_Pin, GPIO_PIN_RESET);break;
				case 6: HAL_GPIO_WritePin(D6_GPIO_Port, D6_Pin, GPIO_PIN_RESET);break;
				case 7: HAL_GPIO_WritePin(D7_GPIO_Port, D7_Pin, GPIO_PIN_RESET);break;
				default:break;
			}
		}
	}
}

void LCD1602_Wait(void)
{
	uint8_t sta;

	//HAL_GPIO_TogglePin(GPIOA,0xff);

	LCD_RS_Clr();
	LCD_RW_Set();
	do
	{
		LCD_EN_Set();
		HAL_Delay(5);
		sta = HAL_GPIO_ReadPin(RW_GPIO_Port, RW_Pin);
		LCD_EN_Clr();
	}while(sta & 0x80);
}

void LCD1602_WriteCMD(unsigned char cmd)
{
	LCD1602_Wait();
	HAL_Delay(1);
	LCD_RS_Clr();
	HAL_Delay(1);
	LCD_RW_Clr();
	HAL_Delay(1);


	LCD1602_DataToggle(cmd);

	HAL_Delay(1);
	LCD_EN_Set();
	HAL_Delay(1);
	LCD_EN_Clr();
	HAL_Delay(1);
}

void LCD1602_WriteData(unsigned char dat)
{
	LCD1602_Wait();
	HAL_Delay(1);
	LCD_RS_Set();
	HAL_Delay(1);
	LCD_RW_Clr();
	HAL_Delay(1);

	LCD1602_DataToggle(dat);

	HAL_Delay(1);
	LCD_EN_Set();
	HAL_Delay(1);
	LCD_EN_Clr();
	HAL_Delay(1);
}

void LCD1602_CLS(void)
{
	LCD1602_WriteCMD(0x01);
}

//Set the Cursor Place to x,y Pixel
void LCD1602_SetCursor(unsigned char x, unsigned char y)
{
	uint8_t addr;
	if (y == 0)
	{
		addr = 0x00 + x;
	}
	else
	{
		addr = 0x40 + x;
	}
	LCD1602_WriteCMD(addr | 0x80);
}

//For Looping String Shown
void LCD1602_LinePrint(unsigned char lineNumber, char *str, int delay)
{
	for(int i=0;i<15;i++)//hold 16 - 1
	{
		LCD1602_SetCursor(0, lineNumber);//Set the Front Cursor
		char *strrp = str + sizeof(char) * i;//Move Memory to next
		if(*strrp == '\0')//catch the final char
		{
			LCD1602_CLS(); //stops the output
			return;//finish
		}
		else
		{
			while(*strrp != '\0')//Wait String Ends
			{
				LCD1602_WriteData(*strrp++);//Get bytes of Char And Put Into Screen
			}
		}
		HAL_Delay(delay);//wait the char finish output
	}
}

//Simple Show String
void LCD1602_ShowString(unsigned char x, unsigned char y, char *str)
{
	LCD1602_SetCursor(x, y);
	while(*str != '\0')//Wait String Ends
	{
		LCD1602_WriteData(*str++);//Get bytes of Char And Put Into Screen
	}
}

//for LCD1602 Initialization
void LCD1602_Init(void)
{
	LCD1602_WriteCMD(0x38);	//Initialization
	LCD1602_WriteCMD(0x0c);	//Lock Courser
	LCD1602_WriteCMD(0x06);	//address +1
	LCD1602_WriteCMD(0x01);	//CLR Screen

}

