// Replaces placeholder with variable values
//https://randomnerdtutorials.com/esp32-dht11-dht22-temperature-humidity-web-server-arduino-ide/
String processor(const String& var) {
  //Serial.println(var);
  if (var == "TIMEMEASSECOND") {
    return String(TimeMeasSecond);
  }
  //  else if(var == "HUMIDITY"){
  //    return readDHTHumidity();
  //  }
  return String();
}

class CaptiveRequestHandler : public AsyncWebHandler {
  public:
    CaptiveRequestHandler() {}
    virtual ~CaptiveRequestHandler() {}

    bool canHandle(AsyncWebServerRequest *request) {
      //request->addInterestingHeader("ANY");
      return true;
    }
    void handleRequest(AsyncWebServerRequest *request) {
      request->redirect("/");
      //      request->redirect("/webserial");
    }
};


void parseWebSerialData(char* tempChars) {      // split the data into its parts
  char * strtokIndx; // this is used by strtok() as an index

  strtokIndx = strtok(tempChars, ",");     // get the first part - the string
  strcpy(messageFromPC, strtokIndx); // copy it to messageFromPC
  Serial.print("Variable "); Serial.println(messageFromPC);

  strtokIndx = strtok(NULL, ","); // this continues where the previous call left off
  //  Serial.print("Integer "); Serial.println(strtokIndx);
  if (strtokIndx != NULL) {
    integerFromPC = atoi(strtokIndx);     // convert this part to an integer
    Serial.print("Integer "); Serial.println(integerFromPC);
  }
  else {
    Serial.println("No value recieved for variable (strtokIndx was NULL)");
  }
}

void printAcceptedVariablesWebSerial() {
  WebSerial.println("Enter data in this style: TimeMeasSecond,12");
  WebSerial.println("Accepted variables are: TimeMeasSecond and TimeMeasMin");
}

void understandWebSerialData() {  //comparing char with string
  if (strcmp(messageFromPC, "TimeMeasSecond") == 0) {
    Serial.println("TimeMeasSecond recieved correctly");
    TimeMeasSecond = integerFromPC;
    Serial.print("New TimeMeasSecond: ");  Serial.println(TimeMeasSecond);
    WebSerial.print("New TimeMeasSecond: ");  WebSerial.println(TimeMeasSecond);
    // Store the variable to the Preferences
    preferences.begin("ESP32Config", false);
    preferences.putInt("TimeMeasSecond", TimeMeasSecond);
    preferences.end();
  }
  else if (strcmp(messageFromPC, "TimeMeasMin") == 0) {
    Serial.println("TimeMeasMin recieved correctly");
    TimeMeasSecond = integerFromPC * 60;
    Serial.print("New TimeMeasSecond: ");  Serial.println(TimeMeasSecond);
    WebSerial.print("New TimeMeasSecond: ");  WebSerial.println(TimeMeasSecond);
    preferences.begin("ESP32Config", false);
    preferences.putInt("TimeMeasSecond", TimeMeasSecond);
    preferences.end();
  }
  else {
    Serial.println("Recieved an invalid variable");
    WebSerial.println("Recieved an invalid variable");
    printAcceptedVariablesWebSerial();
  }
}

/* Message callback of WebSerial */
void recvMsg(uint8_t *data, size_t len) {
  WebSerial.print("Received Data...   ");
  String d = "";
  for (int i = 0; i < len; i++) {
    d += char(data[i]);
  }
  WebSerial.println(d);
  Serial.print("Received Data: "); Serial.println(d);

  char tempChars[len + 1];
  d.toCharArray(tempChars, len + 1);
  Serial.print("tempChars: "); Serial.println(tempChars);
  parseWebSerialData(tempChars);
  understandWebSerialData();
}


String dirLFS(void) {
  //https://www.codeproject.com/Articles/5300719/ESP32-WEB-Server-with-Asynchronous-Technology-and
  String x = "<html><head>  <title>LittleFS File System</title><style>";
  x += "table, th, td {  border: 0px solid black;  }  th, td {  padding: 2px;  }";
  x += "td:nth-child(even), th:nth-child(even) {  background-color: #D6EEEE;  }";
  x += "</style>  </head><body><center>";
  x += "Printing File System in LittleFS<br><br>Careful, most links to SD filesystem!<br><br><table>";
  x += "<tr><th>Filename</th><th>Size (KB)</th></tr>";
  File root = LittleFS.open("/");
  if (!root) {
    return "Failed to open directory";
  }
  if (!root.isDirectory()) {
    return "Not a directory";
  }
  File file = root.openNextFile();
  while (file) {
//    x += "<tr><td>" + String(file.name());  //no link for LittleFS
    x += "<tr><td><a href=" + String(file.name());
    x += ">" + String(file.name()) + "</a>";
    if (file.isDirectory()) {
      x += "<td style='text-align:right'>DIR</td>";
    } else {
      x += "<td style='text-align:right'>" + String(file.size()/1024)+"</td>";
    }
    file = root.openNextFile();
  }
  x += "<tr><td>Occupied space (KB)<td style='text-align:right'>" + String(LittleFS.usedBytes()/1024) + "</td></tr>";
  x += "<tr><td>Total space (KB)<td style='text-align:right'>" + String(LittleFS.totalBytes()/1024)+ "</td></tr>";
  x += "</table><br><br><a href=/> Main webpage</a></body></html>";
  
  return x ;
}

String dirSDMMC(void) {
  //https://www.codeproject.com/Articles/5300719/ESP32-WEB-Server-with-Asynchronous-Technology-and
  String x = "<html><head>  <title>SD MMC File System</title><style>";
  x += "table, th, td {  border: 0px solid black;  }  th, td {  padding: 2px;  }";
  x += "td:nth-child(even), th:nth-child(even) {  background-color: #D6EEEE;  }";
  x += "</style>  </head><body><center>";
  x += "Printing File System in SD MMC<br><br><table>";
  x += "<tr><th>Filename</th><th>Size (KB)</th></tr>";
  File root = SD_MMC.open("/");
  if (!root) {
    return "Failed to open directory";
  }
  if (!root.isDirectory()) {
    return "Not a directory";
  }
  File file = root.openNextFile();
  while (file) {
    x += "<tr><td><a href=" + String(file.name());
    x += ">" + String(file.name()) + "</a>";
    if (file.isDirectory()) {
      x += "<td style='text-align:right'>DIR</td>";
    } else {
      x += "<td style='text-align:right'>" + String(file.size()/1024) +"</td>";
    }
    file = root.openNextFile();
  }
  // have problems to print SD total size, error of string function with int64, need to check
//  x += "<tr><td>Occupied space (MB)<td style='text-align:right'>" + String(SD_MMC.usedBytes() / (1024 * 1024)) + "</td></tr>";
//  x += "<tr><td>Total space (MB)<td style='text-align:right'>" + (SD_MMC.totalBytes() / (1024 * 1024));
  x += "</table><br><br><a href=/> Main webpage</a></body></html>";
  
  return x ;
}
