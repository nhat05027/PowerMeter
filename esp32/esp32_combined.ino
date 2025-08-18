#include <Arduino.h>
#include <ESP32DMASPISlave.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <EEPROM.h>
#include <WiFiClientSecure.h>

// ==== SPI DMA Configuration ====
ESP32DMASPI::Slave slave;
static constexpr size_t BUFFER_SIZE = 256;
static constexpr size_t QUEUE_SIZE = 1;
uint8_t *dma_rx_buf;

// ==== Frame Protocol ====
static constexpr int FRAME_SIZE = 55;
static constexpr int SYNC_SIZE = 2;        // AA 55
static constexpr int HEADER_SIZE = 1;      // header byte
static constexpr int PAYLOAD_SIZE = 52;    // 13 floats = 52 bytes
static constexpr int EXPECTED_HEADER = 0x00; // STM32 đang gửi 0x00, không phải 0x01

// FNV-1a hash constants
static constexpr uint32_t FNV_OFFSET_BASIS_32 = 0x811C9DC5;
static constexpr uint32_t FNV_PRIME_32 = 16777619;

// ==== Power Meter Data Structure ====
typedef struct {
    // Raw data from STM32 (13 floats)
    float frequency;
    float voltage_a, current_a, active_power_a, apparent_power_a;
    float voltage_b, current_b, active_power_b, apparent_power_b;
    float voltage_c, current_c, active_power_c, apparent_power_c;
    
    // Calculated values
    float reactive_power_a, reactive_power_b, reactive_power_c;
    float power_factor_a, power_factor_b, power_factor_c;
    float total_active_power, total_apparent_power, total_reactive_power;
    float total_current, total_power_factor;
    float total_energy_kwh;
    
    // Status flags
    bool phase_a_active, phase_b_active, phase_c_active;
} EnergyData;

EnergyData energyData = {0};

// ==== SPI Variables ====
unsigned long last_summary = 0;
unsigned long frames_decoded = 0;
static bool has_last_hash = false;
static uint32_t last_hash = 0;

// ==== MQTT & WiFi ====
WiFiClientSecure espClient;
PubSubClient mqttClient(espClient);
WebServer webServer(80);

String mqtt_server = "";
String mqtt_user = "";
String mqtt_pass = "";
int mqtt_port = 1883;
unsigned long lastSend = 0;

#define EEPROM_SIZE 512

// ==== Function Declarations ====
void processReceivedData(uint8_t* buffer, size_t len);
bool decodePowerMeterFrame(const uint8_t* data, size_t len);
float extractFloat(const uint8_t* data, int index);
uint32_t fnv1a32(const uint8_t* data, size_t len);
void calculateDerivedValues();
void checkPhaseStatus();
void printRawData(const uint8_t* data, size_t len);
void printPowerMeterSummary();

// MQTT & Web functions
void saveMQTTConfig(const char* server, const char* user, const char* pass, int port);
void loadMQTTConfig();
void handleRoot();
void handleRealtime();
void handleApi();
void handleReset();
void reconnectMQTT();
void publishEnergyData();

// ==== MQTT Config Functions ====
void saveMQTTConfig(const char* server, const char* user, const char* pass, int port) {
    EEPROM.begin(EEPROM_SIZE);
    char buf[128];
    memset(buf, 0, sizeof(buf));
    strncpy(buf, server, 63); EEPROM.put(0, buf);
    strncpy(buf, user, 31);   EEPROM.put(64, buf);
    strncpy(buf, pass, 31);   EEPROM.put(96, buf);
    EEPROM.put(128, port);
    EEPROM.commit();
    mqtt_server = String(server);
    mqtt_user   = String(user);
    mqtt_pass   = String(pass);
    mqtt_port   = port;
}

void loadMQTTConfig() {
    EEPROM.begin(EEPROM_SIZE);
    char s[64] = "", u[32] = "", p[32] = ""; 
    int port = 1883;
    EEPROM.get(0, s);  mqtt_server = String(s);
    EEPROM.get(64, u); mqtt_user   = String(u);
    EEPROM.get(96, p); mqtt_pass   = String(p);
    EEPROM.get(128, port); mqtt_port = port;
    if (mqtt_server.length() == 0) mqtt_server = "";
    if (mqtt_user.length() == 0) mqtt_user = "";
    if (mqtt_pass.length() == 0) mqtt_pass = "";
    if (mqtt_port <= 0 || mqtt_port > 65535) mqtt_port = 1883;
}

// ==== Web Pages ====
String apWebPage = R"rawliteral(
<!DOCTYPE html><html><head><title>MQTT Config</title>
<meta charset="UTF-8"><meta name='viewport' content='width=device-width,initial-scale=1'>
<style>body{font-family:sans-serif;background:#e7fbe8;padding:22px;}form{background:#fff;border-radius:13px;max-width:320px;margin:auto;box-shadow:0 6px 30px #7ae38933;padding:20px;}h2{color:#2aa449;margin-bottom:10px;}input[type=text],input[type=password],input[type=number]{width:98%;padding:8px 5px;border-radius:5px;border:1px solid #ccc;margin-bottom:14px;}input[type=submit]{background:#2aa449;color:#fff;border:none;padding:9px 0;border-radius:7px;width:100%;font-size:1.1em;}a{color:#2aa449;font-size:1em;display:block;margin-top:10px;text-align:center;}</style></head>
<body><form method='POST'><h2>MQTT Config</h2>
Server:<br><input name='server' required><br>
Port:<br><input name='port' value='1883' type='number'><br>
Username:<br><input name='user'><br>
Password:<br><input name='pass' type='password'><br>
<input type='submit' value='Save & Restart'></form>
<a href='/realtime'>Xem dữ liệu real-time</a></body></html>)rawliteral";

String realtimePageHeader = R"rawliteral(
<!DOCTYPE html><html lang="vi"><head><meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Real-Time Energy Meter</title>
<style>body{background:#e7fbe8;font-family:sans-serif;margin:0;padding:0;}
.container{max-width:420px;background:#fff;border-radius:16px;box-shadow:0 4px 32px #7ae38933;margin:26px auto;padding:18px;}
h2{color:#2aa449;text-align:center;margin-bottom:12px;}
table{width:100%;border-collapse:collapse;}
td,th{padding:7px 6px;text-align:right;font-size:1.02em;}
th{text-align:left;font-weight:600;color:#20923d;}
tr:nth-child(even){background:#f5fef6;}
.inactive{color:#999;font-style:italic;}
a{color:#2aa449;text-decoration:none;font-size:0.97em;}
.status{font-weight:bold;}
.active{color:#2aa449;}
.inactive{color:#ccc;}
</style>
<script>
function reloadData(){
  fetch('/api').then(r=>r.json()).then(d=>{
    for (const [k,v] of Object.entries(d)){
      let el = document.getElementById(k);
      if(el) {
        if(k.includes('phase_') && k.includes('_active')) {
          el.textContent = v ? 'ACTIVE' : 'INACTIVE';
          el.className = v ? 'status active' : 'status inactive';
        } else if(typeof v === 'number') {
          el.textContent = v.toFixed(2);
        } else {
          el.textContent = v;
        }
      }
    }
  });
}
setInterval(reloadData, 2000);
window.onload = reloadData;
</script></head><body>
<div class='container'><h2>Power Meter Real-Time</h2><table>
<tr><th colspan="2">System Status</th></tr>
<tr><td>Frames Decoded</td><td id='frames_decoded'></td></tr>
<tr><td>Frequency (Hz)</td><td id='frequency'></td></tr>
<tr><th colspan="2">Phase A</th></tr>
<tr><td>Status</td><td id='phase_a_active' class='status'></td></tr>
<tr><td>Voltage (V)</td><td id='voltage_a'></td></tr>
<tr><td>Current (A)</td><td id='current_a'></td></tr>
<tr><td>Active Power (W)</td><td id='active_power_a'></td></tr>
<tr><td>Apparent Power (VA)</td><td id='apparent_power_a'></td></tr>
<tr><td>Reactive Power (VAR)</td><td id='reactive_power_a'></td></tr>
<tr><td>Power Factor</td><td id='power_factor_a'></td></tr>
<tr><th colspan="2">Phase B</th></tr>
<tr><td>Status</td><td id='phase_b_active' class='status'></td></tr>
<tr><td>Voltage (V)</td><td id='voltage_b'></td></tr>
<tr><td>Current (A)</td><td id='current_b'></td></tr>
<tr><td>Active Power (W)</td><td id='active_power_b'></td></tr>
<tr><td>Apparent Power (VA)</td><td id='apparent_power_b'></td></tr>
<tr><td>Reactive Power (VAR)</td><td id='reactive_power_b'></td></tr>
<tr><td>Power Factor</td><td id='power_factor_b'></td></tr>
<tr><th colspan="2">Phase C</th></tr>
<tr><td>Status</td><td id='phase_c_active' class='status'></td></tr>
<tr><td>Voltage (V)</td><td id='voltage_c'></td></tr>
<tr><td>Current (A)</td><td id='current_c'></td></tr>
<tr><td>Active Power (W)</td><td id='active_power_c'></td></tr>
<tr><td>Apparent Power (VA)</td><td id='apparent_power_c'></td></tr>
<tr><td>Reactive Power (VAR)</td><td id='reactive_power_c'></td></tr>
<tr><td>Power Factor</td><td id='power_factor_c'></td></tr>
<tr><th colspan="2">System Totals</th></tr>
<tr><td>Total Active (W)</td><td id='total_active_power'></td></tr>
<tr><td>Total Apparent (VA)</td><td id='total_apparent_power'></td></tr>
<tr><td>Total Reactive (VAR)</td><td id='total_reactive_power'></td></tr>
<tr><td>Total Current (A)</td><td id='total_current'></td></tr>
<tr><td>Total Power Factor</td><td id='total_power_factor'></td></tr>
<tr><td>Total Energy (kWh)</td><td id='total_energy_kwh'></td></tr>
</table><a href='/'>Cấu hình MQTT</a></div></body></html>)rawliteral";

// ==== Web Handlers ====
void handleRoot() {
    if (webServer.method() == HTTP_POST && webServer.hasArg("server")) {
        saveMQTTConfig(
            webServer.arg("server").c_str(),
            webServer.arg("user").c_str(),
            webServer.arg("pass").c_str(),
            webServer.arg("port").toInt()
        );
        webServer.send(200, "text/html", "<h3>Saved. Rebooting...</h3>");
        delay(1000); 
        ESP.restart();
    } else {
        webServer.send(200, "text/html", apWebPage);
    }
}

void handleRealtime() {
    webServer.send(200, "text/html", realtimePageHeader);
}

void handleApi() {
    String json = "{";
    json += "\"frames_decoded\":" + String(frames_decoded) + ",";
    json += "\"frequency\":" + String(energyData.frequency,2) + ",";
    
    // Phase A
    json += "\"phase_a_active\":" + String(energyData.phase_a_active ? "true" : "false") + ",";
    json += "\"voltage_a\":" + String(energyData.voltage_a,2) + ",";
    json += "\"current_a\":" + String(energyData.current_a,2) + ",";
    json += "\"active_power_a\":" + String(energyData.active_power_a,2) + ",";
    json += "\"apparent_power_a\":" + String(energyData.apparent_power_a,2) + ",";
    json += "\"reactive_power_a\":" + String(energyData.reactive_power_a,2) + ",";
    json += "\"power_factor_a\":" + String(energyData.power_factor_a,3) + ",";
    
    // Phase B
    json += "\"phase_b_active\":" + String(energyData.phase_b_active ? "true" : "false") + ",";
    json += "\"voltage_b\":" + String(energyData.voltage_b,2) + ",";
    json += "\"current_b\":" + String(energyData.current_b,2) + ",";
    json += "\"active_power_b\":" + String(energyData.active_power_b,2) + ",";
    json += "\"apparent_power_b\":" + String(energyData.apparent_power_b,2) + ",";
    json += "\"reactive_power_b\":" + String(energyData.reactive_power_b,2) + ",";
    json += "\"power_factor_b\":" + String(energyData.power_factor_b,3) + ",";
    
    // Phase C
    json += "\"phase_c_active\":" + String(energyData.phase_c_active ? "true" : "false") + ",";
    json += "\"voltage_c\":" + String(energyData.voltage_c,2) + ",";
    json += "\"current_c\":" + String(energyData.current_c,2) + ",";
    json += "\"active_power_c\":" + String(energyData.active_power_c,2) + ",";
    json += "\"apparent_power_c\":" + String(energyData.apparent_power_c,2) + ",";
    json += "\"reactive_power_c\":" + String(energyData.reactive_power_c,2) + ",";
    json += "\"power_factor_c\":" + String(energyData.power_factor_c,3) + ",";
    
    // Totals
    json += "\"total_active_power\":" + String(energyData.total_active_power,2) + ",";
    json += "\"total_apparent_power\":" + String(energyData.total_apparent_power,2) + ",";
    json += "\"total_reactive_power\":" + String(energyData.total_reactive_power,2) + ",";
    json += "\"total_current\":" + String(energyData.total_current,2) + ",";
    json += "\"total_power_factor\":" + String(energyData.total_power_factor,3) + ",";
    json += "\"total_energy_kwh\":" + String(energyData.total_energy_kwh,4);
    json += "}";
    
    webServer.send(200, "application/json", json);
}

void handleReset() {
    webServer.send(200, "text/html", "<h3>Restarting...</h3>");
    delay(1000); 
    ESP.restart();
}

// ==== MQTT Functions ====
void reconnectMQTT() {
    static unsigned long lastTry = 0;
    if (mqtt_server.length() == 0 || mqttClient.connected()) return;
    if (millis() - lastTry < 5000) return; // Try every 5 seconds
    lastTry = millis();
    
    if (mqttClient.connect("ESPEnergyMeter", mqtt_user.c_str(), mqtt_pass.c_str())) {
        Serial.println("[MQTT] Connected!");
    } else {
        Serial.print("[MQTT] Failed, rc="); 
        Serial.println(mqttClient.state());
    }
}

void publishEnergyData() {
    if (!mqttClient.connected()) return;
    
    // System data
    mqttClient.publish("energy_meter/frequency", String(energyData.frequency,2).c_str(), true);
    mqttClient.publish("energy_meter/frames_decoded", String(frames_decoded).c_str(), true);
    
    // Phase A (always publish, 0 if inactive)
    if (energyData.phase_a_active) {
        mqttClient.publish("energy_meter/voltage_a", String(energyData.voltage_a,2).c_str(), true);
        mqttClient.publish("energy_meter/current_a", String(energyData.current_a,2).c_str(), true);
        mqttClient.publish("energy_meter/active_power_a", String(energyData.active_power_a,2).c_str(), true);
        mqttClient.publish("energy_meter/apparent_power_a", String(energyData.apparent_power_a,2).c_str(), true);
        mqttClient.publish("energy_meter/reactive_power_a", String(energyData.reactive_power_a,2).c_str(), true);
        mqttClient.publish("energy_meter/power_factor_a", String(energyData.power_factor_a,3).c_str(), true);
    } else {
        mqttClient.publish("energy_meter/voltage_a", "0.00", true);
        mqttClient.publish("energy_meter/current_a", "0.00", true);
        mqttClient.publish("energy_meter/active_power_a", "0.00", true);
        mqttClient.publish("energy_meter/apparent_power_a", "0.00", true);
        mqttClient.publish("energy_meter/reactive_power_a", "0.00", true);
        mqttClient.publish("energy_meter/power_factor_a", "0.000", true);
    }
    
    // Phase B (always publish, 0 if inactive)
    if (energyData.phase_b_active) {
        mqttClient.publish("energy_meter/voltage_b", String(energyData.voltage_b,2).c_str(), true);
        mqttClient.publish("energy_meter/current_b", String(energyData.current_b,2).c_str(), true);
        mqttClient.publish("energy_meter/active_power_b", String(energyData.active_power_b,2).c_str(), true);
        mqttClient.publish("energy_meter/apparent_power_b", String(energyData.apparent_power_b,2).c_str(), true);
        mqttClient.publish("energy_meter/reactive_power_b", String(energyData.reactive_power_b,2).c_str(), true);
        mqttClient.publish("energy_meter/power_factor_b", String(energyData.power_factor_b,3).c_str(), true);
    } else {
        mqttClient.publish("energy_meter/voltage_b", "0.00", true);
        mqttClient.publish("energy_meter/current_b", "0.00", true);
        mqttClient.publish("energy_meter/active_power_b", "0.00", true);
        mqttClient.publish("energy_meter/apparent_power_b", "0.00", true);
        mqttClient.publish("energy_meter/reactive_power_b", "0.00", true);
        mqttClient.publish("energy_meter/power_factor_b", "0.000", true);
    }
    
    // Phase C (always publish, 0 if inactive)
    if (energyData.phase_c_active) {
        mqttClient.publish("energy_meter/voltage_c", String(energyData.voltage_c,2).c_str(), true);
        mqttClient.publish("energy_meter/current_c", String(energyData.current_c,2).c_str(), true);
        mqttClient.publish("energy_meter/active_power_c", String(energyData.active_power_c,2).c_str(), true);
        mqttClient.publish("energy_meter/apparent_power_c", String(energyData.apparent_power_c,2).c_str(), true);
        mqttClient.publish("energy_meter/reactive_power_c", String(energyData.reactive_power_c,2).c_str(), true);
        mqttClient.publish("energy_meter/power_factor_c", String(energyData.power_factor_c,3).c_str(), true);
    } else {
        mqttClient.publish("energy_meter/voltage_c", "0.00", true);
        mqttClient.publish("energy_meter/current_c", "0.00", true);
        mqttClient.publish("energy_meter/active_power_c", "0.00", true);
        mqttClient.publish("energy_meter/apparent_power_c", "0.00", true);
        mqttClient.publish("energy_meter/reactive_power_c", "0.00", true);
        mqttClient.publish("energy_meter/power_factor_c", "0.000", true);
    }
    
    // Phase status
    mqttClient.publish("energy_meter/phase_a_active", energyData.phase_a_active ? "true" : "false", true);
    mqttClient.publish("energy_meter/phase_b_active", energyData.phase_b_active ? "true" : "false", true);
    mqttClient.publish("energy_meter/phase_c_active", energyData.phase_c_active ? "true" : "false", true);
    
    // System totals
    mqttClient.publish("energy_meter/total_active_power", String(energyData.total_active_power,2).c_str(), true);
    mqttClient.publish("energy_meter/total_apparent_power", String(energyData.total_apparent_power,2).c_str(), true);
    mqttClient.publish("energy_meter/total_reactive_power", String(energyData.total_reactive_power,2).c_str(), true);
    mqttClient.publish("energy_meter/total_current", String(energyData.total_current,2).c_str(), true);
    mqttClient.publish("energy_meter/total_power_factor", String(energyData.total_power_factor,3).c_str(), true);
    mqttClient.publish("energy_meter/total_energy_kwh", String(energyData.total_energy_kwh,4).c_str(), true);
}

// ==== SPI Processing Functions ====
uint32_t fnv1a32(const uint8_t* data, size_t len) {
    uint32_t hash = FNV_OFFSET_BASIS_32;
    for (size_t i = 0; i < len; i++) {
        hash ^= data[i];
        hash *= FNV_PRIME_32;
    }
    return hash;
}

float extractFloat(const uint8_t* data, int index) {
    // Byte order reversed [b3 b2 b1 b0] - giống code working
    size_t offset = index * 4;
    uint8_t bytes[4];
    bytes[0] = data[offset + 3];
    bytes[1] = data[offset + 2];
    bytes[2] = data[offset + 1];
    bytes[3] = data[offset + 0];

    float value;
    memcpy(&value, bytes, 4);

    // Validate float value
    if (!isfinite(value) || fabsf(value) > 100000.0f) return 0.0f;
    return value;
}

void processReceivedData(uint8_t* buffer, size_t len) {
    // Look for sync pattern AA 55
    int sync_pos = -1;
    for (int i = 0; i <= (int)len - FRAME_SIZE; i++) {
        if (buffer[i] == 0xAA && buffer[i+1] == 0x55) {
            sync_pos = i;
            break;
        }
    }

    if (sync_pos >= 0 && sync_pos + FRAME_SIZE <= (int)len) {
        // Hash payload (HEADER + 13 floats), excluding AA 55
        const uint8_t* payload = &buffer[sync_pos + SYNC_SIZE];
        uint32_t h = fnv1a32(payload, 53); // 1 header + 13*4 floats = 53 bytes
        bool is_new = (!has_last_hash) || (h != last_hash);

        if (is_new) {
            Serial.printf("\n[SPI] NEW frame %u bytes at %lu ms (sync@%d)\n",
                          (unsigned)len, millis(), sync_pos);
            
            // Print raw data như code working
            printRawData(buffer, len);
            
            if (decodePowerMeterFrame(buffer, len)) {
                frames_decoded++;
                last_hash = h;
                has_last_hash = true;
                
                // Check which phases are active and calculate derived values
                checkPhaseStatus();
                calculateDerivedValues();
            }
        }
    }
}

bool decodePowerMeterFrame(const uint8_t* data, size_t len) {
    // Find sync pattern
    int sync_pos = -1;
    for (int i = 0; i <= (int)len - FRAME_SIZE; i++) {
        if (data[i] == 0xAA && data[i+1] == 0x55) {
            sync_pos = i;
            break;
        }
    }

    if (sync_pos < 0 || sync_pos + FRAME_SIZE > (int)len) {
        return false;
    }

    const uint8_t* frame = &data[sync_pos];
    uint8_t header = frame[2];

    // Debug: in header byte để xem STM32 gửi gì
    // Serial.printf("[DEBUG] Header byte: 0x%02X\n", header);

    if (header != EXPECTED_HEADER) {
        return false;
    }

    // Extract 13 floats from frame (theo field mapping của code working)
    const uint8_t* float_data = &frame[3];
    
    // Parse 13 floats after header
    float v[13];
    for (int i = 0; i < 13; i++) {
        v[i] = extractFloat(float_data, i);
    }

    // Field mapping từ code esp32_dma.ino working:
    // 0: Freq
    // 1: S_C, 2: S_B, 3: S_A
    // 4: P_C, 5: P_B, 6: P_A  
    // 7: I_C, 8: I_B, 9: I_A
    // 10: V_C, 11: V_B, 12: V_A
    energyData.frequency = v[0];

    // Phase C
    energyData.apparent_power_c = v[1];
    energyData.active_power_c = v[4];
    energyData.current_c = v[7];
    energyData.voltage_c = v[10];

    // Phase B  
    energyData.apparent_power_b = v[2];
    energyData.active_power_b = v[5];
    energyData.current_b = v[8];
    energyData.voltage_b = v[11];

    // Phase A
    energyData.apparent_power_a = v[3];
    energyData.active_power_a = v[6];
    energyData.current_a = v[9];
    energyData.voltage_a = v[12];

    // Debug output như code working
    Serial.println("\n[SUCCESS] ✓ New frame decoded");
    Serial.printf("  A: V=%.1fV  I=%.2fA  P=%.1fW  S=%.1fVA\n",
                  energyData.voltage_a, energyData.current_a,
                  energyData.active_power_a, energyData.apparent_power_a);
    Serial.printf("  B: V=%.1fV  I=%.2fA  P=%.1fW  S=%.1fVA\n",
                  energyData.voltage_b, energyData.current_b,
                  energyData.active_power_b, energyData.apparent_power_b);
    Serial.printf("  C: V=%.1fV  I=%.2fA  P=%.1fW  S=%.1fVA\n",
                  energyData.voltage_c, energyData.current_c,
                  energyData.active_power_c, energyData.apparent_power_c);
    Serial.printf("  f = %.2f Hz\n", energyData.frequency);

    return true;
}

void checkPhaseStatus() {
    // Check if phases are active based on voltage and current thresholds
    energyData.phase_a_active = (energyData.voltage_a > 10.0 && energyData.current_a > 0.01);
    energyData.phase_b_active = (energyData.voltage_b > 10.0 && energyData.current_b > 0.01);
    energyData.phase_c_active = (energyData.voltage_c > 10.0 && energyData.current_c > 0.01);
}

void calculateDerivedValues() {
    // Calculate reactive power for each phase (only if active)
    if (energyData.phase_a_active) {
        float S2 = energyData.apparent_power_a * energyData.apparent_power_a;
        float P2 = energyData.active_power_a * energyData.active_power_a;
        energyData.reactive_power_a = sqrt(abs(S2 - P2));
        
        if (energyData.apparent_power_a > 0.1) {
            energyData.power_factor_a = energyData.active_power_a / energyData.apparent_power_a;
        } else {
            energyData.power_factor_a = 0.0;
        }
    } else {
        energyData.reactive_power_a = 0.0;
        energyData.power_factor_a = 0.0;
    }
    
    if (energyData.phase_b_active) {
        float S2 = energyData.apparent_power_b * energyData.apparent_power_b;
        float P2 = energyData.active_power_b * energyData.active_power_b;
        energyData.reactive_power_b = sqrt(abs(S2 - P2));
        
        if (energyData.apparent_power_b > 0.1) {
            energyData.power_factor_b = energyData.active_power_b / energyData.apparent_power_b;
        } else {
            energyData.power_factor_b = 0.0;
        }
    } else {
        energyData.reactive_power_b = 0.0;
        energyData.power_factor_b = 0.0;
    }
    
    if (energyData.phase_c_active) {
        float S2 = energyData.apparent_power_c * energyData.apparent_power_c;
        float P2 = energyData.active_power_c * energyData.active_power_c;
        energyData.reactive_power_c = sqrt(abs(S2 - P2));
        
        if (energyData.apparent_power_c > 0.1) {
            energyData.power_factor_c = energyData.active_power_c / energyData.apparent_power_c;
        } else {
            energyData.power_factor_c = 0.0;
        }
    } else {
        energyData.reactive_power_c = 0.0;
        energyData.power_factor_c = 0.0;
    }
    
    // Calculate system totals (only from active phases)
    energyData.total_active_power = 0.0;
    energyData.total_apparent_power = 0.0;
    energyData.total_reactive_power = 0.0;
    energyData.total_current = 0.0;
    
    if (energyData.phase_a_active) {
        energyData.total_active_power += energyData.active_power_a;
        energyData.total_apparent_power += energyData.apparent_power_a;
        energyData.total_reactive_power += energyData.reactive_power_a;
        energyData.total_current += energyData.current_a;
    }
    
    if (energyData.phase_b_active) {
        energyData.total_active_power += energyData.active_power_b;
        energyData.total_apparent_power += energyData.apparent_power_b;
        energyData.total_reactive_power += energyData.reactive_power_b;
        energyData.total_current += energyData.current_b;
    }
    
    if (energyData.phase_c_active) {
        energyData.total_active_power += energyData.active_power_c;
        energyData.total_apparent_power += energyData.apparent_power_c;
        energyData.total_reactive_power += energyData.reactive_power_c;
        energyData.total_current += energyData.current_c;
    }
    
    // Calculate total power factor
    if (energyData.total_apparent_power > 0.1) {
        energyData.total_power_factor = energyData.total_active_power / energyData.total_apparent_power;
    } else {
        energyData.total_power_factor = 0.0;
    }
    
    // Calculate energy consumption (kWh)
    static unsigned long last_energy_calc = 0;
    unsigned long now = millis();
    
    if (last_energy_calc > 0 && energyData.total_active_power > 0) {
        float hours = (now - last_energy_calc) / 3600000.0; // Convert ms to hours
        energyData.total_energy_kwh += (energyData.total_active_power / 1000.0) * hours; // Convert W to kW
    }
    
    last_energy_calc = now;
}

void printRawData(const uint8_t* data, size_t len) {
    Serial.print("[RAW] ");
    for (size_t i = 0; i < len && i < 64; i++) {
        Serial.printf("%02X ", data[i]);
        if ((i + 1) % 16 == 0) Serial.println("\n      ");
    }
    if (len % 16 != 0) Serial.println();
}

void printPowerMeterSummary() {
    if (frames_decoded == 0) {
        Serial.println("\n[INFO] No frames decoded yet. Waiting for STM32...");
        return;
    }
    
    Serial.println("\n============= POWER METER SUMMARY =============");
    Serial.printf("Frames decoded: %lu\n", frames_decoded);
    Serial.printf("Frequency: %.2f Hz\n", energyData.frequency);
    
    Serial.printf("--- Phase A (%s) ---\n", energyData.phase_a_active ? "ACTIVE" : "INACTIVE");
    if (energyData.phase_a_active) {
        Serial.printf("Voltage: %.2f V, Current: %.2f A\n", energyData.voltage_a, energyData.current_a);
        Serial.printf("Active: %.2f W, Apparent: %.2f VA, Reactive: %.2f VAR\n", 
                      energyData.active_power_a, energyData.apparent_power_a, energyData.reactive_power_a);
        Serial.printf("Power Factor: %.3f\n", energyData.power_factor_a);
    }
    
    Serial.printf("--- Phase B (%s) ---\n", energyData.phase_b_active ? "ACTIVE" : "INACTIVE");
    if (energyData.phase_b_active) {
        Serial.printf("Voltage: %.2f V, Current: %.2f A\n", energyData.voltage_b, energyData.current_b);
        Serial.printf("Active: %.2f W, Apparent: %.2f VA, Reactive: %.2f VAR\n", 
                      energyData.active_power_b, energyData.apparent_power_b, energyData.reactive_power_b);
        Serial.printf("Power Factor: %.3f\n", energyData.power_factor_b);
    }
    
    Serial.printf("--- Phase C (%s) ---\n", energyData.phase_c_active ? "ACTIVE" : "INACTIVE");
    if (energyData.phase_c_active) {
        Serial.printf("Voltage: %.2f V, Current: %.2f A\n", energyData.voltage_c, energyData.current_c);
        Serial.printf("Active: %.2f W, Apparent: %.2f VA, Reactive: %.2f VAR\n", 
                      energyData.active_power_c, energyData.apparent_power_c, energyData.reactive_power_c);
        Serial.printf("Power Factor: %.3f\n", energyData.power_factor_c);
    }
    
    Serial.println("--- SYSTEM TOTALS ---");
    Serial.printf("Total Active: %.2f W, Total Apparent: %.2f VA\n", 
                  energyData.total_active_power, energyData.total_apparent_power);
    Serial.printf("Total Reactive: %.2f VAR, Total Current: %.2f A\n", 
                  energyData.total_reactive_power, energyData.total_current);
    Serial.printf("Total Power Factor: %.3f\n", energyData.total_power_factor);
    Serial.printf("Total Energy: %.4f kWh\n", energyData.total_energy_kwh);
    Serial.println("=============================================\n");
}

// ==== Setup Function ====
void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("Starting ESP32 Power Meter with SPI + WiFi/MQTT...");
    
    // Load MQTT configuration from EEPROM
    loadMQTTConfig();
    
    // Allocate DMA buffer for SPI
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
    
    Serial.println("[OK] SPI slave initialized");
    
    // Setup WiFi using WiFiManager
    WiFiManager wm;
    wm.setTimeout(120);
    if (!wm.autoConnect("ESP-PowerMeter", "12345678")) {
        Serial.println("[WARN] WiFi connection failed, starting AP mode");
        WiFi.softAP("ESP-Config", "12345678");
    } else {
        Serial.println("[OK] WiFi connected");
        Serial.print("[INFO] IP address: ");
        Serial.println(WiFi.localIP());
    }
    
    // Setup web server
    webServer.on("/", handleRoot);
    webServer.on("/realtime", handleRealtime);
    webServer.on("/api", handleApi);
    webServer.on("/reset", handleReset);
    webServer.begin();
    Serial.println("[OK] Web server started");
    
    // Setup MQTT client
    if (mqtt_server.length() > 0) {
        espClient.setInsecure();
        mqttClient.setServer(mqtt_server.c_str(), mqtt_port);
        Serial.printf("[INFO] MQTT configured: %s:%d\n", mqtt_server.c_str(), mqtt_port);
    }
    
    Serial.println("\n=========================================");
    Serial.println("ESP32 Power Meter Ready!");
    Serial.println("SPI: Waiting for STM32 data...");
    Serial.printf("Web: http://%s\n", WiFi.localIP().toString().c_str());
    Serial.println("=========================================\n");
    
    last_summary = millis();
    lastSend = millis();
}

// ==== Main Loop ====
void loop() {
    // Handle SPI communication
    if (slave.hasTransactionsCompletedAndAllResultsHandled()) {
        // Initialize rx buffer
        memset(dma_rx_buf, 0x00, BUFFER_SIZE);
        
        // Queue receive transaction
        slave.queue(nullptr, dma_rx_buf, BUFFER_SIZE);
        
        // Trigger transaction
        slave.trigger();
    }

    // Check if SPI transaction is completed
    if (slave.hasTransactionsCompletedAndAllResultsReady(QUEUE_SIZE)) {
        // Get received bytes
        const std::vector<size_t> received_bytes = slave.numBytesReceivedAll();
        if (received_bytes.size() > 0) {
            size_t n = received_bytes[0];
            if (n > 0) {
                processReceivedData(dma_rx_buf, n);
            }
        }
    }
    
    // Handle web server
    webServer.handleClient();
    
    // Handle MQTT
    if (mqttClient.connected()) {
        mqttClient.loop();
    } else {
        reconnectMQTT();
    }
    
    // Check WiFi connection
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[WARN] WiFi disconnected, restarting...");
        delay(1000);
        ESP.restart();
    }
    
    // Publish data to MQTT every 2 seconds
    if (millis() - lastSend > 2000) {
        publishEnergyData();
        lastSend = millis();
    }
    
    // Print summary every 10 seconds
    if (millis() - last_summary > 10000) {
        printPowerMeterSummary();
        last_summary = millis();
    }
    
    delay(10); // Small delay for stability
}
