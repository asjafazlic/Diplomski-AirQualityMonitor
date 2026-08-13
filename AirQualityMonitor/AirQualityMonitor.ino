#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <Adafruit_CCS811.h>
#include <PMS.h>

// --- Wi-Fi POSTAVKE ---
const char* ssid = "_SSID_";
const char* password = "_PASSWORD_";

// HiveMQ Cloud Broker
const char* mqtt_server = "broker.hivemq.com"; 
const int mqtt_port = 1883;

// Jedinstveni JSON topic za sve podatke
const char* topic_data = "moj_diplomski_2026/sensors/data";

// Termički offset za BME280 (°C)
constexpr float TEMP_OFFSET = 3.0f;

// Pinovi za I2C
#define I2C_SDA 21
#define I2C_SCL 22

// Objekti
WiFiClient espClient;
PubSubClient mqtt(espClient);

Adafruit_BME280 bme;
Adafruit_CCS811 ccs;

// Serijska komunikacija za PMS5003 (RX=16, TX=17)
HardwareSerial pmsSerial(2);
PMS pms(pmsSerial);
PMS::DATA pmsData;

// Mjerne varijable
float temperature = 0.0, humidity = 0.0, pressure = 0.0;
uint16_t co2 = 0, tvoc = 0, pm25 = 0;

// Varijable za EMA filter
float co2_filtered = 400.0;
float tvoc_filtered = 0.0;

// Tajmeri sa razfaziranjem
unsigned long lastRead = 0;
unsigned long lastPublish = 2500; // Pomjeranje slanja za 2.5s unaprijed

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== IoT Monitor Kvaliteta Zraka (JSON Mode) ===");

  setupSensors();
  setupWiFi();
  setupMQTT();

  Serial.println("\n=== Sistem Spreman - Cekam ocitavanja ===");
}

void setupSensors() {
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(100000); 

  // BME280 adresa 0x76
  if (!bme.begin(0x76)) { 
    Serial.println("Greska: BME280 nije pronadjen na adresi 0x76!");
  } else {
    Serial.println("BME280 inicijalizovan.");
  }

  // CCS811 adresa 0x5A
  if (!ccs.begin(0x5A)) {
    Serial.println("Greska: CCS811 nije pronadjen na adresi 0x5A!");
  } else {
    ccs.setDriveMode(CCS811_DRIVE_MODE_1SEC);
    Serial.println("CCS811 inicijalizovan.");
  }
  
  // PMS5003 u pasivnom režimu rada
  pmsSerial.begin(9600, SERIAL_8N1, 16, 17);
  pms.passiveMode(); 
}

void setupWiFi() {
  Serial.print("Spajanje na WiFi");
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi konektovan! IP: ");
  Serial.println(WiFi.localIP());
}

void setupMQTT() {
  mqtt.setServer(mqtt_server, mqtt_port);
}

void mqttReconnect() {
  if (!mqtt.connected()) {
    String clientId = "ESP32-Publisher-" + String(random(0xffff), HEX);
    if (mqtt.connect(clientId.c_str())) {
      Serial.println("Spojen na HiveMQ Cloud broker.");
    }
  }
}

void readSensors() {
  // Očitavanje svakih 2000 ms (2 sekunde)
  if (millis() - lastRead < 2000) return;
  lastRead = millis();

  // 1. Očitavanje BME280 uz primjenu termičkog offseta
  temperature = bme.readTemperature() - TEMP_OFFSET;
  humidity = bme.readHumidity();
  pressure = bme.readPressure() / 100.0F;

  // 2. Očitavanje CCS811
/*
 // SIROVI (RAW) PODACI CCS811
  if (ccs.available()) {
    if (!ccs.readData()) {
      co2 = ccs.geteCO2();
      tvoc = ccs.getTVOC();
    }
  } else if (ccs.checkError()) {
    Serial.println("CCS811: Detektovana interna greska!");
  }*/


  // OPCIJA 2: FILTRIRANI (EMA) PODACI
  if (ccs.available()) {
    if (!ccs.readData()) {
      uint16_t raw_co2 = ccs.geteCO2();
      uint16_t raw_tvoc = ccs.getTVOC();

      if (raw_co2 >= 400) {
        co2_filtered = (0.8f * co2_filtered) + (0.2f * raw_co2);
        tvoc_filtered = (0.8f * tvoc_filtered) + (0.2f * raw_tvoc);

        co2 = (uint16_t)co2_filtered;
        tvoc = (uint16_t)tvoc_filtered;
      }
    }
  } else if (ccs.checkError()) {
    Serial.println("CCS811: Detektovana interna greska!");
  }
  

  // 3. Pasivno očitavanje PMS5003
  pms.requestRead();
  if (pms.readUntil(pmsData)) {
    pm25 = pmsData.PM_AE_UG_2_5;
  }
}

void publishSensorsJSON() {
  // Slanje JSON paketa svakih 5000 ms (5 sekundi)
  if (millis() - lastPublish < 5000) return; 
  lastPublish = millis();

  if (!mqtt.connected()) return;

  // Kreiranje JSON stringa sa svim mjerenjima
  String jsonPayload = "{";
  jsonPayload += "\"temp\":" + String(temperature, 1) + ",";
  jsonPayload += "\"hum\":" + String(humidity, 1) + ",";
  jsonPayload += "\"pres\":" + String(pressure, 1) + ",";
  jsonPayload += "\"co2\":" + String(co2) + ",";
  jsonPayload += "\"tvoc\":" + String(tvoc) + ",";
  jsonPayload += "\"pm25\":" + String(pm25);
  jsonPayload += "}";

  // Slanje poruke na HiveMQ
  mqtt.publish(topic_data, jsonPayload.c_str());
  
  delay(10); // Pauza od 10ms da se napon stabilizuje na Serial Monitoru
  Serial.print("Poslan JSON paket: ");
  Serial.println(jsonPayload);
}

void loop() {
  if (!mqtt.connected()) {
    mqttReconnect();
  } else {
    mqtt.loop();
  }

  readSensors();
  publishSensorsJSON();
}