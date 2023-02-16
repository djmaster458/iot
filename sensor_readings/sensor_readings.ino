// Libs for interfacing with the BME 680
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"

// Conversion Factors for BME 680 and UV Sensor
#define SEALEVELPRESSURE_HPA (1013.25)
#define MILLIVOLTS_PER_VOLTS (1000)
#define UV_INDEX_FACTOR (0.1)
#define UV_SENSOR_GPIO_PIN (36)
#define LIGHT_SENSOR_GPIO_PIN (39)

Adafruit_BME680 bme;

// Print the current BME reading values to the serial monitor
void PrintBME680Data() {
  Serial.println("BME 680 Readings");

  Serial.printf("Temperature = %f *C\r\n", bme.temperature);

  Serial.printf("Pressure = %f hpa\r\n", bme.pressure / 100.0);

  Serial.printf("Humidity = %f \%\r\n", bme.humidity);

  Serial.printf("Gas = %f KOhms\r\n", bme.gas_resistance / 1000.0);

  Serial.printf("Altitude = %f m\r\n", bme.readAltitude(SEALEVELPRESSURE_HPA));

  Serial.println();
}

// Print the uv sensor data to the serial monitor
void PrintUVSensorData() {
  unsigned int uvAnalogValue = analogRead(UV_SENSOR_GPIO_PIN);
  Serial.printf("UV Sensor Analog Value: %.3f\r\n", uvAnalogValue);

  // Convert the uv analog value to voltage and then convert to index according to adafruit website
  float uvVoltage = (float) uvAnalogValue / 4095 * 3.3;
  float uvIndex = uvVoltage / UV_INDEX_FACTOR;

  Serial.printf("UV Sensor Voltage in Volts = %.3f V\r\n", uvVoltage);

  Serial.printf("UV Index = %.3f\r\n", uvIndex);
}

// Print the light sensor data to the serial monitor
void PrintLightSensorData() {
  unsigned int lightSensorAnalogValue = analogRead(LIGHT_SENSOR_GPIO_PIN);
  Serial.printf("Light Sensor Analog Value: %d\r\n", lightSensorAnalogValue);

  float lightVoltage = (float) lightSensorAnalogValue / 4095 * 3.3;
  float lightRatio = (float) lightVoltage / 3.3;

  Serial.printf("Light Sensor Voltage: %.3f V\r\n", lightVoltage);
  Serial.printf("Light Sensor Ratio Value: %.3f \r\n", lightRatio);
}

void setup() {

  // Setup serial monitor
  Serial.begin(115200);
  while (!Serial);

  // Init the BME 680 sensor
  if (!bme.begin()) {
    Serial.println("Could not find a valid BME680 sensor, check wiring!");
    while (1);
  }

  // Set up oversampling and filters to defaults and heater for gas sensor
  // Gas heater may affect results of temperature sensor, need to check documents
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320*C for 150 ms
}

void loop() {
  // Tell BME680 to begin measurement.
  unsigned long endTime = bme.beginReading();
  if (endTime == 0) {
    Serial.println("Failed to begin reading...");
    return;
  }

  Serial.print("Reading started at ");
  Serial.print(millis());
  Serial.print(" and will finish at ");
  Serial.println(endTime);

  delay(50);

  if (!bme.endReading()) {
    Serial.println("Failed to complete reading :(");
    return;
  }
  Serial.print("Reading completed at ");
  Serial.println(millis());

  PrintBME680Data();
  PrintUVSensorData();
  PrintLightSensorData();

  delay(5000);
}