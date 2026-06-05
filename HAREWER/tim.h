#ifndef __TIM_H
#define __TIM_H

#include "stm32f10x.h"

void TIM2_PWM_Init(uint16_t arr, uint16_t psc);
void TIM2_SetSamplingRate(uint32_t samples_per_sec);

#endif
