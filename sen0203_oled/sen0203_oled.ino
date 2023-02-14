#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#include "DFRobot_Heartrate.h"
#define heartratePin A0

DFRobot_Heartrate heartrate(ANALOG_MODE);   // ANALOG_MODE or DIGITAL_MODE

unsigned long previousMillis = 0;

uint16_t  analogData[70], indexdata, lastyaxis, maxYaxis, minYaxis;
uint8_t  bpm;


void setup() {
  Serial.begin(115200);

  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);

  // Clear the buffer
  display.clearDisplay();
}

void loop() {

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= 20) {
    previousMillis = currentMillis;

    uint16_t heartrateValueLast = heartrate.getValue(heartratePin); // A0 foot sampled values
    uint8_t rateValue = heartrate.getRate();   // Get heart rate value
    //    Serial.println(heartrateValueLast);
    analogData[indexdata] = heartrateValueLast;
    indexdata ++;

    if (heartrateValueLast > maxYaxis) {
      maxYaxis = heartrateValueLast;
    }
    if (heartrateValueLast < minYaxis) {
      minYaxis = heartrateValueLast;
    }

    if (rateValue)  {
      bpm = rateValue;
      //      Serial.println(rateValue);
    }
  }

  if (indexdata >= 70) {

    display.clearDisplay();
    display.setTextSize(2);             // Normal 1:1 pixel scale
    display.setTextColor(SSD1306_WHITE);        // Draw white text
    display.setCursor(90, 10);
    display.println(bpm);
    display.setCursor(90, 30);
    display.println("BPM");

    uint16_t Yaxis = maxYaxis - minYaxis;

    float y_1, y_2;
    for (int data = 1; data < 70; data++) {
      
      y_1 = analogData[data - 1] - minYaxis;
      y_1 /= Yaxis;
      y_1 *= 34;
      y_1 =  int(34 - y_1);


      y_2 =  int(34 - ((analogData[data]  - minYaxis ) / Yaxis * 34 )) ;
      display.drawLine(10+ data,   20 + y_1,  12 + data, 20 + y_2, SSD1306_WHITE);
    }
    display.display();
    indexdata = 0;
    maxYaxis = 600;
    minYaxis = 300;
  }

}
