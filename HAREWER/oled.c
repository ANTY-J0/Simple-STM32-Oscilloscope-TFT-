#include "oled.h"
#include "oledfont.h"
#include "delay.h"
#include <string.h>
#include <stdlib.h>

/* ========== 缓冲区（用于 DMA 批量传输） ========== */
static uint16_t dma_line_buffer[240];  // 一行缓冲区（240像素）
static uint8_t dma_initialized = 0;

/* ========== DMA 配置（改名避免冲突） ========== */
void DMA_Config(void)
{
    DMA_InitTypeDef DMA_InitStructure;
    
    if(dma_initialized) return;
    
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
    
    // DMA1 通道3 用于 SPI1 TX
    DMA_DeInit(DMA1_Channel3);
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&SPI1->DR;
    DMA_InitStructure.DMA_MemoryBaseAddr = 0;  // 动态设置
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
    DMA_InitStructure.DMA_BufferSize = 0;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_Init(DMA1_Channel3, &DMA_InitStructure);
    
    // 使能 SPI1 TX DMA 请求
    SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, ENABLE);
    
    dma_initialized = 1;
}

/* ========== 硬件 SPI 初始化 ========== */
void SPI_Hardware_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    SPI_InitTypeDef SPI_InitStructure;
    
    // 使能时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_SPI1, ENABLE);
    
    // 1. SPI 引脚 PA5(SCK), PA7(MOSI) 复用推挽输出
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // 2. 控制引脚 PB0(RST), PB1(DC), PB10(BLK) 推挽输出
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    // 初始状态：RST、DC、BLK 拉高
    GPIO_SetBits(GPIOB, GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_10);
    
    // 3. SPI 配置（18MHz）
    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;  // 18MHz
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;
    SPI_Init(SPI1, &SPI_InitStructure);
    
    SPI_Cmd(SPI1, ENABLE);
    
    // 初始化 DMA
    DMA_Config();
}

/* ========== DMA 批量发送 ========== */
void DMA_Send(uint8_t *data, uint16_t len)
{
    if(len == 0) return;
    
    // 等待上次传输完成
    while(DMA_GetFlagStatus(DMA1_FLAG_TC3));
    DMA_ClearFlag(DMA1_FLAG_TC3);
    
    // 配置 DMA
    DMA_Cmd(DMA1_Channel3, DISABLE);
    DMA1_Channel3->CMAR = (uint32_t)data;
    DMA1_Channel3->CNDTR = len;
    DMA_Cmd(DMA1_Channel3, ENABLE);
    
    // 等待传输完成
    while(!DMA_GetFlagStatus(DMA1_FLAG_TC3));
    DMA_ClearFlag(DMA1_FLAG_TC3);
}

/* ========== 硬件 SPI 写字节（用于命令和少量数据） ========== */
void LCD_Writ_Bus(u8 dat)
{
    while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
    SPI_I2S_SendData(SPI1, dat);
    while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET);
}

/* ========== 写命令 ========== */
void LCD_WR_REG(u8 cmd)
{
    OLED_DC_L();
    LCD_Writ_Bus(cmd);
    OLED_DC_H();
}

/* ========== 写数据（16位 RGB565）- 普通方式 ========== */
void LCD_WR_DATA(u16 dat)
{
    OLED_DC_H();
    LCD_Writ_Bus(dat >> 8);
    LCD_Writ_Bus(dat);
}

/* ========== 写数据（8位） ========== */
void LCD_WR_DATA8(u8 dat)
{
    OLED_DC_H();
    LCD_Writ_Bus(dat);
}

/* ========== DMA 批量写数据（最快！） ========== */
void LCD_WR_DATA_Batch(uint16_t *data, uint32_t len)
{
    if(len == 0) return;
    OLED_DC_H();
    DMA_Send((uint8_t*)data, len * 2);
}

/* ========== 设置显示区域 ========== */
void Address_Set(u16 x1, u16 y1, u16 x2, u16 y2)
{
    LCD_WR_REG(0x2A);
    LCD_WR_DATA8(x1 >> 8);
    LCD_WR_DATA8(x1);
    LCD_WR_DATA8(x2 >> 8);
    LCD_WR_DATA8(x2);
    
    LCD_WR_REG(0x2B);
    LCD_WR_DATA8(y1 >> 8);
    LCD_WR_DATA8(y1);
    LCD_WR_DATA8(y2 >> 8);
    LCD_WR_DATA8(y2);
    
    LCD_WR_REG(0x2C);
}

/* ========== 初始化屏幕 ========== */
void Lcd_Init(void)
{
    SPI_Hardware_Init();
    
    // 复位
    OLED_RST_L();
    delay_ms(20);
    OLED_RST_H();
    delay_ms(20);
    
    // 背光开启
    OLED_BLK_H();
    
    // ST7789 初始化序列
    LCD_WR_REG(0x11); delay_ms(120);
    LCD_WR_REG(0x36); LCD_WR_DATA8(0x00);
    LCD_WR_REG(0x3A); LCD_WR_DATA8(0x05);
    LCD_WR_REG(0xB2); LCD_WR_DATA8(0x0C); LCD_WR_DATA8(0x0C); LCD_WR_DATA8(0x00); LCD_WR_DATA8(0x33); LCD_WR_DATA8(0x33);
    LCD_WR_REG(0xB7); LCD_WR_DATA8(0x35);
    LCD_WR_REG(0xBB); LCD_WR_DATA8(0x19);
    LCD_WR_REG(0xC0); LCD_WR_DATA8(0x2C);
    LCD_WR_REG(0xC2); LCD_WR_DATA8(0x01);
    LCD_WR_REG(0xC3); LCD_WR_DATA8(0x12);
    LCD_WR_REG(0xC4); LCD_WR_DATA8(0x20);
    LCD_WR_REG(0xC6); LCD_WR_DATA8(0x0F);
    LCD_WR_REG(0xD0); LCD_WR_DATA8(0xA4); LCD_WR_DATA8(0xA1);
    LCD_WR_REG(0xE0); LCD_WR_DATA8(0xD0); LCD_WR_DATA8(0x04); LCD_WR_DATA8(0x0D); LCD_WR_DATA8(0x11); LCD_WR_DATA8(0x13);
    LCD_WR_REG(0xE0); LCD_WR_DATA8(0x2B); LCD_WR_DATA8(0x3F); LCD_WR_DATA8(0x54); LCD_WR_DATA8(0x4C); LCD_WR_DATA8(0x18);
    LCD_WR_REG(0xE0); LCD_WR_DATA8(0x0D); LCD_WR_DATA8(0x0B); LCD_WR_DATA8(0x1F); LCD_WR_DATA8(0x23);
    LCD_WR_REG(0xE1); LCD_WR_DATA8(0xD0); LCD_WR_DATA8(0x04); LCD_WR_DATA8(0x0C); LCD_WR_DATA8(0x11); LCD_WR_DATA8(0x13);
    LCD_WR_REG(0xE1); LCD_WR_DATA8(0x2C); LCD_WR_DATA8(0x3F); LCD_WR_DATA8(0x44); LCD_WR_DATA8(0x51); LCD_WR_DATA8(0x2F);
    LCD_WR_REG(0xE1); LCD_WR_DATA8(0x1F); LCD_WR_DATA8(0x1F); LCD_WR_DATA8(0x20); LCD_WR_DATA8(0x23);
    LCD_WR_REG(0x21);
    LCD_WR_REG(0x29);
    
    // 初始化一行缓冲区
    for(int i = 0; i < 240; i++) {
        dma_line_buffer[i] = BLACK;
    }
    
    LCD_Clear_Fast(BLACK);
}

/* ========== DMA 加速清屏 ========== */
void LCD_Clear_Fast(u16 Color)
{
    // 填充一行缓冲区
    for(int i = 0; i < LCD_W; i++) {
        dma_line_buffer[i] = Color;
    }
    
    Address_Set(0, 0, LCD_W - 1, LCD_H - 1);
    
    // 用 DMA 逐行发送
    for(int y = 0; y < LCD_H; y++) {
        LCD_WR_DATA_Batch(dma_line_buffer, LCD_W);
    }
}

/* ========== DMA 加速区域填充 ========== */
void LCD_Fill_Fast(u16 x1, u16 y1, u16 x2, u16 y2, u16 color)
{
    u16 width = x2 - x1 + 1;
    u16 height = y2 - y1 + 1;
    
    // 填充一行缓冲区
    for(int i = 0; i < width; i++) {
        dma_line_buffer[i] = color;
    }
    
    Address_Set(x1, y1, x2, y2);
    
    // 用 DMA 逐行发送
    for(int y = 0; y < height; y++) {
        LCD_WR_DATA_Batch(dma_line_buffer, width);
    }
}

/* ========== 普通清屏（兼容旧代码） ========== */
void LCD_Clear(u16 Color)
{
    LCD_Clear_Fast(Color);
}

/* ========== 普通填充（兼容旧代码） ========== */
void LCD_Fill(u16 x1, u16 y1, u16 x2, u16 y2, u16 color)
{
    LCD_Fill_Fast(x1, y1, x2, y2, color);
}

/* ========== 画点 ========== */
void LCD_DrawPoint(u16 x, u16 y)
{
    if(x >= LCD_W || y >= LCD_H) return;
    Address_Set(x, y, x, y);
    LCD_WR_DATA(WHITE);
}

/* ========== 画线 ========== */
void LCD_DrawLine(u16 x1, u16 y1, u16 x2, u16 y2)
{
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;
    int e2;
    
    while(1)
    {
        LCD_DrawPoint(x1, y1);
        if(x1 == x2 && y1 == y2) break;
        e2 = 2 * err;
        if(e2 > -dy) { err -= dy; x1 += sx; }
        if(e2 < dx)  { err += dx; y1 += sy; }
    }
}

/* ========== 快速画水平线 ========== */
void LCD_DrawHLine(u16 x1, u16 x2, u16 y, u16 color)
{
    if(y >= LCD_H) return;
    if(x1 >= LCD_W) x1 = LCD_W - 1;
    if(x2 >= LCD_W) x2 = LCD_W - 1;
    
    u16 width = x2 - x1 + 1;
    
    // 填充一行缓冲区
    for(int i = 0; i < width; i++) {
        dma_line_buffer[i] = color;
    }
    
    Address_Set(x1, y, x2, y);
    LCD_WR_DATA_Batch(dma_line_buffer, width);
}

/* ========== 快速画垂直线 ========== */
void LCD_DrawVLine(u16 x, u16 y1, u16 y2, u16 color)
{
    if(x >= LCD_W) return;
    if(y1 >= LCD_H) y1 = LCD_H - 1;
    if(y2 >= LCD_H) y2 = LCD_H - 1;
    
    // 逐点画（垂直线不适合批量优化）
    for(u16 y = y1; y <= y2; y++) {
        LCD_DrawPoint(x, y);
    }
}

/* ========== 显示字符（6x8字体） ========== */
void LCD_ShowChar(u16 x, u16 y, char chr)
{
    u8 i, j;
    u8 temp;
    u8 index;
    
    if(x > LCD_W - 6 || y > LCD_H - 8) return;
    if(chr < ' ' || chr > '~') return;
    
    index = chr - ' ';
    
    for(i = 0; i < 6; i++)
    {
        temp = F6x8[index][i];
        for(j = 0; j < 8; j++)
        {
            if(temp & (1 << j))
                LCD_DrawPoint(x + i, y + j);
        }
    }
}

/* ========== 显示字符串 ========== */
void LCD_ShowString(u16 x, u16 y, const char *str)
{
    while(*str)
    {
        LCD_ShowChar(x, y, *str);
        x += 6;
        if(x > LCD_W - 6)
        {
            x = 0;
            y += 8;
        }
        str++;
    }
}

/* ========== 显示数字 ========== */
void LCD_ShowNum(u16 x, u16 y, u32 num, u8 len)
{
    char buf[12];
    u8 i;
    for(i = 0; i < len; i++)
    {
        buf[len - 1 - i] = (num % 10) + '0';
        num /= 10;
    }
    buf[len] = '\0';
    LCD_ShowString(x, y, buf);
}