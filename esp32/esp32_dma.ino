#include <Arduino.h>
#include <ESP32DMASPISlave.h>

// ==== DMA SPI ====
ESP32DMASPI::Slave slave;

static constexpr size_t BUFFER_SIZE = 256; // should be multiple of 4
static constexpr size_t QUEUE_SIZE = 1;    // single queue cho đơn giản
uint8_t *dma_rx_buf;

#define FRAME_SIZE 55

typedef struct {
    float voltage_a, current_a, active_power_a, apparent_power_a;
    float voltage_b, current_b, active_power_b, apparent_power_b;
    float voltage_c, current_c, active_power_c, apparent_power_c;
    float frequency;
} PowerMeterData_t;

PowerMeterData_t power_data = {0};
unsigned long last_summary = 0;
int frames_decoded = 0;

// ==== Frame detection variables ====
static bool has_last_hash = false;
static uint32_t last_hash = 0;

// ======== forward decl ========
void processReceivedData(uint8_t* data, size_t len);
void printRawData(uint8_t* data, size_t len);
bool decodePowerMeterFrame(uint8_t* data, size_t len);
void printPowerMeterSummary();
float extractFloat(uint8_t* data, size_t offset);

// ==== FNV-1a 32-bit hash ====
static inline uint32_t fnv1a32(const uint8_t* data, size_t len) {
    uint32_t hash = 0x811C9DC5u;
    const uint32_t prime = 16777619u;
    for (size_t i = 0; i < len; i++) { 
        hash ^= data[i]; 
        hash *= prime; 
    }
    return hash;
}

void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println("Starting ESP32 Power Meter with DMA SPI...");

    // Allocate DMA buffer
    dma_rx_buf = slave.allocDMABuffer(BUFFER_SIZE);
    if (!dma_rx_buf) {
        Serial.println("[ERR] Failed to allocate DMA buffer");
        while (1) delay(1000);
    }

    // Configure DMA SPI
    slave.setDataMode(SPI_MODE0);
    slave.setMaxTransferSize(BUFFER_SIZE);
    slave.setQueueSize(QUEUE_SIZE);

    // Begin SPI (default HSPI: SCK=14, MISO=12, MOSI=13, SS=15)
    if (!slave.begin()) {
        Serial.println("[ERR] SPI slave begin failed");
        while (1) delay(1000);
    }

    Serial.println("=========================================");
    Serial.println("ESP32 Power Meter (DMA SPI Slave)");
    Serial.println("Waiting for STM32 data...");
    Serial.println("=========================================\n");

    last_summary = millis();
}

void loop() {
    // Check if we can queue a new transaction
    if (slave.hasTransactionsCompletedAndAllResultsHandled()) {
        // Initialize rx buffer
        memset(dma_rx_buf, 0x00, BUFFER_SIZE);
        
        // Queue receive transaction
        slave.queue(nullptr, dma_rx_buf, BUFFER_SIZE);
        
        // Trigger transaction
        slave.trigger();
    }

    // Check if transaction is completed and results are ready
    if (slave.hasTransactionsCompletedAndAllResultsReady(QUEUE_SIZE)) {
        // Get received bytes
        const std::vector<size_t> received_bytes = slave.numBytesReceivedAll();
        size_t n = received_bytes[0];

        if (n > 0) {
            // Look for sync pattern and decode
            processReceivedData(dma_rx_buf, n);
        }
    }

    // Print summary every 5 seconds
    // if (millis() - last_summary > 5000) {
    //     printPowerMeterSummary();
    //     last_summary = millis();
    // }

    delay(10); // Small delay for stability
}

void processReceivedData(uint8_t* data, size_t len) {
    // Find sync pattern AA 55
    int sync_pos = -1;
    for (int i = 0; i <= (int)len - FRAME_SIZE; i++) {
        if (data[i] == 0xAA && data[i+1] == 0x55) { 
            sync_pos = i; 
            break; 
        }
    }

    if (sync_pos >= 0 && sync_pos + FRAME_SIZE <= (int)len) {
        // Hash payload to detect new frames (skip AA 55 sync)
        const uint8_t* payload = &data[sync_pos + 2];
        uint32_t h = fnv1a32(payload, 53); // 1 header + 13*4 floats = 53 bytes
        bool is_new = (!has_last_hash) || (h != last_hash);

        if (is_new) {
            Serial.printf("\n[SPI] NEW frame %u bytes at %lu ms (sync@%d)\n",
                         (unsigned)len, millis(), sync_pos);
            
            if (decodePowerMeterFrame(data, len)) {
                frames_decoded++;
                last_hash = h;
                has_last_hash = true;
            }
        }
    }
}

void printRawData(uint8_t* data, size_t len) {
    Serial.print("[RAW] ");
    for (size_t i = 0; i < len && i < 64; i++) {
        Serial.printf("%02X ", data[i]);
        if ((i + 1) % 16 == 0) Serial.println("\n      ");
    }
    if (len % 16 != 0) Serial.println();
}

float extractFloat(uint8_t* data, size_t offset) {
    // Byte order reversed [b3 b2 b1 b0]
    uint8_t bytes[4];
    bytes[0] = data[offset + 3];
    bytes[1] = data[offset + 2];
    bytes[2] = data[offset + 1];
    bytes[3] = data[offset + 0];

    float value;
    memcpy(&value, bytes, 4);

    if (!isfinite(value) || fabsf(value) > 100000.0f) return 0.0f;
    return value;
}

bool decodePowerMeterFrame(uint8_t* data, size_t len) {
    // Find sync
    int sync_pos = -1;
    for (int i = 0; i <= (int)len - FRAME_SIZE; i++) {
        if (data[i] == 0xAA && data[i+1] == 0x55) { 
            sync_pos = i; 
            break; 
        }
    }
    if (sync_pos < 0 || sync_pos + FRAME_SIZE > (int)len) return false;

    // Parse 13 floats after header
    float v[13];
    for (int i = 0; i < 13; i++) {
        size_t byte_offset = sync_pos + 3 + (i * 4);
        v[i] = extractFloat(data, byte_offset);
    }

    // Field mapping:
    // 0: Freq
    // 1: S_C, 2: P_C, 3: I_C, 4: V_C
    // 5: S_B, 6: P_B, 7: I_B, 8: V_B
    // 9: S_A, 10: P_A, 11: I_A, 12: V_A
    power_data.frequency         = v[0];

    power_data.apparent_power_c  = v[1];
    power_data.active_power_c    = v[4];
    power_data.current_c         = v[7];
    power_data.voltage_c         = v[10];

    power_data.apparent_power_b  = v[2];
    power_data.active_power_b    = v[5];
    power_data.current_b         = v[8];
    power_data.voltage_b         = v[11];

    power_data.apparent_power_a  = v[3];
    power_data.active_power_a    = v[6];
    power_data.current_a         = v[9];
    power_data.voltage_a         = v[12];

    Serial.println("\n[SUCCESS] ✓ New frame decoded");
    Serial.printf("  A: V=%.1fV  I=%.2fA  P=%.1fW  S=%.1fVA\n",
                  power_data.voltage_a, power_data.current_a,
                  power_data.active_power_a, power_data.apparent_power_a);
    Serial.printf("  B: V=%.1fV  I=%.2fA  P=%.1fW  S=%.1fVA\n",
                  power_data.voltage_b, power_data.current_b,
                  power_data.active_power_b, power_data.apparent_power_b);
    Serial.printf("  C: V=%.1fV  I=%.2fA  P=%.1fW  S=%.1fVA\n",
                  power_data.voltage_c, power_data.current_c,
                  power_data.active_power_c, power_data.apparent_power_c);
    Serial.printf("  f = %.2f Hz\n", power_data.frequency);
    return true;
}

void printPowerMeterSummary() {
    if (frames_decoded == 0) {
        Serial.println("\n[INFO] No frames decoded yet. Waiting for STM32...");
        return;
    }

    Serial.println("\n=============== POWER METER SUMMARY ===============");
    Serial.println("PHASE A:");
    Serial.printf("  Voltage:     %8.1f V\n", power_data.voltage_a);
    Serial.printf("  Current:     %8.2f A\n", power_data.current_a);
    Serial.printf("  Active Pwr:  %8.1f W\n", power_data.active_power_a);
    Serial.printf("  Apparent Pwr:%8.1f VA\n", power_data.apparent_power_a);

    Serial.println("PHASE B:");
    Serial.printf("  Voltage:     %8.1f V\n", power_data.voltage_b);
    Serial.printf("  Current:     %8.2f A\n", power_data.current_b);
    Serial.printf("  Active Pwr:  %8.1f W\n", power_data.active_power_b);
    Serial.printf("  Apparent Pwr:%8.1f VA\n", power_data.apparent_power_b);

    Serial.println("PHASE C:");
    Serial.printf("  Voltage:     %8.1f V\n", power_data.voltage_c);
    Serial.printf("  Current:     %8.2f A\n", power_data.current_c);
    Serial.printf("  Active Pwr:  %8.1f W\n", power_data.active_power_c);
    Serial.printf("  Apparent Pwr:%8.1f VA\n", power_data.apparent_power_c);

    Serial.printf("FREQUENCY:     %8.2f Hz\n", power_data.frequency);

    float total_active = power_data.active_power_a + power_data.active_power_b + power_data.active_power_c;
    float total_apparent = power_data.apparent_power_a + power_data.apparent_power_b + power_data.apparent_power_c;

    Serial.printf("\nSYSTEM TOTALS:\n");
    Serial.printf("  Total Active: %7.1f W\n", total_active);
    Serial.printf("  Total Apparent:%6.1f VA\n", total_apparent);

    Serial.printf("\nFrames: %d\n", frames_decoded);
    Serial.println("==================================================\n");
}
