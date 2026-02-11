#ifndef __OLED_H
#define __OLED_H


#include "bsp_system.h"


void OLED_Init(void);
void OLED_Clear(void);
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char);
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String);
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length);
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_Clear_Part(uint8_t Line, uint8_t start, uint8_t end);
void OLED_ShowWord(uint8_t Line, uint8_t Column, uint8_t Chinese);
void OLED_ShowChinese(uint8_t Line, uint8_t Column, uint8_t *Chinese,uint8_t Length);

void OLED_ShowProgress(uint8_t Progress);

void OLED_DrawCN_Block(uint8_t x, uint8_t y, const uint8_t *data);
void OLED_ShowStr(uint8_t x, uint8_t y, char *str);

void OLED_DrawBMP(uint8_t x0, uint8_t y0, uint8_t w, uint8_t h, const uint8_t *BMP);


#endif
