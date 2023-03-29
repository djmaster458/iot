/* For complete details visit Microcontrollerslab.com */
#include <WiFi.h>
#include "time.h"

// Replace ssid and password with your network credentials
const char* ssid = "The Retreat 8";
const char* password = "";

// a string to hold NTP server to request epoch time
const char* ntpServer = "pool.ntp.org";

// Variable to hold current epoch timestamp
unsigned long Epoch_Time; 

// Get_Epoch_Time() Function that gets current epoch time
unsigned long Get_Epoch_Time() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    //Serial.println("Failed to obtain time");
    return(0);
  }
  time(&now);
  return now;
}

// Initialize WiFi
void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  initWiFi();
  configTime(0, 0, ntpServer);
}

void loop() {
  Epoch_Time = Get_Epoch_Time();
  Serial.print("Epoch Time: ");
  Serial.println(Epoch_Time);
  delay(1000);
}
