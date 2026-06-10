#ifndef __LCD1602_H
#define __LCD1602_H	 

//1602 RS_
#define	LCD_RS_Set()	HAL_GPIO_WritePin(RS_GPIO_Port,RS_Pin,GPIO_PIN_SET)
#define	LCD_RS_Clr()	HAL_GPIO_WritePin(RS_GPIO_Port,RS_Pin,GPIO_PIN_RESET)
//1602 RW_
#define	LCD_RW_Set()	HAL_GPIO_WritePin(RW_GPIO_Port,RW_Pin,GPIO_PIN_SET)
#define	LCD_RW_Clr()	HAL_GPIO_WritePin(RW_GPIO_Port,RW_Pin,GPIO_PIN_RESET)
//1602 E_
#define	LCD_EN_Set()	HAL_GPIO_WritePin(E_GPIO_Port,E_Pin,GPIO_PIN_SET)
#define	LCD_EN_Clr()	HAL_GPIO_WritePin(E_GPIO_Port,E_Pin,GPIO_PIN_RESET)

void LCD1602_DataToggle(char s);
void LCD1602_Wait(void);
void LCD1602_WriteCMD(unsigned char cmd);
void LCD1602_WriteData(unsigned char dat);
void LCD1602_CLS(void);
void LCD1602_SetCursor(unsigned char x, unsigned char y);
void LCD1602_ShowString(unsigned char x, unsigned char y, char *str);
void LCD1602_LinePrint(unsigned char lineNumber, char *str, int delay);
void LCD1602_Init(void);

#endif
