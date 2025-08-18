#include <Arduino.h>
#include <ESP32SPISlave.h>
#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>

//==================== SPI SLAVE ====================
ESP32SPISlave spi;

static constexpr uint8_t SPI_BUS = HSPI;
static constexpr int PIN_SCLK = 14;
static constexpr int PIN_MISO = 12;
static constexpr int PIN_MOSI = 13;
static constexpr int PIN_SS   = 15;

#define RX_SIZE 64
DMA_ATTR static uint8_t rx_buf[RX_SIZE];

//==================== POWER DATA ====================
struct PowerData {
    float voltage[3];
    float current[3]; 
    float active_power[3];
    float reactive_power[3];
    float frequency;
};

PowerData power = {0};
unsigned long last_print = 0;
int frame_count = 0;

//==================== WIFI CONFIG =====================
char wifi_ssid[32] = "";
char wifi_pass[32] = "";
const char* ap_ssid = "ESP32-PowerMeter";
const char* ap_pass = "12345678";

WebServer server(80);

//==================== FUNCTIONS ====================
float extractFloat(uint8_t* data, int offset) {
    if (!data) return 0.0f;
    
    union {
        uint8_t bytes[4];
        float value;
    } converter;
    
    // Reverse byte order
    converter.bytes[0] = data[offset + 3];
    converter.bytes[1] = data[offset + 2]; 
    converter.bytes[2] = data[offset + 1];
    converter.bytes[3] = data[offset + 0];
    
    if (!isfinite(converter.value) || fabs(converter.value) > 1000000.0f) {
        return 0.0f;
    }
    
    return converter.value;
}

bool decodeFrame(uint8_t* data, size_t len) {
    if (!data || len < 55) return false;
    
    // Find sync pattern
    int sync_pos = -1;
    for (int i = 0; i <= (int)len - 55; i++) {
        if (data[i] == 0xAA && data[i+1] == 0x55) {
            sync_pos = i;
            break;
        }
    }
    
    if (sync_pos == -1) return false;
    
    // Extract 13 floats safely
    float values[13];
    for (int i = 0; i < 13; i++) {
        int offset = sync_pos + 3 + (i * 4);
        if (offset + 3 < (int)len) {
            values[i] = extractFloat(data, offset);
        } else {
            values[i] = 0.0f;
        }
    }
    
    // Map to power structure (based on STM32 order)
    for (int i = 0; i < 3; i++) {
        power.voltage[i] = values[i + 10];      // Voltage A,B,C (index 10,11,12)
        power.current[i] = values[i + 7];       // Current A,B,C (index 7,8,9)
        power.active_power[i] = values[i + 4];  // Active Power A,B,C (index 4,5,6)
        power.reactive_power[i] = values[i + 1]; // Reactive Power A,B,C (index 1,2,3)
    }
    power.frequency = values[0];                // Frequency (index 0)
    
    return true;
}

void printPowerData() {
    Serial.println("\n========== POWER DATA ==========");
    for (int i = 0; i < 3; i++) {
        char phase = 'A' + i;
        float P = power.active_power[i];
        float Q = power.reactive_power[i];
        float S = sqrt(P*P + Q*Q);
        float PF = (S > 0.001f) ? (P / S) : 0.0f;
        
        Serial.printf("Phase %c: V=%.1fV I=%.2fA P=%.1fW Q=%.1fVAR S=%.1fVA PF=%.3f\n",
            phase, power.voltage[i], power.current[i], P, Q, S, PF);
    }
    Serial.printf("Frequency: %.2f Hz\n", power.frequency);
    Serial.printf("Frame count: %d\n", frame_count);
    Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
    Serial.println("================================\n");
}

void loadWiFiConfig() {
    EEPROM.begin(512);
    for (int i = 0; i < 31; i++) {
        wifi_ssid[i] = EEPROM.read(i);
        if (wifi_ssid[i] == 0) break;
    }
    wifi_ssid[31] = 0;
    
    for (int i = 0; i < 31; i++) {
        wifi_pass[i] = EEPROM.read(32 + i);
        if (wifi_pass[i] == 0) break;
    }
    wifi_pass[31] = 0;
}

void saveWiFiConfig() {
    for (int i = 0; i < 32; i++) {
        EEPROM.write(i, wifi_ssid[i]);
        EEPROM.write(32 + i, wifi_pass[i]);
    }
    EEPROM.commit();
}

bool connectWiFi() {
    if (strlen(wifi_ssid) == 0) return false;
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifi_ssid, wifi_pass);
    
    for (int i = 0; i < 20; i++) {
        if (WiFi.status() == WL_CONNECTED) {
            Serial.printf("WiFi connected: %s\n", WiFi.localIP().toString().c_str());
            return true;
        }
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi failed");
    return false;
}

void startAP() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ap_ssid, ap_pass);
    Serial.printf("AP started: %s\n", WiFi.softAPIP().toString().c_str());
}

void handleRoot() {
    String html = "<!DOCTYPE html><html><body>";
    html += "<h1>ESP32 Power Meter Config</h1>";
    html += "<form action='/save' method='POST'>";
    html += "WiFi SSID: <input type='text' name='ssid' value='";
    html += String(wifi_ssid);
    html += "'><br><br>";
    html += "WiFi Password: <input type='password' name='pass' value='";
    html += String(wifi_pass);
    html += "'><br><br>";
    html += "<input type='submit' value='Save & Restart'>";
    html += "</form>";
    
    html += "<h2>Power Data</h2>";
    for (int i = 0; i < 3; i++) {
        html += "Phase " + String(char('A' + i)) + ": ";
        html += String(power.voltage[i], 1) + "V, ";
        html += String(power.current[i], 2) + "A, ";
        html += String(power.active_power[i], 1) + "W<br>";
    }
    html += "Frequency: " + String(power.frequency, 2) + " Hz<br>";
    html += "Frames: " + String(frame_count) + "<br>";
    html += "Free Memory: " + String(ESP.getFreeHeap()) + " bytes<br>";
    
    html += "</body></html>";
    server.send(200, "text/html", html);
}

void handleSave() {
    String ssid = server.arg("ssid");
    String pass = server.arg("pass");
    
    ssid.toCharArray(wifi_ssid, 32);
    pass.toCharArray(wifi_pass, 32);
    
    saveWiFiConfig();
    
    server.send(200, "text/html", "<h1>Saved! Restarting...</h1>");
    delay(2000);
    ESP.restart();
}

//==================== SETUP ===========================
void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("ESP32 Power Meter Starting...");
    Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
    
    // Load WiFi config
    loadWiFiConfig();
    
    // Try WiFi, fallback to AP
    if (!connectWiFi()) {
        startAP();
    }
    
    // Web server
    server.on("/", handleRoot);
    server.on("/save", handleSave);
    server.begin();
    
    // SPI setup
    spi.setDataMode(SPI_MODE0);
    spi.setQueueSize(1);
    
    if (!spi.begin(SPI_BUS, PIN_SCLK, PIN_MISO, PIN_MOSI, PIN_SS)) {
        Serial.println("SPI init failed!");
        delay(5000);
        ESP.restart();
    }
    
    Serial.println("Setup complete");
    last_print = millis();
}

//==================== MAIN LOOP ============================
void loop() {
    // Handle web server
    server.handleClient();
    yield();
    
    // Check memory
    if (ESP.getFreeHeap() < 8000) {
        Serial.println("Low memory, restarting...");
        ESP.restart();
    }
    
    // SPI receive
    memset(rx_buf, 0, RX_SIZE);
    size_t n = spi.transfer(nullptr, rx_buf, RX_SIZE, 30);
    
    if (n >= 55 && rx_buf[0] == 0xAA && rx_buf[1] == 0x55) {
        if (decodeFrame(rx_buf, n)) {
            frame_count++;
        }
    }
    
    // Print data every 5 seconds
    if (millis() - last_print > 5000) {
        printPowerData();
        last_print = millis();
    }
    
    delay(50); // Reduce CPU load
}
