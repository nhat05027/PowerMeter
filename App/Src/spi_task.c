/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Private Include~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#include "app.h"
#include "spi_task.h"
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
    float voltage_a;
    float current_a;
    float active_power_a;
    float apparent_power_a;

    float voltage_b;
    float current_b;
    float active_power_b;
    float apparent_power_b;

    float voltage_c;
    float current_c;
    float active_power_c;
    float apparent_power_c;

    float frequency;
} EnergyData_t;
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Private Class ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Private Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Private Variables ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
static EnergyData_t random_energy_data() {
    EnergyData_t data;
    data.voltage_a = 220.0f;
    data.current_a = 5.0f;
    data.active_power_a = 1000.0f;
    data.apparent_power_a = 1100.0f;

    data.voltage_b = 221.0f;
    data.current_b = 5.1f;
    data.active_power_b = 1005.0f;
    data.apparent_power_b = 1110.0f;

    data.voltage_c = 222.0f;
    data.current_c = 5.2f;
    data.active_power_c = 1010.0f;
    data.apparent_power_c = 1120.0f;

    data.frequency = 50.0f;
    return data;
}



void spi_task_Init()
{
    SPI_Init(&ESP_SPI, SPI_HANDLE, SPI_IRQ,
            g_temp_SPI_RX_buffer, 6,
			g_SPI_TX_buffer, 256,
			g_SPI_RX_buffer, 256,
            SPI_CS_PORT, SPI_CS_PIN);
}



void random_data_Task(void*)
{
    SPI_TX_data_t s_SPI_data_array[4];
    SPI_frame_t   s_SPI_frame;

    EnergyData_t data = random_energy_data();
    uint8_t* float_bytes = (uint8_t*)&data;

    for (uint8_t field_idx = 0; field_idx < 13; field_idx++)
    {
        // Mỗi float chiếm 4 byte
        uint8_t* p = &float_bytes[field_idx * sizeof(float)];

        // Gắn từng byte vào mảng data
        for (uint8_t i = 0; i < 4; i++) {
            s_SPI_data_array[i].data = p[i];
            s_SPI_data_array[i].mask = 0xFF;
        }

        s_SPI_frame.addr = field_idx;           // Địa chỉ 0x00 đến 0x0C
        s_SPI_frame.data_size = 4;              // 4 byte float
        s_SPI_frame.p_data_array = s_SPI_data_array;

        // Gửi từng frame
        SPI_Overwrite(&ESP_SPI, &s_SPI_frame);
    }
    spi_task_cnt ++;

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
        // if (p_tx->data_type == SPI_ENDER)
        // {
        //     LL_GPIO_SetOutputPin(SPI_CS_PORT, SPI_CS_PIN);
        // }

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