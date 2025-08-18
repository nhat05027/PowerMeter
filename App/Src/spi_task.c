/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Private Include~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#include "app.h"
#include "spi_task.h"
#include "calculate_task.h"
#include <stdio.h>
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Private Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#define SPI_BUFFER_SIZE 64
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Private Prototype ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Public Variables ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
uint16_t spi_task_cnt = 0;
spi_stdio_typedef ESP_SPI;

SPI_TX_buffer_t g_SPI_TX_buffer[256];
uint8_t g_SPI_RX_buffer[256];
uint8_t g_temp_SPI_RX_buffer[6];
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Private Enum ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Private Struct ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
typedef struct {
    float rms_voltage[3];     // V1, V2, V3 (voltage RMS)
    float rms_current[3];     // I1, I2, I3 (current RMS)
    float active_power[3];    // P1, P2, P3 (active power)
    float apparent_power[3];  // Q1, Q2, Q3 (reactive power)
    float frequency;          // Frequency
} EnergyData_t;  // Total: 13 floats = 52 bytes

// Extended data structure with THD (for future use)
typedef struct {
    EnergyData_t basic_data;  // 13 floats = 52 bytes
    float thd_voltage[3];     // THD for 3 phase voltages (%)
    float fundamental_voltage[3]; // Fundamental voltage component (V)
    // Note: This structure is larger than current SPI frame format
} ExtendedEnergyData_t;
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Private Class ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Private Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Private Variables ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
static EnergyData_t get_real_energy_data() {
    EnergyData_t data;
    
    // Copy voltage RMS values (channels 3, 4, 5)
    data.rms_voltage[0] = g_RMS_Value[3];  // L1 voltage
    data.rms_voltage[1] = g_RMS_Value[4];  // L2 voltage
    data.rms_voltage[2] = g_RMS_Value[5];  // L3 voltage
    
    // Copy current RMS values (channels 0, 1, 2)  
    data.rms_current[0] = g_RMS_Value[0];  // L1 current
    data.rms_current[1] = g_RMS_Value[1];  // L2 current
    data.rms_current[2] = g_RMS_Value[2];  // L3 current
    
    // Copy power values (3 phases)
    for(int i = 0; i < 3; i++) {
        data.active_power[i] = g_Active_Power[i];
        data.apparent_power[i] = g_Apparent_Power[i];
    }
    
    // Copy frequency
    data.frequency = g_Signal_Frequency;
    
    return data;
}

// Function to get extended energy data including THD (for future use)
static ExtendedEnergyData_t get_extended_energy_data() {
    ExtendedEnergyData_t extended_data;
    
    // Get basic energy data
    extended_data.basic_data = get_real_energy_data();
    
    // Get THD data
    for(int i = 0; i < 3; i++) {
        extended_data.thd_voltage[i] = THD_Get_Voltage_THD(i);
        extended_data.fundamental_voltage[i] = g_Fundamental_Voltage[i];
    }
    
    return extended_data;
}



void spi_task_Init()
{
    SPI_Init(&ESP_SPI, SPI_HANDLE, SPI_IRQ,
            g_temp_SPI_RX_buffer, 6,
			g_SPI_TX_buffer, 256,
			g_SPI_RX_buffer, 256,
            SPI_CS_PORT, SPI_CS_PIN);
}



void real_data_Task(void *pvParameters)
{
    // Tạo 1 frame chứa 52 bytes (13 floats)
    SPI_TX_data_t s_SPI_data_array[52];  // 13 floats x 4 bytes = 52 bytes
    SPI_frame_t   s_SPI_frame;

    EnergyData_t data = get_real_energy_data();
    uint8_t* float_bytes = (uint8_t*)&data;

    // Copy tất cả 52 bytes vào data array
    for (uint8_t i = 0; i < 52; i++) {
        s_SPI_data_array[i].data = float_bytes[i];
        s_SPI_data_array[i].mask = 0xFF;
    }

    // Tạo 1 frame duy nhất với address 0x00 và size 52
    s_SPI_frame.addr = 0x00;              // Start address
    s_SPI_frame.data_size = 52;           // 13 floats x 4 bytes
    s_SPI_frame.p_data_array = s_SPI_data_array;

    // Gửi 1 frame chứa 52 bytes data thật
    SPI_Overwrite(&ESP_SPI, &s_SPI_frame);
    
    spi_task_cnt++;
}



void SPI_IRQHandler(void)
{

    // Xử lý chỉ khi có data
    if (LL_SPI_IsActiveFlag_RXNE(SPI_HANDLE))
    {
        // Cache index & pointer cho nhanh, hạn chế truy RAM nhiều lần
        SPI_TX_buffer_t *p_tx = &ESP_SPI.p_TX_buffer[ESP_SPI.TX_read_index];
        uint8_t rx_temp       = LL_SPI_ReceiveData8(SPI_HANDLE);

        if (p_tx->data_type == SPI_HEADER) 
        {
            rx_temp = p_tx->data;
        }

        // Nếu là READ, chuyển thẳng sang RX_buffer
        if (p_tx->command == SPI_READ)
        {
            ESP_SPI.p_RX_buffer[ESP_SPI.RX_write_index] = rx_temp;
            SPI_ADVANCE_RX_WRITE_INDEX(&ESP_SPI);
        }
        // Nếu là READ_TO_TEMP, chỉ advance index tạm
        else if (p_tx->command == SPI_READ_TO_TEMP)
        {
            ESP_SPI.p_temp_RX_buffer[ESP_SPI.temp_RX_index] = rx_temp;
            ESP_SPI.temp_RX_index++;
        }

        // Còn lại (WRITE, WRITE_MODIFY) KHÔNG LÀM GÌ - vì không có nhận data

        // Nếu kết thúc 1 frame
        if (p_tx->data_type == SPI_ENDER)
        {
            LL_GPIO_SetOutputPin(SPI_CS_PORT, SPI_CS_PIN);
        }

        // Advance TX index sang byte tiếp theo
        SPI_ADVANCE_TX_READ_INDEX(&ESP_SPI);

        // Nếu hết buffer truyền, tắt ngắt
        if (SPI_TX_BUFFER_EMPTY(&ESP_SPI))
        {
            LL_SPI_DisableIT_RXNE(SPI_HANDLE);
            return;
        }

        // Tiếp tục gửi byte tiếp theo
        SPI_Prime_Transmit(&ESP_SPI);
    }
}
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ End of the program ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */