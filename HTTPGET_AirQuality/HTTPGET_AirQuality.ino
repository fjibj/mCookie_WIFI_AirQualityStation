/*
  LICENSE: GPL v3 (http://www.gnu.org/licenses/gpl.html)
  版权所有: @老潘orz  wasdpkj@hotmail.com

  利用HTTPGET 获取空气重量的例程
  本例程展示了一种占用内存资源极低的字符串处理办法
  需要从这申请你自己的token: http://aqicn.org/data-platform/token/#/ 替换GETDATA的"demo"字段
  如果用软串口，建议9600波特率
*/

#include "userDef.h"
#include "msgComputer.h"

//--------------------------------------------
#include <U8glib.h>

U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE);

//--------------------------------------------
#include <ESP8266.h>

/**
**CoreUSB UART Port: [Serial1] [D0,D1]
**Core+ UART Port: [Serial1] [D2,D3]
**/
#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1284P__) || defined (__AVR_ATmega644P__) || defined(__AVR_ATmega128RFA1__)
#define EspSerial Serial1
#define UARTSPEED  115200
#endif

/**
**Core UART Port: [SoftSerial] [D2,D3]
**/
#if defined (__AVR_ATmega168__) || defined (__AVR_ATmega328__) || defined (__AVR_ATmega328P__)
#include <SoftwareSerial.h>
SoftwareSerial mySerial(2, 3); /* RX:D2, TX:D3 */

#define EspSerial mySerial
#define UARTSPEED  9600
#endif

ESP8266 wifi(&EspSerial);

float Data[12];
char updataDate[] = "2017-08-12 20:00:00";
boolean networkSta = false;
boolean dataSta = false;

void setup(void) {
  u8g.firstPage();
  do {
    u8g.setFont(u8g_font_timB10);

    u8g.setPrintPos(0, 16);
    u8g.print(F("Hello!"));
    u8g.setPrintPos(0, 32);
    u8g.print(F("AirQuality Station!"));
  } while ( u8g.nextPage() );

  Serial.begin(115200);
  //while (!Serial); // wait for Leonardo enumeration, others continue immediately
  Serial.print(F("setup begin\r\n"));
  delay(100);

  WifiInit(EspSerial, UARTSPEED);

  if (wifi.setOprToStationSoftAP()) {
    Serial.print(F("to station + softap ok\r\n"));
  } else {
    Serial.print(F("to station + softap err\r\n"));
    return;
  }

  Serial.println(wifi.getAPList().c_str());

  if (wifi.joinAP(SSID, PASSWORD)) {
    Serial.print(F("Join AP success\r\n"));

    Serial.print("IP:");
    Serial.println( wifi.getLocalIP().c_str());
  } else {
    Serial.print(F("Join AP failure\r\n"));
    return;
  }

  if (wifi.disableMUX()) {
    Serial.print(F("single ok\r\n"));
  } else {
    Serial.print(F("single err\r\n"));
  }


  msgInit(&EspSerial);  //初始化数据处理端口

  Serial.print(F("setup end\r\n"));
}

void loop(void) {
  Serial.print(F("\r\n"));
  //----------------------------------
  if (wifi.createTCP(WEBSITE, HOST_PORT)) {
    networkSta = true;
    Serial.print(F("create tcp ok\r\n"));
  } else {
    networkSta = false;
    Serial.print(F("create tcp err\r\n"));
  }

  //delay(500);
  wifi.sendFromFlash(GETDATA, sizeof(GETDATA));


  String buffer;
  if (available("+IPD", ":", 10000)) {
    Serial.print(F("available Data"));
    Serial.print(F("\r\n"));

    buffer = "{\"";
    dataSta = recvStringAt("iaqi", "}}}", buffer, 10000, BUFFER_CLR);
  }

  if (wifi.releaseTCP()) {
    Serial.print(F("release tcp ok\r\n"));
  } else {
    Serial.print(F("release tcp err\r\n"));
  }

  Serial.print(F("RAM buffer:"));
  Serial.print(buffer);
  Serial.print(F("\r\n"));

  //----------------------------------
  freeRam();

  //----------------------------------
  if (dataSta) {
    for (int a = 0; a < MSGNUM; a++) {
      Data[a] = findAndFilter(string_target, String(string_head[a]) + string_data, string_body, buffer, 16).toFloat();
      Serial.print(F("["));
      Serial.print(string_head[a]);
      Serial.print(F(":"));
      Serial.print(Data[a], 1);
      Serial.print(F("] "));
    }
    Serial.print(F("\r\n"));

    //.toCharArray(updataDate, 20);
    Serial.print(F("updataDate:"));
    //Serial.print(updataDate);
    Serial.print(findAndFilter(string_target, "\"time\":{\"s\":\"", "\"", buffer, 24));
    Serial.print(F("\r\n"));
  }
  else {
    Serial.print(F("data error\r\n"));
  }

  //----------------------------------
  u8g.firstPage();
  do {
    u8g.setFont(u8g_font_timB10);

    u8g.setPrintPos(0, 16);
    u8g.print(F("PM25: "));
    u8g.print(Data[7], 0);
    u8g.setPrintPos(0, 32);
    u8g.print(F("PM10: "));
    u8g.print(Data[6], 0);

    u8g.setPrintPos(80, 16);
    u8g.print(Data[9], 1);
    u8g.print(F("`C"));
    u8g.setPrintPos(80, 32);
    u8g.print(Data[2], 1);
    u8g.print(F("%"));

    u8g.setPrintPos(0, 64);
    u8g.print(updataDate);
  } while ( u8g.nextPage() );

  delay(1000);
}
