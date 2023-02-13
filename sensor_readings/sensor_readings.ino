// Libs for interfacing with the BME 680
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"

// Conversion Factors for BME 680 and UV Sensor
#define SEALEVELPRESSURE_HPA (1013.25)
#define MILLIVOLTS_PER_VOLTS (1000)
#define UV_INDEX_FACTOR (0.1)

Adafruit_BME680 bme;

void setup() {

  // Setup serial monitor
  Serial.begin(115200);
  while (!Serial);

  // Init the BME 680 sensor
  if (!bme.begin()) {
    Serial.println("Could not find a valid BME680 sensor, check wiring!"));
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
    Serial.println("Failed to begin reading..."));
    return;
  }

  // Serial.print("Reading started at "));
  // Serial.print(millis());
  // Serial.print(" and will finish at "));
  // Serial.println(endTime);

  delay(50);

  if (!bme.endReading()) {
    Serial.println("Failed to complete reading :(");
    return;
  }
  Serial.print("Reading completed at ");
  Serial.println(millis());

  Serial.print("Temperature = ");
  Serial.print(bme.temperature);
  Serial.println(" *C"));

  Serial.print("Pressure = ");
  Serial.print(bme.pressure / 100.0);
  Serial.println(" hPa");

  Serial.print("Humidity = "));
  Serial.print(bme.humidity);
  Serial.println(F(" %"));

  Serial.print("Gas = "));
  Serial.print(bme.gas_resistance / 1000.0);
  Serial.println(" KOhms");

  Serial.print("Altitude = ");
  Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA);
  Serial.println(" m"));

  Serial.println();

  // Read the ADC value from the UV sensor
  unsigned int uvAnalogValue = analogRead(2);
  Serial.println(uvAnalogValue);

  // Convert the uv analog value to voltage and then convert to index according to adafruit website
  float uvVoltage = (float) uvAnalogValue / 4095 * 3.3;
  float uvIndex = uvVoltage / UV_INDEX_FACTOR;

  Serial.print("UV Sensor Voltage in Volts = ");
  Serial.print"%.2f V", uvVoltage);
  Serial.println();

  Serial.print("UV Index = ");
  Serial.print(uvIndex);
  Serial.println();
  delay(2000);
}