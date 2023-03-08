#include "EspMQTTClient.h"

// Libs for interfacing with the BME 680
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"

#include <ArduinoJson.h>

#include "secrets.h"
// Conversion Factors for BME 680 and UV Sensor
#define SEALEVELPRESSURE_HPA (1013.25)
#define MILLIVOLTS_PER_VOLTS (1000)
#define UV_INDEX_FACTOR (0.1)
#define UV_SENSOR_GPIO_PIN (36)
#define LIGHT_SENSOR_GPIO_PIN (39)

struct LightSensorData
{
  float lightVoltage;
  float lightRatio;
};

LightSensorData lightSensor;
Adafruit_BME680 bme;

EspMQTTClient client(
  WIFI_SSID,
  WIFI_PASS,
  BROKER_IP,  // MQTT Broker server ip
  CLIENT_NAME,     // Client name that uniquely identify your device
  1883              // The MQTT port, default to 1883. this line can be omitted
);

void SerializeSensorData(char *payload, size_t payloadSize)
{
  StaticJsonDocument<200> doc;
  doc["temp"] = bme.temperature;
  doc["pressure"] = bme.pressure / 100.0;
  doc["humidity"] = bme.humidity;
  doc["gas"] = bme.gas_resistance / 1000.0;
  doc["altitude"] = bme.readAltitude(SEALEVELPRESSURE_HPA);
  doc["lsVoltage"] = lightSensor.lightVoltage;
  doc["lsRatio"] = lightSensor.lightRatio;
  serializeJson(doc, payload, payloadSize);
}

void PrintBME680Data() {
  Serial.println("BME 680 Readings");
  Serial.printf("Temperature = %f *C\r\n", bme.temperature);
  Serial.printf("Pressure = %f hpa\r\n", bme.pressure / 100.0);
  Serial.printf("Humidity = %f \%\r\n", bme.humidity);
  Serial.printf("Gas = %f KOhms\r\n", bme.gas_resistance / 1000.0);
  Serial.printf("Altitude = %f m\r\n", bme.readAltitude(SEALEVELPRESSURE_HPA));
  Serial.println();
}

void PrintLightSensorData() {
  unsigned int lightSensorAnalogValue = analogRead(LIGHT_SENSOR_GPIO_PIN);
  Serial.printf("Light Sensor Analog Value: %d\r\n", lightSensorAnalogValue);

  lightSensor.lightVoltage = (float) lightSensorAnalogValue / 4095 * 3.3;
  lightSensor.lightRatio = (float) lightSensor.lightVoltage / 3.3;

  Serial.printf("Light Sensor Voltage: %.3f V\r\n", lightSensor.lightVoltage);
  Serial.printf("Light Sensor Ratio Value: %.3f \r\n", lightSensor.lightRatio);
}

void PrintUVSensorData() {
  unsigned int uvAnalogValue = analogRead(UV_SENSOR_GPIO_PIN);
  Serial.printf("UV Sensor Analog Value: %.3f\r\n", uvAnalogValue);

  // Convert the uv analog value to voltage and then convert to index according to adafruit website
  float uvVoltage = (float) uvAnalogValue / 4095 * 3.3;
  float uvIndex = uvVoltage / UV_INDEX_FACTOR;

  Serial.printf("UV Sensor Voltage in Volts = %.3f V\r\n", uvVoltage);

  Serial.printf("UV Index = %.3f\r\n", uvIndex);
}

void setup()
{
  Serial.begin(115200);
  if (!bme.begin()) {
    Serial.println("Could not find BME 680 sensor. Check wiring.");
    while(1);
  }

  //doc["device"] = "esp32_derek";
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320*C for 150 ms

  // Optional functionalities of EspMQTTClient
  client.enableDebuggingMessages(); // Enable debugging messages sent to serial output
  client.enableHTTPWebUpdater(); // Enable the web updater. User and password default to values of MQTTUsername and MQTTPassword. These can be overridded with enableHTTPWebUpdater("user", "password").
  client.enableOTA(); // Enable OTA (Over The Air) updates. Password defaults to MQTTPassword. Port is the default OTA port. Can be overridden with enableOTA("password", port).
  client.enableLastWillMessage("TestClient/lastwill", "I am going offline");  // You can activate the retain flag by setting the third parameter to true
}

// This function is called once everything is connected (Wifi and MQTT)
// WARNING : YOU MUST IMPLEMENT IT IF YOU USE EspMQTTClient
void onConnectionEstablished()
{
  Serial.println("Connection Established");

  client.subscribe("esp32", [](const String & payload) {
    Serial.println("Received Message on ESP32. Data:");
    Serial.println(payload);
  });
}

void loop()
{
  client.loop();

  int readingStatus = bme.remainingReadingMillis();

  if(readingStatus == Adafruit_BME680::reading_not_started) {
    Serial.println("Starting BME680 reading...");
    bme.beginReading();
  }
  else if(readingStatus == Adafruit_BME680::reading_complete) {
    PrintBME680Data();
    PrintLightSensorData();
    char payload[200];

    SerializeSensorData(payload, sizeof(payload));

    if (client.isConnected())
    {
      client.publish("esp32", payload);
    }
  }
}
