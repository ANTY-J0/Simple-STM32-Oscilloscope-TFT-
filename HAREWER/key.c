#include "key.h"
#include "delay.h"

// 按键引脚定义: PA1, PA2, PA3, PA4
#define KEY_UP_PIN      GPIO_Pin_1
#define KEY_DOWN_PIN    GPIO_Pin_2
#define KEY_LEFT_PIN    GPIO_Pin_3
#define KEY_RIGHT_PIN   GPIO_Pin_4
#define KEY_PORT        GPIOA

void KEY_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    
    GPIO_InitStructure.GPIO_Pin = KEY_UP_PIN | KEY_DOWN_PIN | KEY_LEFT_PIN | KEY_RIGHT_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(KEY_PORT, &GPIO_InitStructure);
}

Key_Type KEY_Scan(void)
{
    static uint8_t key_lock = 0;
    
    if(GPIO_ReadInputDataBit(KEY_PORT, KEY_UP_PIN) == 0 && !key_lock) {
        delay_ms(20);
        if(GPIO_ReadInputDataBit(KEY_PORT, KEY_UP_PIN) == 0) {
            key_lock = 1;
            return KEY_UP;
        }
    }
    else if(GPIO_ReadInputDataBit(KEY_PORT, KEY_DOWN_PIN) == 0 && !key_lock) {
        delay_ms(20);
        if(GPIO_ReadInputDataBit(KEY_PORT, KEY_DOWN_PIN) == 0) {
            key_lock = 1;
            return KEY_DOWN;
        }
    }
    else if(GPIO_ReadInputDataBit(KEY_PORT, KEY_LEFT_PIN) == 0 && !key_lock) {
        delay_ms(20);
        if(GPIO_ReadInputDataBit(KEY_PORT, KEY_LEFT_PIN) == 0) {
            key_lock = 1;
            return KEY_LEFT;
        }
    }
    else if(GPIO_ReadInputDataBit(KEY_PORT, KEY_RIGHT_PIN) == 0 && !key_lock) {
        delay_ms(20);
        if(GPIO_ReadInputDataBit(KEY_PORT, KEY_RIGHT_PIN) == 0) {
            key_lock = 1;
            return KEY_RIGHT;
        }
    }
    else {
        key_lock = 0;
    }
    
    return KEY_NONE;
}
