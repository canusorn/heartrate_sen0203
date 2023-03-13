/*
   Library ที่ใช้
   DFRobot_Heartrate -> ติดตั้งจาก Zipfile https://github.com/DFRobot/DFRobot_Heartrate
   Adafruit_SH110X ค้นหาจาก library manager เลือก install all

   sensor
    signal(เขียว) -> A0
    VCC(แดง)     -> 5v
    GND(ดำ)     -> GND

   oled
    vcc -> 3v
    gnd -> gnd
    SDA -> D2
    SCL -> D1
*/

// leap year calculator expects year argument as years offset from 1970
#define LEAP_YEAR(Y)     ( ((1970+(Y))>0) && !((1970+(Y))%4) && ( ((1970+(Y))%100) || !((1970+(Y))%400) ) )

#include <ESP8266WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <WiFiUdp.h>
#include <Ticker.h>

Ticker timestamp;

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#include <TridentTD_LineNotify.h>

#define SSID        "wifi ssid"
#define PASSWORD    "wifi pass"
#define LINE_TOKEN  "token"

// 0-not detect  1-normal  2-lower 50 bpm  3-upper 120 bpm
uint8_t notifystate;

#include "DFRobot_Heartrate.h"

#define heartratePin A0
#define SAMPLEDISPLAY 80     // จำนวนจุดกราฟที่ต้องการแสดง

unsigned int localPort = 2390;      // local port to listen for UDP packets

DFRobot_Heartrate heartrate(ANALOG_MODE);   // ANALOG_MODE or DIGITAL_MODE

IPAddress timeServerIP; // time.nist.gov NTP server address
const char* ntpServerName = "time.nist.gov";
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
WiFiUDP udp;// A UDP instance to let us send and receive packets over UDP
uint32_t timestp, nexttimestpupdate;
String datetimeStr;
static  const uint8_t monthDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}; // API starts months from 1, this array starts from 0
unsigned long previousMillis = 0;

uint16_t  analogData[SAMPLEDISPLAY], indexdata, lastyaxis, maxYaxis, minYaxis;
uint8_t  bpm;

void setup() {
  Serial.begin(115200);

  display.begin(SCREEN_ADDRESS, true);

  // Clear the buffer
  display.clearDisplay();
  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(SH110X_WHITE);        // Draw white text
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

  udp.begin(localPort);
  NTPupdate(); // update time
  timestamp.attach(1, []() {
    timestp++;
    if (timestp > nexttimestpupdate) {
      NTPupdate();
    }
  });
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
    display.setTextColor(SH110X_WHITE);        // Draw white text
    display.setCursor(0, 0);

    stamptodatetime();
    display.print(datetimeStr);

    //    display.setCursor(65, 00);
    //    display.println("Heart beat");
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
      display.drawLine(5 + data,   20 + y_1,  6 + data, 20 + y_2, SH110X_WHITE);
    }

    display.display();

    indexdata = 0;
    maxYaxis = 0;
    minYaxis = 1000;
  }

}

void NTPupdate() {  //NTP update อัพเดทเวลาจากอินเทอร์เน็ต
  WiFi.hostByName(ntpServerName, timeServerIP);
  sendNTPpacket(timeServerIP);
  delay(1000);
  int cb = udp.parsePacket();
  if (!cb) {
    Serial.println("no packet yet");
  } else {
    // We've received a packet, read the data from it
    udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears;

    timestp = epoch + 7 * 3600;

    nexttimestpupdate = timestp + 3600;
  }
}

void sendNTPpacket(IPAddress& address) {
  Serial.println("Update NTP ...");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

void stamptodatetime() {
  uint8_t year;
  uint8_t month, monthLength;
  uint32_t time;
  unsigned long days;

  uint8_t tm_Second, tm_Minute, tm_Hour, tm_Wday, tm_Year, tm_Month, tm_Day;

  time = (uint32_t)timestp;
  tm_Second = time % 60;
  time /= 60; // now it is minutes
  tm_Minute = time % 60;
  time /= 60; // now it is hours
  tm_Hour = time % 24;
  time /= 24; // now it is days
  tm_Wday = ((time + 4) % 7) + 1;  // Sunday is day 1

  year = 0;
  days = 0;
  while ((unsigned)(days += (LEAP_YEAR(year) ? 366 : 365)) <= time) {
    year++;
  }
  tm_Year = year + 70; // year is offset from 1970
  tm_Year %= 100;

  days -= LEAP_YEAR(year) ? 366 : 365;
  time  -= days; // now it is days in this year, starting at 0

  days = 0;
  month = 0;
  monthLength = 0;
  for (month = 0; month < 12; month++) {
    if (month == 1) { // february
      if (LEAP_YEAR(year)) {
        monthLength = 29;
      } else {
        monthLength = 28;
      }
    } else {
      monthLength = monthDays[month];
    }

    if (time >= monthLength) {
      time -= monthLength;
    } else {
      break;
    }
  }
  tm_Month = month + 1;  // jan is month 1
  tm_Day = time + 1;     // day of month

  datetimeStr = String(tm_Day) + "/" + String(tm_Month) + "/" + String(tm_Year) + " " + String(tm_Hour) + ":";
  if (tm_Minute < 10) datetimeStr += "0";
  datetimeStr += String(tm_Minute);

  Serial.println(datetimeStr);
}
