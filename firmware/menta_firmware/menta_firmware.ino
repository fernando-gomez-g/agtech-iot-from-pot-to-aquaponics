// AgTech IoT: From a Pot to Aquaponics - Level 1 firmware (mint plant)
//
// Reads a capacitive soil moisture sensor and a DHT22, and publishes the
// readings to AWS IoT Core over MQTT with mutual TLS once per minute.
//
// Secrets (Wi-Fi, endpoint, certificates) live in secrets.h, which is NOT
// committed. Copy secrets.example.h to secrets.h and fill in your values.

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "DHT.h"
#include "secrets.h"

// ---------- Sensors ----------
const int SOIL_PIN = 32;          // ADC1 pin, safe to use with Wi-Fi active
const int SOIL_DRY_RAW = 3200;    // raw ADC value measured in air  -> 0%
const int SOIL_WET_RAW = 900;     // raw ADC value measured in water -> 100%

#define DHT_PIN 4
#define DHT_TYPE DHT22
DHT dht(DHT_PIN, DHT_TYPE);

// ---------- MQTT ----------
const char* MQTT_TOPIC = "menta/sensores";
const unsigned long PUBLISH_INTERVAL_MS = 60000;  // one reading per minute

WiFiClientSecure net;
PubSubClient client(net);

void connectWiFi() {
  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void connectAWS() {
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);
  client.setServer(AWS_IOT_ENDPOINT, 8883);

  Serial.print("Connecting to AWS IoT");
  while (!client.connect(THING_NAME)) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println(" connected");
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  connectWiFi();
  connectAWS();
}

void loop() {
  if (!client.connected()) {
    connectAWS();
  }
  client.loop();

  // Soil moisture: map raw ADC value to a relative 0-100% scale and clamp.
  int soilRaw = analogRead(SOIL_PIN);
  int soilPct = map(soilRaw, SOIL_DRY_RAW, SOIL_WET_RAW, 0, 100);
  if (soilPct < 0)   soilPct = 0;
  if (soilPct > 100) soilPct = 100;

  float temperature = dht.readTemperature();
  float airHumidity = dht.readHumidity();

  // The DHT22 occasionally fails to read (especially right after boot).
  if (isnan(temperature) || isnan(airHumidity)) {
    Serial.println("DHT22 read error, retrying...");
    delay(2000);
    return;
  }

  // Field names are used by the downstream pipeline (IoT rules, Athena table,
  // CloudWatch metrics). Do not rename them without updating those.
  String message = "{";
  message += "\"humedad_suelo\":" + String(soilPct) + ",";
  message += "\"temperatura\":"   + String(temperature) + ",";
  message += "\"humedad_aire\":"  + String(airHumidity);
  message += "}";

  client.publish(MQTT_TOPIC, message.c_str());
  Serial.print("Published: ");
  Serial.println(message);

  delay(PUBLISH_INTERVAL_MS);
}
