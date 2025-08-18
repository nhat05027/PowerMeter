#include <WiFi.h>
#include <WebServer.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <EEPROM.h>
#include <WiFiClientSecure.h>

//------------------- MQTT topics ------------------------
const char* MQTT_BASE_TOPIC = "energy_meter/";

//------------------- Biến toàn cục ----------------------
WiFiClientSecure espClient;
PubSubClient mqttClient(espClient);
WebServer webServer(80);

String mqtt_server = "";
String mqtt_user = "";
String mqtt_pass = "";
int mqtt_port = 1883;

unsigned long lastSend = 0;

struct EnergyData {
  float voltage_a, voltage_b, voltage_c;
  float current_a, current_b, current_c;
  float active_power_a, active_power_b, active_power_c;
  float reactive_power_a, reactive_power_b, reactive_power_c;
  float apparent_power_a, apparent_power_b, apparent_power_c;
  float power_factor_a, power_factor_b, power_factor_c;
  float frequency;
} energyData;

#define EEPROM_SIZE 512
void handleReset();

//------------------- Config MQTT qua EEPROM -------------
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
  char s[64] = "", u[32] = "", p[32] = ""; int port = 1883;
  EEPROM.get(0, s);  mqtt_server = String(s);
  EEPROM.get(64, u); mqtt_user   = String(u);
  EEPROM.get(96, p); mqtt_pass   = String(p);
  EEPROM.get(128, port); mqtt_port = port;
  if (mqtt_server.length() == 0) mqtt_server = "";
  if (mqtt_user.length() == 0) mqtt_user = "";
  if (mqtt_pass.length() == 0) mqtt_pass = "";
  if (mqtt_port <= 0 || mqtt_port > 65535) mqtt_port = 1883;
}

//------------------- Web pages --------------------------
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
a{color:#2aa449;text-decoration:none;font-size:0.97em;}
</style>
<script>
function reloadData(){
  fetch('/api').then(r=>r.json()).then(d=>{
    for (const [k,v] of Object.entries(d)){
      let el = document.getElementById(k);
      if(el) el.textContent = Number(v).toFixed(2);
    }
  });
}
setInterval(reloadData, 2000);
window.onload = reloadData;
</script></head><body>
<div class='container'><h2>Dữ liệu Real-Time</h2><table>
<tr><th>Thông số</th><th>Giá trị</th></tr>
<tr><td>Voltage A (V)</td><td id='voltage_a'></td></tr>
<tr><td>Voltage B (V)</td><td id='voltage_b'></td></tr>
<tr><td>Voltage C (V)</td><td id='voltage_c'></td></tr>
<tr><td>Current A (A)</td><td id='current_a'></td></tr>
<tr><td>Current B (A)</td><td id='current_b'></td></tr>
<tr><td>Current C (A)</td><td id='current_c'></td></tr>
<tr><td>Active Power A (W)</td><td id='active_power_a'></td></tr>
<tr><td>Active Power B (W)</td><td id='active_power_b'></td></tr>
<tr><td>Active Power C (W)</td><td id='active_power_c'></td></tr>
<tr><td>Reactive Power A</td><td id='reactive_power_a'></td></tr>
<tr><td>Reactive Power B</td><td id='reactive_power_b'></td></tr>
<tr><td>Reactive Power C</td><td id='reactive_power_c'></td></tr>
<tr><td>Apparent Power A</td><td id='apparent_power_a'></td></tr>
<tr><td>Apparent Power B</td><td id='apparent_power_b'></td></tr>
<tr><td>Apparent Power C</td><td id='apparent_power_c'></td></tr>
<tr><td>Power Factor A</td><td id='power_factor_a'></td></tr>
<tr><td>Power Factor B</td><td id='power_factor_b'></td></tr>
<tr><td>Power Factor C</td><td id='power_factor_c'></td></tr>
<tr><td>Frequency (Hz)</td><td id='frequency'></td></tr>
</table><a href='/'>Cấu hình MQTT</a></div></body></html>)rawliteral";

//------------------- Web handlers -----------------------
void handleRoot() {
  if (webServer.method() == HTTP_POST && webServer.hasArg("server")) {
    saveMQTTConfig(
      webServer.arg("server").c_str(),
      webServer.arg("user").c_str(),
      webServer.arg("pass").c_str(),
      webServer.arg("port").toInt()
    );
    webServer.send(200, "text/html", "<h3>Saved. Rebooting...</h3>");
    delay(1000); ESP.restart();
  } else {
    webServer.send(200, "text/html", apWebPage);
  }
}

void handleRealtime() {
  webServer.send(200, "text/html", realtimePageHeader);
}

void handleApi() {
  String json = "{";
  json += "\"voltage_a\":" + String(energyData.voltage_a,2) + ",";
  json += "\"voltage_b\":" + String(energyData.voltage_b,2) + ",";
  json += "\"voltage_c\":" + String(energyData.voltage_c,2) + ",";
  json += "\"current_a\":" + String(energyData.current_a,2) + ",";
  json += "\"current_b\":" + String(energyData.current_b,2) + ",";
  json += "\"current_c\":" + String(energyData.current_c,2) + ",";
  json += "\"active_power_a\":" + String(energyData.active_power_a,2) + ",";
  json += "\"active_power_b\":" + String(energyData.active_power_b,2) + ",";
  json += "\"active_power_c\":" + String(energyData.active_power_c,2) + ",";
  json += "\"reactive_power_a\":" + String(energyData.reactive_power_a,2) + ",";
  json += "\"reactive_power_b\":" + String(energyData.reactive_power_b,2) + ",";
  json += "\"reactive_power_c\":" + String(energyData.reactive_power_c,2) + ",";
  json += "\"apparent_power_a\":" + String(energyData.apparent_power_a,2) + ",";
  json += "\"apparent_power_b\":" + String(energyData.apparent_power_b,2) + ",";
  json += "\"apparent_power_c\":" + String(energyData.apparent_power_c,2) + ",";
  json += "\"power_factor_a\":" + String(energyData.power_factor_a,2) + ",";
  json += "\"power_factor_b\":" + String(energyData.power_factor_b,2) + ",";
  json += "\"power_factor_c\":" + String(energyData.power_factor_c,2) + ",";
  json += "\"frequency\":" + String(energyData.frequency,2);
  json += "}";
  webServer.send(200, "application/json", json);
}

void handleReset() {
  webServer.send(200, "text/html", "<h3>Restarting...</h3>");
  delay(1000); ESP.restart();
}

//------------------- MQTT -------------------------------
void reconnectMQTT() {
  static unsigned long lastTry = 0;
  if (mqtt_server.length() == 0 || mqttClient.connected()) return;
  if (millis() - lastTry < 2000) return;
  lastTry = millis();
  if (mqttClient.connect("ESPClient", mqtt_user.c_str(), mqtt_pass.c_str())) {
    Serial.println("[MQTT] Connected!");
  } else {
    Serial.print("[MQTT] Failed, rc="); Serial.println(mqttClient.state());
  }
}

//------------------- Simulate Data ----------------------
float randFloat(float minV, float maxV) {
  return minV + (float)random(0, 10000) / 10000.0 * (maxV - minV);
}

void randomizeEnergyData(EnergyData &d) {
  d.voltage_a = randFloat(210, 250);
  d.voltage_b = randFloat(210, 250);
  d.voltage_c = randFloat(210, 250);
  d.current_a = randFloat(0, 40);
  d.current_b = randFloat(0, 40);
  d.current_c = randFloat(0, 40);
  d.active_power_a = randFloat(0, 10000);
  d.active_power_b = randFloat(0, 10000);
  d.active_power_c = randFloat(0, 10000);
  d.reactive_power_a = randFloat(0, 2000);
  d.reactive_power_b = randFloat(0, 2000);
  d.reactive_power_c = randFloat(0, 2000);
  d.apparent_power_a = randFloat(0, 12000);
  d.apparent_power_b = randFloat(0, 12000);
  d.apparent_power_c = randFloat(0, 12000);
  d.power_factor_a = randFloat(0.7, 1.0);
  d.power_factor_b = randFloat(0.7, 1.0);
  d.power_factor_c = randFloat(0.7, 1.0);
  d.frequency = randFloat(49, 51);
}

void publishEnergyData() {
  mqttClient.publish("energy_meter/voltage_a", String(energyData.voltage_a,2).c_str(), true);
  mqttClient.publish("energy_meter/voltage_b", String(energyData.voltage_b,2).c_str(), true);
  mqttClient.publish("energy_meter/voltage_c", String(energyData.voltage_c,2).c_str(), true);

  mqttClient.publish("energy_meter/current_a", String(energyData.current_a,2).c_str(), true);
  mqttClient.publish("energy_meter/current_b", String(energyData.current_b,2).c_str(), true);
  mqttClient.publish("energy_meter/current_c", String(energyData.current_c,2).c_str(), true);

  mqttClient.publish("energy_meter/active_power_a", String(energyData.active_power_a,2).c_str(), true);
  mqttClient.publish("energy_meter/active_power_b", String(energyData.active_power_b,2).c_str(), true);
  mqttClient.publish("energy_meter/active_power_c", String(energyData.active_power_c,2).c_str(), true);

  mqttClient.publish("energy_meter/reactive_power_a", String(energyData.reactive_power_a,2).c_str(), true);
  mqttClient.publish("energy_meter/reactive_power_b", String(energyData.reactive_power_b,2).c_str(), true);
  mqttClient.publish("energy_meter/reactive_power_c", String(energyData.reactive_power_c,2).c_str(), true);

  mqttClient.publish("energy_meter/apparent_power_a", String(energyData.apparent_power_a,2).c_str(), true);
  mqttClient.publish("energy_meter/apparent_power_b", String(energyData.apparent_power_b,2).c_str(), true);
  mqttClient.publish("energy_meter/apparent_power_c", String(energyData.apparent_power_c,2).c_str(), true);

  mqttClient.publish("energy_meter/power_factor_a", String(energyData.power_factor_a,2).c_str(), true);
  mqttClient.publish("energy_meter/power_factor_b", String(energyData.power_factor_b,2).c_str(), true);
  mqttClient.publish("energy_meter/power_factor_c", String(energyData.power_factor_c,2).c_str(), true);

  mqttClient.publish("energy_meter/frequency", String(energyData.frequency,2).c_str(), true);

  // Serial.println("[MQTT] Published all energy data!");
}

//------------------- Setup ------------------------------
void setup() {
  Serial.begin(115200);
  loadMQTTConfig();

  WiFiManager wm;
  wm.setTimeout(120);
  if (!wm.autoConnect("ESP-EnergyMeter", "12345678")) {
    WiFi.softAP("ESP-Config", "12345678");
    webServer.on("/", handleRoot);
    webServer.on("/realtime", handleRealtime);
    webServer.on("/api", handleApi);
    webServer.begin();
    while (true) { webServer.handleClient(); delay(5); }
  }

  webServer.on("/", handleRoot);
  webServer.on("/realtime", handleRealtime);
  webServer.on("/api", handleApi);
  webServer.on("/reset", handleReset);
  webServer.begin();

  if (mqtt_server.length() > 0) {
    espClient.setInsecure();
    mqttClient.setServer(mqtt_server.c_str(), mqtt_port);
  }

  randomSeed(micros());
  lastSend = millis();
}

//------------------- Loop -------------------------------
void loop() {
  mqttClient.loop();
  webServer.handleClient();

  if (WiFi.status() != WL_CONNECTED) {
    ESP.restart();
  }

  reconnectMQTT();

  if (millis() - lastSend > 2000) {
    randomizeEnergyData(energyData);
    publishEnergyData();
    lastSend = millis();
  }
}
