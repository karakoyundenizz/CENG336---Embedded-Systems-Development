#include <xc.h>
#include "adc.h"

static volatile uint16_t raw_adc_value = 0;
static volatile ThermalBand current_band = THERMAL_NORMAL;


void adc_init(void)
{
    TRISHbits.TRISH4 = 1; // RH4 setted as input 
    ADCON0 = 0x31; // channel 12 selected , adc enabled but not started
    ADCON1 = 0x02; // pcfg set as 0010 making A12 analog,  no external voltage reference values
    ADCON2 = 0xbe; // right justified, acquisition and conversion time setted as max values not to get garbage values. CHECK THIS
    PIR1bits.ADIF = 0; // reset the flag before enabling interrupts to avoid random triggers
    PIE1bits.ADIE = 1; // ADC interrupt enabled
}

void adc_start_conversion(void)
{
    ADCON0bits.GO = 1;
}



void adc_isr_handle(void)
{
    raw_adc_value = ((uint16_t)ADRESH << 8) | ADRESL;

    if (raw_adc_value < 700) {
        current_band = THERMAL_NORMAL;
    } 
    else if (raw_adc_value >= 700 && raw_adc_value < 900) {
        current_band = THERMAL_DERATED;
    } 
    else {
        current_band = THERMAL_OVERHEAT;
    }

    PIR1bits.ADIF = 0; 
}



uint16_t adc_get_raw_value(void)
{
    return raw_adc_value; // for 7 segment display
}

char adc_get_mode_char(void)
{
    // getter for STM 
    switch (current_band) {
        case THERMAL_NORMAL:   return 'N';
        case THERMAL_DERATED:  return 'D';
        case THERMAL_OVERHEAT: return 'H';
    }
}

uint8_t adc_get_thermal_cap(void)
{
    // to be used for calculating efective limit
    switch (current_band) {
        case THERMAL_NORMAL:   return 24;
        case THERMAL_DERATED:  return 8;
        case THERMAL_OVERHEAT: return 0;
    }
}