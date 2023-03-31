#include <ESPPerfectTime.h>
#include <sntp_pt.h>

#include <Adafruit_Sensor.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include "Adafruit_BME680.h"
#include "secrets.h"
#include "WiFi.h"
#include "time.h"

// Constants and Pins for Sensors
#define SEALEVELPRESSURE_HPA (1013.25)
#define MILLIVOLTS_PER_VOLTS (1000)
#define LIGHT_SENSOR_GPIO_PIN (39)
#define MSEC_PER_SEC (1000)
#define USEC_PER_MSEC (1000)
#define SEC_PER_MIN (60)
#define VREF (3.3)
#define ADC_RESOLUTION (4095)
#define MQTT_PAYLOAD_LENGTH (200)

// MQTT Topics, sub topic is for debugging purposes
#define AWS_IOT_PUBLISH_TOPIC   "esp32/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp32/sub"
#define DEVICE_ID "<your_device_id>"

const char *ntpServer = "pool.ntp.org";

const unsigned long PUBLISH_INTERVAL_MSEC = 1 * SEC_PER_MIN * MSEC_PER_SEC; //1 minutes or  60,000 ms

struct SensorData
{
  time_t tsSec;
  time_t tsMsec;
  float temperature;
  float pressure;
  float humidity;
  float gasResistance;
  float lsVoltage;
  float lsRatio;
};

// Globals for Network Client and mqtt client using AWS credentials in setup
WiFiClientSecure net = WiFiClientSecure();
PubSubClient client(net);

Adafruit_BME680 bme;
SensorData cachedRecord;

unsigned long lastPublish = 0;

// Connects the user to AWS IoT Core for MQTT messaging, use provided Thing credentials from AWS in secrets.h
void ConnectAWS()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
 
  Serial.println(F("Connecting to Wi-Fi"));
 
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
 
  Serial.println(F("Connecting to AWS IOT"));
 
  while (!client.connect(THING_NAME))
  {
    Serial.print(".");
    delay(100);
  }
 
  if (!client.connected())
  {
    Serial.println(F("AWS IoT Timeout!"));
    return;
  }
 
  Serial.println(F("AWS IoT Connected!"));
}

// Uses the ESPPerfectTime Lib to get a microsecond resolution timestamp
void StoreTimestamps()
{
  struct timeval tv;

  if(pftime::gettimeofday(&tv, nullptr) != 0)
  {
    tv.tv_sec = 0;
    tv.tv_usec = 0;
  }

  cachedRecord.tsSec = tv.tv_sec;
  cachedRecord.tsMsec = tv.tv_usec / USEC_PER_MSEC;
}

//Moves data from sensors into temporary struct
void StoreSensorReadings()
{
  cachedRecord.temperature = bme.temperature;
  cachedRecord.pressure = bme.pressure / 100.0;
  cachedRecord.humidity = bme.humidity;
  cachedRecord.gasResistance = bme.gas_resistance / 1000.0;
  unsigned int lightSensorAnalogValue = analogRead(LIGHT_SENSOR_GPIO_PIN);
  cachedRecord.lsVoltage = (float) lightSensorAnalogValue / ADC_RESOLUTION * VREF;
  cachedRecord.lsRatio = (float) cachedRecord.lsVoltage / VREF;
}

// Init the BME Sensor, and set the oversampling for precision (defaults used). and warmup for gas heater
// Should be done before Wifi Init
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

// Serialize sensor data and timestamps into payload as JSON format
void SerializeSensorData(char *payload, size_t payloadSize)
{
  StaticJsonDocument<MQTT_PAYLOAD_LENGTH> doc;
  doc["id"] = DEVICE_ID;
  doc["temp"] = cachedRecord.temperature;
  doc["pressure"] = cachedRecord.pressure;
  doc["humidity"] = cachedRecord.humidity;
  doc["gas"] = cachedRecord.gasResistance;
  doc["lsVoltage"] = cachedRecord.lsVoltage;
  doc["lsRatio"] = cachedRecord.lsRatio;
  doc["tsSec"] = cachedRecord.tsSec;
  doc["tsMsec"] = cachedRecord.tsMsec;
  serializeJson(doc, payload, payloadSize);
}

// Callback for subscription messages, for debugging or diagnostics
void ReceivedMessageHandler(char* topic, byte* payload, unsigned int length)
{
  Serial.print("incoming: ");
  Serial.println(topic);
 
  StaticJsonDocument<MQTT_PAYLOAD_LENGTH> doc;
  deserializeJson(doc, payload);
  const char* message = doc["message"];
  Serial.println(message);
}

void setup()
{
  Serial.begin(115200);
  SetupBMESensor();
  ConnectAWS();

  // Config the time library to GMT / UTC time
  pftime::configTzTime(PSTR("GMT"), ntpServer);
}
 
void loop()
{
  // loop for messages arrived and stay alives
  client.loop();
  unsigned long now = millis();
  unsigned long timeSinceLastPublish = now - lastPublish;

  // Attempt Publish every Interval
  if(timeSinceLastPublish > PUBLISH_INTERVAL_MSEC) {
    lastPublish = now;

    // Synchronous reading of bme sensor, should be very quick, sub-ms
    unsigned long endTime = bme.beginReading();
    if(endTime == 0 || !bme.endReading())
    {
      Serial.println(F("Failed to complete reading"));
      return;
    }

    // Record data and timestamps into a payload and publish to the AWS Topic
    StoreSensorReadings();
    StoreTimestamps();
    
    char payload[MQTT_PAYLOAD_LENGTH];
    SerializeSensorData(payload, sizeof(payload));
    Serial.println(payload);

    Serial.println("Publishing...");
    if(!client.publish(AWS_IOT_PUBLISH_TOPIC, payload))
    {
      Serial.println(F("Unable to publish reading, check wifi connection..."));
    }
  }
}