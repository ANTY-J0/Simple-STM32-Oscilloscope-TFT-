#ifndef __ADC_H
#define __ADC_H

#include "stm32f10x.h"

#define ADC_BUFFER_SIZE   128
#define ADC_MAX_VALUE     4095
#define VREF              3.3f

extern uint16_t ADC_Buffer[ADC_BUFFER_SIZE];
extern volatile uint8_t ADC_ConversionComplete;

void ADC1_DMA_Init(void);
float ADC_GetVoltage(uint16_t adc_val);

#endif