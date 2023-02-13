#include "DFRobot_Heartrate.h"
#define heartratePin A0

DFRobot_Heartrate heartrate(ANALOG_MODE);   // ANALOG_MODE or DIGITAL_MODE

void setup() {
  Serial.begin(115200);
}

void loop() {
  uint8_t rateValue;

  uint16_t heartrateValueLast = heartrate.getValue(heartratePin); // A1 foot sampled values
  rateValue = heartrate.getRate();   // Get heart rate value
  Serial.println(String(heartrateValueLast) + " ");
  if (rateValue)  {
    //    Serial.println(rateValue);
  }
  delay(20);
}
