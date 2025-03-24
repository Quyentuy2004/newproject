#include "wifiConfic.h"
#include <Arduino.h>
#include "dodht11.h"

void setup() {
  Serial.begin(115200);
  wifiConfig.begin();
   // Cấu hình Web Server web 1
  if(WiFi.status()==WL_CONNECTED){
    dhtConfig.begin();
  }
}
void loop() {
  wifiConfig.run();
  if(WiFi.status()==WL_CONNECTED){
   dhtConfig.run();
  }
}