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

void setup() {
  Serial.begin(115200);

  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);

  // Clear the buffer
  display.clearDisplay();
}

void loop() {
  uint8_t rateValue;

  uint16_t heartrateValueLast = heartrate.getValue(heartratePin); // A0 foot sampled values
  rateValue = heartrate.getRate();   // Get heart rate value

  if (rateValue)  {
    Serial.println(rateValue);

    display.clearDisplay();
    display.setTextSize(2);             // Normal 1:1 pixel scale
    display.setTextColor(SSD1306_WHITE);        // Draw white text
    display.setCursor(90, 10);            // Start at top-left corner
    display.println(rateValue);
    display.setCursor(90, 30);            // Start at top-left corner
    display.println("BMP");
    display.display();
  }
  delay(20);
}
