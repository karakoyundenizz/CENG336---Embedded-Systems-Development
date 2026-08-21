#ifndef ADC_H
#define ADC_H

#include <stdint.h>


typedef enum {
    THERMAL_NORMAL,   // < 700       (Cap: 24A) 
    THERMAL_DERATED,  // 700 - 899   (Cap: 08A) 
    THERMAL_OVERHEAT  // >= 900      (Cap: 00A) 
} ThermalBand;


void adc_init(void);

void adc_start_conversion(void);

void adc_isr_handle(void);

uint16_t adc_get_raw_value(void);

char adc_get_mode_char(void);

uint8_t adc_get_thermal_cap(void);

#endif /* ADC_H */