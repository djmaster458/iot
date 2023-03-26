#include <Adafruit_Sensor.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include "Adafruit_BME680.h"
#include "secrets.h"
#include "WiFi.h"

#define SEALEVELPRESSURE_HPA (1013.25)
#define MILLIVOLTS_PER_VOLTS (1000)
#define UV_INDEX_FACTOR (0.1)
#define UV_SENSOR_GPIO_PIN (36)
#define LIGHT_SENSOR_GPIO_PIN (39)

#define AWS_IOT_PUBLISH_TOPIC   "esp32/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp32/sub"

const unsigned long PUBLISH_INTERVAL_MSEC = 1 * 60 * 1000; //1 minutes or  60,000 ms

struct SensorData
{
  float temperature;
  float pressure;
  float humidity;
  float gasResistance;
  float lsVoltage;
  float lsRatio;
};

WiFiClientSecure net = WiFiClientSecure();
PubSubClient client(net);

Adafruit_BME680 bme;
SensorData cachedRecord;

unsigned long lastPublish = 0;

void ConnectAWS()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
 
  Serial.println("Connecting to Wi-Fi");
 
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
 
  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);
 
  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.setServer(AWS_IOT_ENDPOINT, 8883);
 
  // Create a message handler
  client.setCallback(ReceivedMessageHandler);
 
  Serial.println("Connecting to AWS IOT");
 
  while (!client.connect(THING_NAME))
  {
    Serial.print(".");
    delay(100);
  }
 
  if (!client.connected())
  {
    Serial.println("AWS IoT Timeout!");
    return;
  }
 
  Serial.println("AWS IoT Connected!");
}

void StoreSensorReadings()
{
  cachedRecord.temperature = bme.temperature;
  cachedRecord.pressure = bme.pressure / 100.0;
  cachedRecord.humidity = bme.humidity;
  cachedRecord.gasResistance = bme.gas_resistance / 1000.0;
  unsigned int lightSensorAnalogValue = analogRead(LIGHT_SENSOR_GPIO_PIN);
  cachedRecord.lsVoltage = (float) lightSensorAnalogValue / 4095 * 3.3;
  cachedRecord.lsRatio = (float) cachedRecord.lsVoltage / 3.3;
}

void SetupBMESensor()
{
  if (!bme.begin()) {
    Serial.println("Could not find BME 680 sensor. Check wiring.");
    while(1);
  }

  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320*C for 150 ms
}

void SerializeSensorData(char *payload, size_t payloadSize)
{
  StaticJsonDocument<200> doc;
  doc["temp"] = cachedRecord.temperature;
  doc["pressure"] = cachedRecord.pressure;
  doc["humidity"] = cachedRecord.humidity;
  doc["gas"] = cachedRecord.gasResistance;
  doc["lsVoltage"] = cachedRecord.lsVoltage;
  doc["lsRatio"] = cachedRecord.lsRatio;
  serializeJson(doc, payload, payloadSize);
}
 
void ReceivedMessageHandler(char* topic, byte* payload, unsigned int length)
{
  Serial.print("incoming: ");
  Serial.println(topic);
 
  StaticJsonDocument<200> doc;
  deserializeJson(doc, payload);
  const char* message = doc["message"];
  Serial.println(message);
}
 
void setup()
{
  Serial.begin(115200);
  SetupBMESensor();
  ConnectAWS();
}
 
void loop()
{
  client.loop();
  unsigned long now = millis();
  unsigned long timeSinceLastPublish = now - lastPublish;

  if(timeSinceLastPublish > PUBLISH_INTERVAL_MSEC) {
    lastPublish = now;

    unsigned long endTime = bme.beginReading();
    if(endTime == 0 || !bme.endReading())
    {
      Serial.println(F("Failed to complete reading"));
      return;
    }

    StoreSensorReadings();
    
    char payload[200];
    SerializeSensorData(payload, sizeof(payload));

    if(!client.publish(AWS_IOT_PUBLISH_TOPIC, payload))
    {
      Serial.println(F("Unable to publish reading, check wifi connection..."));
    }
  }
}