#ifndef SPISTDIO_H
#define SPISTDIO_H

#include "stm32f0xx.h"
#include "stm32f0xx_ll_spi.h"
#include "stm32f0xx_ll_gpio.h"

typedef enum
{
    SPI_IDLE_STATE,
    SPI_WRITE_STATE,
    SPI_READ_STATE, 
} SPI_state_t;

typedef enum
{
    SPI_WRITE,  // DO NOT MOVE OR CHANGE POSITION
    SPI_READ,   // DO NOT MOVE OR CHANGE POSITION
    SPI_READ_TO_TEMP,
    SPI_WRITE_MODIFY,
} SPI_command_t;

typedef enum
{
    SPI_HEADER = 0,
    SPI_DATA,
    SPI_ENDER,
} SPI_data_t;

typedef struct
{
    uint8_t mask;
    uint8_t data;
} SPI_TX_data_t;

typedef struct
{
    uint8_t        addr;
    SPI_TX_data_t* p_data_array;

    uint8_t        data_size;
} SPI_frame_t;

typedef struct
{
    SPI_command_t   command;
    SPI_data_t      data_type;

    uint8_t         mask;
    uint8_t         data;

} SPI_TX_buffer_t;

typedef struct _spi_stdio_typedef
{
    SPI_TypeDef*            handle;
    IRQn_Type		        irqn;

    uint8_t*                p_temp_RX_buffer;

    volatile uint16_t       temp_RX_index;
             uint16_t       temp_RX_size;
             uint8_t        temp_RX_irqn_byte;

    SPI_TX_buffer_t*        p_TX_buffer;

    volatile uint16_t       TX_write_index;
    volatile uint16_t       TX_read_index;
             uint16_t       TX_size;

    uint8_t*                p_RX_buffer;

    volatile uint16_t       RX_write_index;
    volatile uint16_t       RX_read_index;
             uint16_t       RX_size;

    GPIO_TypeDef*           cs_port;
    uint32_t                cs_pin;
} spi_stdio_typedef;

void        SPI_Init( spi_stdio_typedef* p_spi, SPI_TypeDef* _handle,
                IRQn_Type _irqn, uint8_t* _p_temp_RX_buffer,
                uint16_t _temp_RX_size, SPI_TX_buffer_t* _p_TX_buffer,
                uint16_t _TX_size, uint8_t* _p_RX_buffer,
                uint16_t _RX_size, GPIO_TypeDef* _cs_port, uint32_t _cs_pin);

void        SPI_Write(spi_stdio_typedef* p_spi, SPI_frame_t* p_frame);
void        SPI_Read(spi_stdio_typedef* p_spi, SPI_frame_t* p_frame);
void        SPI_Overwrite(spi_stdio_typedef* p_spi, SPI_frame_t* p_frame);

uint8_t     SPI_is_buffer_full(volatile uint16_t *pui16Read,
                volatile uint16_t *pui16Write, uint16_t ui16Size);

uint8_t     SPI_is_buffer_empty(volatile uint16_t *pui16Read,
                volatile uint16_t *pui16Write);

uint16_t    SPI_get_buffer_count(volatile uint16_t *pui16Read,
                volatile uint16_t *pui16Write, uint16_t ui16Size);

uint16_t    SPI_advance_buffer_index(volatile uint16_t* pui16Index, uint16_t ui16Size);

uint16_t    SPI_get_next_buffer_index(uint16_t ui16Index, uint16_t addition, uint16_t ui16Size);

// void        SPI_flush_temp_to_RX_buffer(spi_stdio_typedef* p_spi);

void        SPI_Prime_Transmit(spi_stdio_typedef* p_spi);

//*****************************************************************************
//
// Macros to determine number of free and used bytes in the transmit buffer.
//
//*****************************************************************************
#define SPI_TX_BUFFER_SIZE(p_spi)          ((p_spi)->TX_size)

#define SPI_TX_BUFFER_USED(p_spi)          (SPI_get_buffer_count(&(p_spi)->TX_read_index,  \
                                                        &(p_spi)->TX_write_index, \
                                                        (p_spi)->TX_size))

#define SPI_TX_BUFFER_FREE(p_spi)          (SPI_TX_BUFFER_SIZE(p_spi) - SPI_TX_BUFFER_USED(p_spi))

#define SPI_TX_BUFFER_EMPTY(p_spi)         (SPI_is_buffer_empty(&(p_spi)->TX_read_index,   \
                                                       &(p_spi)->TX_write_index))

#define SPI_TX_BUFFER_FULL(p_spi)          (SPI_is_buffer_full(&(p_spi)->TX_read_index,  \
                                                      &(p_spi)->TX_write_index, \
                                                      (p_spi)->TX_size))

#define SPI_ADVANCE_TX_WRITE_INDEX(p_spi)  (SPI_advance_buffer_index(&(p_spi)->TX_write_index, \
                                                            (p_spi)->TX_size))

#define SPI_ADVANCE_TX_READ_INDEX(p_spi)   (SPI_advance_buffer_index(&(p_spi)->TX_read_index, \
                                                            (p_spi)->TX_size))

//*****************************************************************************
//
// Macros to determine number of free and used bytes in the receive buffer.
//
//*****************************************************************************
#define SPI_RX_BUFFER_SIZE(p_spi)          ((p_spi)->RX_size)

#define SPI_RX_BUFFER_USED(p_spi)          (SPI_get_buffer_count(&(p_spi)->RX_read_index,  \
                                                        &(p_spi)->RX_write_index, \
                                                        (p_spi)->RX_size))

#define SPI_RX_BUFFER_FREE(p_spi)          (SPI_RX_BUFFER_SIZE(p_spi) - SPI_RX_BUFFER_USED(p_spi))

#define SPI_RX_BUFFER_EMPTY(p_spi)         (SPI_is_buffer_empty(&(p_spi)->RX_read_index,   \
                                                       &(p_spi)->RX_write_index))

#define SPI_RX_BUFFER_FULL(p_spi)          (SPI_is_buffer_full(&(p_spi)->RX_read_index,  \
                                                      &(p_spi)->RX_write_index, \
                                                      (p_spi)->RX_size))

#define SPI_ADVANCE_RX_WRITE_INDEX(p_spi)  (SPI_advance_buffer_index(&(p_spi)->RX_write_index, \
                                                            (p_spi)->RX_size))

#define SPI_ADVANCE_RX_READ_INDEX(p_spi)   (SPI_advance_buffer_index(&(p_spi)->RX_read_index, \
                                                            (p_spi)->RX_size))

#endif // SPISTDIO_H
