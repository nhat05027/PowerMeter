/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Private Include~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#include "adc.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Private Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Private Prototype ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Private Variables ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Private Enum ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Private Struct ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Private Class ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Private Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Private Variables ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Private Prototype ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Public Variables ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Public Function ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* :::::::::: ADC Init ::::::::::::: */
void ADC_Init(ADC_TypeDef* ADCx, uint32_t Sampling_Time)
{
    // Set ADC sampling time
    LL_ADC_SetSamplingTimeCommonChannels(ADCx, Sampling_Time);

    // Set ADC sequence of channel, 1 channel is sellected
    //LL_ADC_REG_SetSequencerChannels(ADCx, LL_ADC_CHANNEL_0);

    // Calib the ADC using self calib
    ADC_Self_Calib(ADCx);

    // Enable the ADC
    ADC_Enable(ADC1);
}

/* :::::::::: ADC Self Calib ::::::::::::: */
void ADC_Self_Calib(ADC_TypeDef* ADCx)
{
    /* (1) Ensure that ADEN = 0 */
    /* (2) Clear ADEN by setting ADDIS*/
    /* (3) Clear DMAEN */
    /* (4) Launch the calibration by setting ADCAL */
    /* (5) Wait until ADCAL=0 */

    if ((ADCx->CR & ADC_CR_ADEN) != 0) /* (1) */
    {
        ADCx->CR |= ADC_CR_ADDIS; /* (2) */
    }

    while ((ADCx->CR & ADC_CR_ADEN) != 0)
    {
        /* For robust implementation, add here time-out management */
    }

    ADCx->CFGR1 &= ~ADC_CFGR1_DMAEN; /* (3) */
    ADCx->CR |= ADC_CR_ADCAL; /* (4) */

    while ((ADCx->CR & ADC_CR_ADCAL) != 0) /* (5) */
    {
        /* For robust implementation, add here time-out management */
    }
}

/* :::::::::: ADC Enable ::::::::::::: */
void ADC_Enable(ADC_TypeDef* ADCx)
{
    /* (1) Ensure that ADRDY = 0 */
    /* (2) Clear ADRDY */
    /* (3) Enable the ADC */
    /* (4) Wait until ADC ready */

    if ((ADCx->ISR & ADC_ISR_ADRDY) != 0) /* (1) */
    {
        ADCx->ISR |= ADC_ISR_ADRDY; /* (2) */
    }

    ADCx->CR |= ADC_CR_ADEN; /* (3) */

    while ((ADCx->ISR & ADC_ISR_ADRDY) == 0) /* (4) */
    {
        /* For robust implementation, add here time-out management */
    }
}

/* :::::::::: ADC Disable ::::::::::::: */
void ADC_Disable(ADC_TypeDef* ADCx)
{
    /* (1) Stop any ongoing conversion */
    /* (2) Wait until ADSTP is reset by hardware i.e. conversion is stopped */
    /* (3) Disable the ADC */
    /* (4) Wait until the ADC is fully disabled */

    ADCx->CR |= ADC_CR_ADSTP; /* (1) */

    while ((ADCx->CR & ADC_CR_ADSTP) != 0) /* (2) */
    {
        /* For robust implementation, add here time-out management */
    }

    ADCx->CR |= ADC_CR_ADDIS; /* (3) */
    
    while ((ADCx->CR & ADC_CR_ADEN) != 0) /* (4) */
    {
        /* For robust implementation, add here time-out management */
    }
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Private Prototype ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ End of the program ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */