#ifndef __OLED_H
#define __OLED_H

#include "stm32f10x.h"

/* ========== 引脚定义 ========== */
#define OLED_SCLK_PIN    GPIO_Pin_5   // PA5 -> SCLK
#define OLED_SDIN_PIN    GPIO_Pin_7   // PA7 -> SDIN
#define OLED_RST_PIN     GPIO_Pin_0   // PB0 -> RES
#define OLED_DC_PIN      GPIO_Pin_1   // PB1 -> DC
#define OLED_BLK_PIN     GPIO_Pin_10  // PB10 -> BLK

#define OLED_SCLK_H()    GPIO_SetBits(GPIOA, OLED_SCLK_PIN)
#define OLED_SCLK_L()    GPIO_ResetBits(GPIOA, OLED_SCLK_PIN)
#define OLED_SDIN_H()    GPIO_SetBits(GPIOA, OLED_SDIN_PIN)
#define OLED_SDIN_L()    GPIO_ResetBits(GPIOA, OLED_SDIN_PIN)
#define OLED_RST_H()     GPIO_SetBits(GPIOB, OLED_RST_PIN)
#define OLED_RST_L()     GPIO_ResetBits(GPIOB, OLED_RST_PIN)
#define OLED_DC_H()      GPIO_SetBits(GPIOB, OLED_DC_PIN)
#define OLED_DC_L()      GPIO_ResetBits(GPIOB, OLED_DC_PIN)
#define OLED_BLK_H()     GPIO_SetBits(GPIOB, OLED_BLK_PIN)
#define OLED_BLK_L()     GPIO_ResetBits(GPIOB, OLED_BLK_PIN)

/* ========== 屏幕参数 ========== */
#define LCD_W   240
#define LCD_H   240

/* ========== 颜色定义 ========== */
#define WHITE           0xFFFF
#define BLACK           0x0000
#define RED             0xF800
#define GREEN           0x07E0
#define BLUE            0x001F
#define YELLOW          0xFFE0

/* ========== 函数声明 ========== */
void Lcd_Init(void);
void LCD_Clear(u16 Color);
void LCD_DrawPoint(u16 x, u16 y);
void LCD_DrawLine(u16 x1, u16 y1, u16 x2, u16 y2);
void LCD_DrawHLine(u16 x1, u16 x2, u16 y, u16 color);
void LCD_DrawVLine(u16 x, u16 y1, u16 y2, u16 color);
void LCD_ShowString(u16 x, u16 y, const char *str);
void LCD_ShowNum(u16 x, u16 y, u32 num, u8 len);
void LCD_Fill(u16 x1, u16 y1, u16 x2, u16 y2, u16 color);
void LCD_Fill_Fast(u16 x1, u16 y1, u16 x2, u16 y2, u16 color);
void LCD_Clear_Fast(u16 Color);
void LCD_WR_DATA_Batch(uint16_t *data, uint32_t len);
void Address_Set(u16 x1, u16 y1, u16 x2, u16 y2);

#endif
