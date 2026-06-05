#include "stm32f10x.h"
#include "delay.h"
#include "oled.h"
#include "adc.h"
#include "key.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* ========== 波形区域 ========== */
#define WAVE_TOP       40
#define WAVE_BOTTOM    200
#define WAVE_CENTER    ((WAVE_TOP + WAVE_BOTTOM) / 2)  // = 120，固定中线
#define WAVE_X_START   40      // 波形绘制起始 X 坐标（避开左侧刻度）
#define WAVE_X_END     235     // 波形绘制结束 X 坐标（留出右侧边距）
#define WAVE_X_WIDTH   (WAVE_X_END - WAVE_X_START)  // = 195

/* ========== 全局变量 ========== */
float y_scale = 2.0f;
int16_t y_offset = 0;
uint8_t pause_flag = 0;
uint8_t show_menu = 0;
uint8_t menu_index = 0;

uint16_t Paused_Buffer[ADC_BUFFER_SIZE];

char display_buf[32];

// 缓存上次显示的值
float last_vpp = -1;
float last_max = -1;
float last_min = -1;
float last_scl = -1;
int16_t last_y_offset = -999;
uint8_t last_pause_flag = 0;

// 刻度值缓存
float last_scale_top = -1;
float last_scale_center = -1;
float last_scale_bottom = -1;

/* ========== 函数声明 ========== */
void Draw_Grid(void);
void Draw_Scale(void);
void Draw_Waveform_Fast(uint16_t *buffer);
void Display_Info_Fast(void);
void Clear_Waveform_Area(void);
void Display_Menu(void);
void Reset_Display_Cache(void);

/* ========== 绘制动态刻度（电压标签） ========== */
void Draw_Scale(void)
{
    float top_voltage = 3.3f / y_scale;
    float center_voltage = top_voltage / 2;
    
    // 清除旧的刻度区域（左侧 45 像素宽）
    LCD_Fill_Fast(0, WAVE_TOP + y_offset - 4, 45, WAVE_BOTTOM + y_offset + 4, BLACK);
    
    // 显示顶部刻度
    sprintf(display_buf, "%.1fV", top_voltage);
    LCD_ShowString(2, WAVE_TOP + y_offset - 4, display_buf);
    
    // 显示中线刻度
    sprintf(display_buf, "%.1fV", center_voltage);
    LCD_ShowString(2, WAVE_CENTER - 4, display_buf);
    
    // 显示底部刻度
    LCD_ShowString(2, WAVE_BOTTOM + y_offset - 4, "0.0V");
    
    // 缓存当前刻度值
    last_scale_top = top_voltage;
    last_scale_center = center_voltage;
    last_scale_bottom = 0;
}

/* ========== 绘制网格 ========== */
void Draw_Grid(void)
{
    // 顶部边界线（跟随偏移）
    LCD_DrawHLine(WAVE_X_START, WAVE_X_END, WAVE_TOP + y_offset, GREEN);
    // 底部边界线（跟随偏移）
    LCD_DrawHLine(WAVE_X_START, WAVE_X_END, WAVE_BOTTOM + y_offset, GREEN);
    
    // X轴中线（固定，不随 y_offset 移动）
    LCD_DrawHLine(WAVE_X_START, WAVE_X_END, WAVE_CENTER, GREEN);
    
    // 垂直刻度线（固定在中线上）
    for(int x = WAVE_X_START; x <= WAVE_X_END; x += 50)
    {
        LCD_DrawVLine(x, WAVE_CENTER - 4, WAVE_CENTER + 4, GREEN);
    }
    
    // 绘制动态刻度标签
    Draw_Scale();
}

/* ========== 波形绘制和参数显示========== */
void Draw_Waveform_Fast(uint16_t *buffer)
{
    int16_t prev_x = WAVE_X_START;
    int16_t prev_y = WAVE_CENTER + y_offset;
    int16_t curr_x, curr_y;
    float percent;
    
    for(uint16_t i = 0; i < ADC_BUFFER_SIZE; i++)
    {
        curr_x = WAVE_X_START + (i * WAVE_X_WIDTH / ADC_BUFFER_SIZE);
        percent = buffer[i] / 4095.0f;
        curr_y = (WAVE_BOTTOM + y_offset) - (int16_t)(percent * (WAVE_BOTTOM - WAVE_TOP) * y_scale);
        
        if(curr_y < WAVE_TOP + y_offset) curr_y = WAVE_TOP + y_offset;
        if(curr_y > WAVE_BOTTOM + y_offset) curr_y = WAVE_BOTTOM + y_offset;
        
        if(i > 0)
        {
            // Bresenham 画线
            int dx = abs(curr_x - prev_x);
            int dy = abs(curr_y - prev_y);
            int sx = (prev_x < curr_x) ? 1 : -1;
            int sy = (prev_y < curr_y) ? 1 : -1;
            int err = dx - dy;
            int e2;
            int x = prev_x, y = prev_y;
            
            while(1)
            {
                Address_Set(x, y, x, y);
                LCD_WR_DATA(YELLOW);
                if(x == curr_x && y == curr_y) break;
                e2 = 2 * err;
                if(e2 > -dy) { err -= dy; x += sx; }
                if(e2 < dx)  { err += dx; y += sy; }
            }
        }
        else
        {
            Address_Set(curr_x, curr_y, curr_x, curr_y);
            LCD_WR_DATA(YELLOW);
        }
        
        prev_x = curr_x;
        prev_y = curr_y;
    }
}

/* ========== 局部清屏 ========== */
void Clear_Waveform_Area(void)
{
    int top = WAVE_TOP + y_offset + 1;
    int bottom = WAVE_BOTTOM + y_offset - 1;
    int center = WAVE_CENTER;
    
    // 清除顶部到中线之间的区域（保留中线）
    if(top < center - 1)
    {
        LCD_Fill_Fast(WAVE_X_START, top, WAVE_X_END, center - 1, BLACK);
    }
    // 清除中线到底部之间的区域（保留中线）
    if(center + 1 < bottom)
    {
        LCD_Fill_Fast(WAVE_X_START, center + 1, WAVE_X_END, bottom, BLACK);
    }
}

/* ========== 智能刷新文字 ========== */
void Display_Info_Fast(void)
{
    float max_voltage = 0, min_voltage = 3.3f;
    float voltage;
    uint16_t *display_buffer;
    
    if(pause_flag)
        display_buffer = Paused_Buffer;
    else
        display_buffer = ADC_Buffer;
    
    for(uint16_t i = 0; i < ADC_BUFFER_SIZE; i++)
    {
        voltage = display_buffer[i] * 3.3f / 4095.0f;
        if(voltage > max_voltage) max_voltage = voltage;
        if(voltage < min_voltage) min_voltage = voltage;
    }
    
    float vpp = max_voltage - min_voltage;
    
    if(vpp != last_vpp || max_voltage != last_max || min_voltage != last_min)
    {
        LCD_Fill_Fast(0, 0, LCD_W - 1, 35, BLACK);
        
        sprintf(display_buf, "Vpp:%.1fV", vpp);
        LCD_ShowString(0, 0, display_buf);
        
        sprintf(display_buf, "Mx:%.1fV Mn:%.1fV", max_voltage, min_voltage);
        LCD_ShowString(0, 12, display_buf);
        
        last_vpp = vpp;
        last_max = max_voltage;
        last_min = min_voltage;
    }
    
    if(y_scale != last_scl)
    {
        LCD_Fill_Fast(0, LCD_H - 16, LCD_W - 40, LCD_H, BLACK);
        sprintf(display_buf, "Scl:%.1f", y_scale);
        LCD_ShowString(0, LCD_H - 16, display_buf);
        last_scl = y_scale;
        
        // 缩放变化时，刻度会变化，需要重绘网格
        Draw_Grid();
    }
    
    if(y_offset != last_y_offset)
    {
        Draw_Grid();
        last_y_offset = y_offset;
    }
    
    if(pause_flag != last_pause_flag)
    {
        LCD_Fill_Fast(LCD_W - 30, LCD_H - 16, LCD_W, LCD_H, BLACK);
        if(pause_flag)
            LCD_ShowString(LCD_W - 30, LCD_H - 16, "P");
        else
            LCD_ShowString(LCD_W - 30, LCD_H - 16, "R");
        last_pause_flag = pause_flag;
    }
}

/* ========== 显示菜单 ========== */
void Display_Menu(void)
{
    LCD_Clear_Fast(BLACK);
    LCD_ShowString(0, 0, "=== SETTINGS ===");
    
    sprintf(display_buf, "  1.Offset: %d", y_offset);
    LCD_ShowString(0, 30, display_buf);
    
    sprintf(display_buf, "  2.Scale: %.1f", y_scale);
    LCD_ShowString(0, 50, display_buf);
    
    LCD_ShowString(0, 70, "  3.Trigger: Auto");
    
    LCD_ShowString(0, 110, "UP/DOWN:Change");
    LCD_ShowString(0, 130, "LEFT:Back RIGHT:Select");
    
    switch(menu_index)
    {
        case 0: LCD_ShowString(0, 30, "->"); break;
        case 1: LCD_ShowString(0, 50, "->"); break;
        case 2: LCD_ShowString(0, 70, "->"); break;
    }
}

/* ========== 重置显示缓存 ========== */
void Reset_Display_Cache(void)
{
    last_vpp = -1;
    last_max = -1;
    last_min = -1;
    last_scl = -1;
    last_y_offset = y_offset + 1;
    last_pause_flag = !pause_flag;
    last_scale_top = -1;
    last_scale_center = -1;
    last_scale_bottom = -1;
}

/* ========== 主函数 ========== */
int main(void)
{
    Key_Type key;
    
    delay_init();
    Lcd_Init();
    KEY_Init();
    ADC1_DMA_Init();
    
    Draw_Grid();
    last_y_offset = y_offset;
    
    LCD_ShowString(0, 0, "Oscilloscope Ready");
    LCD_ShowString(0, 20, "LEFT:Menu RIGHT:Pause");
    delay_ms(2000);
    LCD_Fill_Fast(0, 0, LCD_W - 1, 35, BLACK);
    
    while(1)
    {
        while(!DMA_GetFlagStatus(DMA1_FLAG_TC1));
        DMA_ClearFlag(DMA1_FLAG_TC1);
        
        if(!pause_flag)
        {
            memcpy(Paused_Buffer, ADC_Buffer, sizeof(ADC_Buffer));
        }
        
        key = KEY_Scan();
        
        if(show_menu)
        {
            uint8_t exit_menu = 0;
            
            switch(key)
            {
                case KEY_UP:
                    if(menu_index == 0) y_offset += 2;
                    if(menu_index == 1) y_scale += 0.1f;
                    break;
                case KEY_DOWN:
                    if(menu_index == 0) y_offset -= 2;
                    if(menu_index == 1) y_scale -= 0.1f;
                    break;
                case KEY_LEFT:
                    show_menu = 0;
                    exit_menu = 1;
                    LCD_Clear_Fast(BLACK);
                    Draw_Grid();
                    Reset_Display_Cache();
                    Clear_Waveform_Area();
                    Draw_Waveform_Fast(ADC_Buffer);
                    Display_Info_Fast();
                    last_y_offset = y_offset;
                    break;
                case KEY_RIGHT:
                    menu_index++;
                    if(menu_index > 2) menu_index = 0;
                    break;
                default:
                    break;
            }
            
            if(y_offset > 40) y_offset = 40;
            if(y_offset < -40) y_offset = -40;
            if(y_scale > 10.0f) y_scale = 10.0f;
            if(y_scale < 0.5f) y_scale = 0.5f;
            
            if(!exit_menu)
            {
                Display_Menu();
                delay_ms(100);
            }
        }
        else
        {
            switch(key)
            {
                case KEY_UP:
                    if(y_scale < 10.0f) y_scale += 0.1f;
                    Draw_Grid();
                    break;
                case KEY_DOWN:
                    if(y_scale > 0.5f) y_scale -= 0.1f;
                    Draw_Grid();
                    break;
                case KEY_LEFT:
                    show_menu = 1;
                    menu_index = 0;
                    break;
                case KEY_RIGHT:
                    pause_flag = !pause_flag;
                    break;
                default:
                    break;
            }
            
            if(!pause_flag)
            {
                Clear_Waveform_Area();
                Draw_Waveform_Fast(ADC_Buffer);
				Draw_Grid();
                Display_Info_Fast();
            }
            else
            {
                Display_Info_Fast();
            }
        }
        
        delay_ms(5);
    }
}
