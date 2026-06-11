#ifndef __MOLED_H
#define __MOLED_H

#include <stdint.h>

/* Console 风格的 OLED 输出封装。 */
typedef enum
{
	MOLED_ICON_SMILE = 0,
	MOLED_ICON_HEART,
	MOLED_ICON_CHECK,
	MOLED_ICON_ARROW_RIGHT,
} MOLED_IconTypeDef;

void MOLED_Init(void);
void MOLED_Clear(void);
void MOLED_SetCursor(uint8_t Line, uint8_t Column);
void MOLED_PutChar(char Char);
void MOLED_PutIcon(MOLED_IconTypeDef Icon);
void MOLED_Write(const char *String);
void MOLED_WriteLine(const char *String);
void MOLED_ScrollText(uint8_t Line, const char *String);
void MOLED_Task(void);

#endif