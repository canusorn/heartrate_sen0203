/*
   Library ที่ใช้
   DFRobot_Heartrate -> ติดตั้งจาก Zipfile https://github.com/DFRobot/DFRobot_Heartrate
   Adafruit_SSD1306 ค้นหาจาก library manager เลือก install all
*/



#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#include <TridentTD_LineNotify.h>

#define SSID        "Apichat_2.4G_"
#define PASSWORD    "4507240701"
#define LINE_TOKEN  "7m68381D2LS8ByeflY4rVEf9pPEXMXllsuFRNGBTFfG"

// 0-not detect  1-normal  2-lower 50 bpm  3-upper 120 bpm
uint8_t notifystate;

#include "DFRobot_Heartrate.h"

#define heartratePin A0
#define SAMPLEDISPLAY 80     // จำนวนจุดกราฟที่ต้องการแสดง

DFRobot_Heartrate heartrate(ANALOG_MODE);   // ANALOG_MODE or DIGITAL_MODE

unsigned long previousMillis = 0;

uint16_t  analogData[SAMPLEDISPLAY], indexdata, lastyaxis, maxYaxis, minYaxis;
uint8_t  bpm;

void setup() {
  Serial.begin(115200);

  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);

  // Clear the buffer
  display.clearDisplay();
  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setCursor(0, 0);
  display.print("Connecting Wifi");

  WiFi.begin(SSID, PASSWORD);
  Serial.printf("WiFi connecting to %s\n",  SSID);
  while (WiFi.status() != WL_CONNECTED) {

    display.print(".");
    display.display();

    Serial.print(".");
    delay(400);
  }
  Serial.printf("\nWiFi connected\nIP : ");
  Serial.println(WiFi.localIP());

  // กำหนด Line Token
  LINE.setToken(LINE_TOKEN);

}

void loop() {

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= 20) {
    previousMillis = currentMillis;

    // อ่านค่าจากเซนเซอร์
    uint16_t heartrateValueLast = heartrate.getValue(heartratePin); // A0 foot sampled values
    uint8_t rateValue = heartrate.getRate();   // Get heart rate value
    //    Serial.println("analog:" + (String)heartrateValueLast);
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
      Serial.println(rateValue);


      // แจ้งเตือนผ่านไลน์
      if (rateValue > 0 && rateValue < 50) {  // ชีพจรต่ำ
        if (notifystate != 2) {
          notifystate = 2;
          LINE.notify("Heart rate : " + String(rateValue) + ", มีชีพจรต่ำ");
        }
      }
      else if (rateValue > 120) {  // ชีพจรสูง
        if (notifystate != 3) {
          notifystate = 3;
          LINE.notify("Heart rate : " + String(rateValue) + ", มีชีพจรสูงเกินไป");
        }
      } else if (rateValue > 50 && rateValue < 120) {  // ชีพจรปกติ
        if (notifystate != 1) {
          notifystate = 1;
          LINE.notify("Heart rate : " + String(rateValue) + ", ปกติ");
        }
      }
    }
  }



  // แสดงค่าผ่านจอ oled
  if (indexdata >= SAMPLEDISPLAY) {

    display.clearDisplay();
    display.setTextSize(1);             // Normal 1:1 pixel scale
    display.setTextColor(SSD1306_WHITE);        // Draw white text
    display.setCursor(65, 00);
    display.println("Heart beat");
    display.setTextSize(2);
    display.setCursor(90, 17);
    if (minYaxis < 200) {
      display.println(" -");
    } else {
      display.println(bpm);
    }
    display.setCursor(90, 40);
    display.println("BPM");

    // คำนวณและแสดงค่ากราฟ
    uint16_t Yaxis = maxYaxis - minYaxis;
    //    Serial.println("yaxis:" + String(Yaxis));
    float y_1, y_2;
    y_2 = analogData[0] - minYaxis;
    y_2 /= Yaxis;
    y_2 *= 40;
    y_2 =  int(40 - y_2);

    for (int data = 1; data < SAMPLEDISPLAY; data++) {
      y_1 = y_2;
      y_2 = analogData[data] - minYaxis;
      y_2 /= Yaxis;
      y_2 *= 40;
      y_2 =  int(40 - y_2);

      // display graph
      display.drawLine(5 + data,   20 + y_1,  6 + data, 20 + y_2, SSD1306_WHITE);
    }

    display.display();

    indexdata = 0;
    maxYaxis = 0;
    minYaxis = 1000;
  }

}
