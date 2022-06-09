// Remote Wifi AP debugging and configuration


//------------ DNS server for Captive Portal --------------
//https://makexyz.fun/esp32-captive-portal/
#include <ESPmDNS.h>
#include <DNSServer.h>
const byte DNS_PORT = 53;
DNSServer dnsServer;

//------------ Wife Access Point --------------
#include <WiFi.h>
//IPAddress apIP(192,168,4,1); //doesnt work on mobiles...
IPAddress apIP(8, 8, 4, 4);
IPAddress subnet(255, 255, 255, 0);

//------------ Webserver and Webserial --------------
// ESPAsyncWebServer is selected because WebSerial uses this webserver library, not my favorite
//https://randomnerdtutorials.com/esp32-webserial-library/
#include <AsyncTCP.h>
#include "ESPAsyncWebServer.h"
#include <WebSerial.h>
AsyncWebServer server(80);

//------------ Preferences --------------
//https://randomnerdtutorials.com/esp32-save-data-permanently-preferences/
#include <Preferences.h>
Preferences preferences;

//------------ LittleFS --------------
#define FS_NO_GLOBALS //allow spiffs to coexist with SD card, define BEFORE including FS.h
#include <FS.h>
#include "LittleFS.h"
#include "SD_MMC.h"
//#define SPIFFS LITTLEFS
#define FORMAT_LITTLEFS_IF_FAILED true
#include "LittleFS_func.h" //auxiliary file for LittleFS and SDMMC?

const char* ssid = "FirebeetleESP32E"; // Your WiFi AP SSID
const char* password = "HolaHolaHola"; // Your WiFi Password

// variables to hold the parsed data
//const byte numChars = 32;
char messageFromPC[50] = {0};
int32_t integerFromPC = 0;
int32_t TimeMeasSecond = 0;

#include "Captive_Webserial_func.h"

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_AP);
  //  WiFi.softAP("ESP32Captive");
  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(apIP, apIP, subnet);
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

  if (!MDNS.begin("makexyzfun")) {
    Serial.println("Error starting mDNS");
    return;
  }


  // WebSerial is accessible at "<IP Address>/webserial" in browser
  WebSerial.begin(&server);
  /* Attach Message Callback */
  WebSerial.msgCallback(recvMsg);

  //server index webpage showing the value of variables
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    //    request->send_P(200, "text/html", index_html, processor);
    //https://iotespresso.com/esp32-captive-portal-fetching-html-using-littlefs/
    request->send(LittleFS, "/index.html", "text/html", false, processor);
  });
  //print LittleFS dir list
  server.on("/dirLFS", HTTP_GET, [](AsyncWebServerRequest * request) {
    //https://www.codeproject.com/Articles/5300719/ESP32-WEB-Server-with-Asynchronous-Technology-and
    AsyncResponseStream *response = request->beginResponseStream("text/html");
    response->print(dirLFS());
    request->send(response);
  });
  //print SD MMC dir list
  server.on("/dirSDMMC", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncResponseStream *response = request->beginResponseStream("text/html");
    response->print(dirSDMMC());
    request->send(response);
  });
  // specify IQS image and main csv file to read from LittleFS
  server.on("/IQSSE240_82.png", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(LittleFS, "/IQSSE240_82.png", "image/png");
  });
    server.on("/data.csv", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(LittleFS, "/data.csv", "text/plain");
  });

  //Important, default filesystem to search will be SD_MMC for all other files
  server.serveStatic("/", SD_MMC, "/");


  //important, Captive addHandler AFTER starting webserial and other web handling - before begin
  server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);//only when requested from AP
  server.begin();


  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  Serial.println("This demo expects 2 pieces of data in webserial - variable name and an integer within < >");
  Serial.println("Enter data in this style: TimeMeasSecond,12");
  Serial.println("Accepted variables are: TimeMeasSecond and TimeMeasMin");
  Serial.println();

  // ---------- Config Preferences -----------
  preferences.begin("ESP32Config", false);

  // Remove all preferences under the opened namespace
  //preferences.clear();
  //Obtained all saved variables in Preferences
  TimeMeasSecond = preferences.getInt("TimeMeasSecond", 0);
  // Close the Preferences
  preferences.end();
  Serial.print("TimeMeasSecond from Preferences: ");   Serial.println(TimeMeasSecond);

  // ---------- LittleFS -----------
  if (!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)) {
    Serial.println("LITTLEFS Mount Failed");
    return;
  }
  else {
    Serial.println("LITTLEFS started");
  }
  
  fs::File datafile = LittleFS.open("/data.csv", FILE_APPEND);
  // save the date, time, temperature and humidity, comma separated
  datafile.println("AAPL_date,AAPL_time,AAPL_Temp,AAPL_Hum"); //column headers
  datafile.close();
  readFile(LittleFS, "/data.csv");   //check that file was created succesfully

  Serial.println("----list 1----");
  listDir(LittleFS, "/", 1);
  // ----------- Initialize SD MMC --------------------------
  if (!SD_MMC.begin()) {
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD_MMC.cardType();

  if (cardType == CARD_NONE) {
    Serial.println("No SD_MMC card attached");
    return;
  }

  Serial.print("SD_MMC Card Type: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }
  uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
  Serial.printf("SD_MMC Card Size: %lluMB\n", cardSize);
  listDir(SD_MMC, "/", 0);
  Serial.printf("Total space: %lluMB\n", SD_MMC.totalBytes() / (1024 * 1024));
  Serial.printf("Used space: %lluMB\n", SD_MMC.usedBytes() / (1024 * 1024));
  //  SD_MMC.end(); //close SD properly, in order that no crashes later on
  //  Serial.println("Stoping SD MMC");
  Serial.println( "LittleFS & SD MMC Test complete" );


}

void loop() {
  dnsServer.processNextRequest();
}
