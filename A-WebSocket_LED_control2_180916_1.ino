#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
//#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <WebSocketsServer.h>

ESP8266WiFiMulti wifiMulti;       // Create an instance of the ESP8266WiFiMulti class, called 'wifiMulti'

ESP8266WebServer server(80);       // create a web server on port 80
WebSocketsServer webSocket(81);    // create a websocket server on port 81

File fsUploadFile;                                    // a File variable to temporarily store the received file

// wifi setup
//Static IP address configuration
IPAddress staticIP(192, 168, 1, 179); //ESP static ip
IPAddress gateway(192, 168, 1, 1);   //IP Address of your WiFi Router (Gateway)
IPAddress subnet(255, 255, 255, 0);  //Subnet mask
IPAddress dns(192, 1, 1, 1);  //DNS
const char *deviceName = "soloesp1";
//For static AP connection STA
const char *ssid = "solo2"; // The name of the Wi-Fi network that will be created
const char *password = "risky00699";   // The password required to connect to it, leave blank for an open network
//For SOFT_AP
const char *ssid2 = "ESP8266 Access Point"; // The name of the Wi-Fi network that will be created
const char *password2 = "risky0069";   // The password required to connect to it, leave blank for an open network

const char *OTAName = "ESP8266";           // A name and a password for the OTA service
const char *OTAPassword = "risky0069";

#define LED_RED     15            // specify the pins with an RGB LED connected
#define LED_GREEN   12
#define LED_BLUE    13

int length;         //Used to hold data length

//packet vars
//receive vars for chars
#define SOP '<'
#define EOP '>'

bool started = false;
bool ended = false;

char inData[20];//13 for vars + SOP +EOP +NULL = 16 total (if this is set for < 16 < you may see chars cut-off/missing or connections repudiated
byte indexA;


const char* mdnsName = "esp8266"; // Domain name for the mDNS responder

/*__________________________________________________________SETUP__________________________________________________________*/

void setup() {
  pinMode(LED_RED, OUTPUT);    // the pins with LEDs connected are outputs
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);

  Serial.begin(115200);        // Start the Serial communication to send messages to the computer
  delay(10);
  Serial.println("\r\n");

  startWiFi();                 // Start a Wi-Fi access point, and try to connect to some given access points. Then wait for either an AP or STA connection

  //startOTA();                  // Start the OTA service

  startSPIFFS();               // Start the SPIFFS and list all contents

  startWebSocket();            // Start a WebSocket server

  startMDNS();                 // Start the mDNS responder

  startServer();               // Start a HTTP server with a file read handler and an upload handler

}

/*__________________________________________________________LOOP__________________________________________________________*/

bool rainbow = false;             // The rainbow effect is turned off on startup

unsigned long prevMillis = millis();
int hue = 0;

void loop() {
  webSocket.loop();                           // constantly check for websocket events
  server.handleClient();                      // run the server
//  ArduinoOTA.handle();                        // listen for OTA events

}

/*__________________________________________________________SETUP_FUNCTIONS__________________________________________________________*/

void startWiFi() { // Start a Wi-Fi access point, and try to connect to some given access points. Then wait for either an AP or STA connection
  // WiFi.softAP(ssid2, password2);             // Start the access point
  
  WiFi.hostname(deviceName);      // DHCP Hostname (useful for finding device for static lease)
  WiFi.config(staticIP, subnet, gateway, dns);
  server.serveStatic("/fp", SPIFFS, "/flatpickr3.html");
  server.serveStatic("/datalog.txt", SPIFFS, "/datalog.txt");
  WiFi.begin(ssid, password);

  WiFi.mode(WIFI_STA); //WiFi mode station (connect to wifi router only
  Serial.println("Connecting");
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\r\n");
  if (WiFi.softAPgetStationNum() == 0) {     // If the ESP is connected to an AP
    Serial.print("Connected to ");
    Serial.println(WiFi.SSID());             // Tell us what network we're connected to
    Serial.print("IP address:\t");
    Serial.print(WiFi.localIP());            // Send the IP address of the ESP8266 to the computer
  } else {                                   // If a station is connected to the ESP SoftAP
    Serial.print("Station connected to ESP8266 AP");
  }
  Serial.println("\r\n");
}//end startWiFi

/*
void startOTA() { // Start the OTA service
  ArduinoOTA.setHostname(OTAName);
  // ArduinoOTA.setPassword(OTAPassword);

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
    digitalWrite(LED_RED, 0);    // turn off the LEDs
    digitalWrite(LED_GREEN, 0);
    digitalWrite(LED_BLUE, 0);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\r\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("OTA ready\r\n");
}//end startOTA
*/

void startSPIFFS() { // Start the SPIFFS and list all contents
  SPIFFS.begin();                             // Start the SPI Flash File System (SPIFFS)
  Serial.println("SPIFFS started. Contents:");
  {
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {                      // List the file system contents
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      Serial.printf("\tFS File: %s, size: %s\r\n", fileName.c_str(), formatBytes(fileSize).c_str());
    }
    Serial.printf("\n");
  }
}//end startSPIFFS

void startWebSocket() { // Start a WebSocket server
  webSocket.begin();                          // start the websocket server
  webSocket.onEvent(webSocketEvent);          // if there's an incomming websocket message, go to function 'webSocketEvent'
  Serial.println("WebSocket server started.");
}//end startWebSocket

void startMDNS() { // Start the mDNS responder
  MDNS.begin(mdnsName);                        // start the multicast domain name server
  Serial.print("mDNS responder started: http://");
  Serial.print(mdnsName);
  Serial.println(".local");
} //end startMDNS

void startServer() { // Start a HTTP server with a file read handler and an upload handler
  server.on("/edit.html",  HTTP_POST, []() {  // If a POST request is sent to the /edit.html address,
    server.send(200, "text/plain", "");
  }, handleFileUpload);                       // go to 'handleFileUpload'

  server.onNotFound(handleNotFound);          // if someone requests any other file or page, go to function 'handleNotFound'
  // and check if the file exists

  server.begin();                             // start the HTTP server
  Serial.println("HTTP server started.");
} //end startServer

/*__________________________________________________________SERVER_HANDLERS__________________________________________________________*/

void handleNotFound() { // if the requested file or page doesn't exist, return a 404 not found error
  if (!handleFileRead(server.uri())) {        // check if the file exists in the flash memory (SPIFFS), if so, send it
    server.send(404, "text/plain", "404: File Not Found");
  }
}//end handleNotFound

bool handleFileRead(String path) { // send the right file to the client (if it exists)
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.html";          // If a folder is requested, send the index file
  String contentType = getContentType(path);             // Get the MIME type
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) { // If the file exists, either as a compressed archive, or normal
    if (SPIFFS.exists(pathWithGz))                         // If there's a compressed version available
      path += ".gz";                                         // Use the compressed verion
    File file = SPIFFS.open(path, "r");                    // Open the file
    size_t sent = server.streamFile(file, contentType);    // Send it to the client
    file.close();                                          // Close the file again
    Serial.println(String("\tSent file: ") + path);
    return true;
  }
  Serial.println(String("\tFile Not Found: ") + path);   // If the file doesn't exist, return false
  return false;
}//end handleFileRead


void handleFileUpload() { // upload a new file to the SPIFFS
  HTTPUpload& upload = server.upload();
  String path;
  if (upload.status == UPLOAD_FILE_START) {
    path = upload.filename;
    if (!path.startsWith("/")) path = "/" + path;
    if (!path.endsWith(".gz")) {                         // The file server always prefers a compressed version of a file
      String pathWithGz = path + ".gz";                  // So if an uploaded file is not compressed, the existing compressed
      if (SPIFFS.exists(pathWithGz))                     // version of that file must be deleted (if it exists)
        SPIFFS.remove(pathWithGz);
    }
    Serial.print("handleFileUpload Name: "); Serial.println(path);
    fsUploadFile = SPIFFS.open(path, "w");            // Open the file for writing in SPIFFS (create if it doesn't exist)
    path = String();
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize); // Write the received bytes to the file
  } else if (upload.status == UPLOAD_FILE_END) {
    if (fsUploadFile) {                                   // If the file was successfully created
      fsUploadFile.close();                               // Close the file again
      Serial.print("handleFileUpload Size: "); Serial.println(upload.totalSize);
      server.sendHeader("Location", "/success.html");     // Redirect the client to the success page
      server.send(303);
    } else {
      server.send(500, "text/plain", "500: couldn't create file");
    }
  }
}//end handleFileUpload

//changed tis line
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) { // When a WebSocket message is received
 
  
  switch (type) {
    case WStype_DISCONNECTED:             // if the websocket is disconnected
      //Serial.printf("[%u] Disconnected!\n", num);
       Serial.printf("[%u] Disconnected!\n");
      break;
    case WStype_CONNECTED: {              // if a new websocket connection is established
        IPAddress ip = webSocket.remoteIP(num);
//        IPAddress ip = webSocket.remoteIP();
        //changed this
       Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        //with this
     //   Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", ip[0], ip[1], ip[2], ip[3], payload);
        rainbow = false;                  // Turn rainbow off when a new connection is established
      }
      break;
    case WStype_TEXT:                     // if new text data is received
    //for(int i = 0; i < length; i++) Serial.print((char) payload[i]);
    char output[128] ={};
    for(int i = 0; i < length; i++) output[i] = payload[i] + '0'; Serial.println("this is output ");Serial.println(output[i]);
   // Serial.println("this is output ");
   // Serial.println(output[i]);
//char output[i] = payload[i] + '0';
    //added by Den
  //  char str1,str2;
  //  int val1,val2;
//    for(int i = 0; i < length; i++) sscanf(payload[i],"%[^,],%[^,],%d,%d",&str1, &str2, &val1, &val2);
  //  sscanf((char) payload[i],"%[^,],%[^,],%d,%d",&str1, &str2, &val1, &val2);
    
   Serial.println();


   String payload_str = String((char*) payload);
   Serial.println("This is payload as a string");
Serial.println(payload_str);

//if(payload_str == "requesttemp")
   /*
  USE_SERIAL.printf("[WSc] get text: %s\n", payload);
memcpy(arrPattern, arrRightOn, sizeof arrPattern);
            // send data to back to Server
            webSocket.sendTXT(payload, lenght);
            break;
            */
   //edn possible other option add below line
   //Serial.printf("[%u] here it is: %s\n",payload);
   //and commented this
   Serial.printf("[%u] here it is: %s\n", num, payload);
    webSocket.sendTXT(0,"pong"); //send message back to client
    Serial.println("Handling files now ...ROCKOFF");
 //add spiffss den
// Assign a file name e.g. 'names.dat' or 'data.txt' or 'data.dat' try to use the 8.3 file naming convention format could be 'data.d'
  char filename [] = "datalog.txt";                     // Assign a filename or use the format e.g. SD.open("datalog.txt",...);

 // if (SPIFFS.exists(filename)) SPIFFS.remove(filename); // First in this example check to see if a file already exists, if so delete it

  File myDataFile = SPIFFS.open(filename, "a+");        // Open a file for reading and writing (appending)
  if (!myDataFile)Serial.println("file open failed");   // Check for errors
  
  //den add
  for(int i = 0; i < length; i++) myDataFile.println((char) payload[i]);
  //myDataFile.println(payloads);     // Write some data to it (26-characters)
  myDataFile.println(3.141592654);
  Serial.println(myDataFile.size());                    // Display the file size (26 characters + 4-byte floating point number + 6 termination bytes (2/line) = 34 bytes)
  myDataFile.close();                                   // Close the file

  myDataFile = SPIFFS.open(filename, "r");              // Open the file again, this time for reading
  if (!myDataFile) Serial.println("file open failed");  // Check for errors
  while (myDataFile.available()) {
    Serial.write(myDataFile.read());                    // Read all the data from the file and display it
  }
  myDataFile.close();                                   // Close the file
  delay(10000);                                         // wait and then do it all again
 //end add spiffs den
 server.send(200, "text/html", "Handling files now ROCKOFF"); //Send ADC value only to client ajax request
    //end added by Den
     //22/06/18 Serial.printf("[%u] get Text: %s\n", num, payload);
      //added 180917 6.20
      
      //decoded %u >Print decimal unsigned int $s > null-terminated string -- num and payload are the vars
      if (payload[0] == '#') {            // we get RGB data
        /*
        uint32_t rgb = (uint32_t) strtol((const char *) &payload[1], NULL, 16);   // decode rgb data
        int r = ((rgb >> 20) & 0x3FF);                     // 10 bits per color, so R: bits 20-29
        int g = ((rgb >> 10) & 0x3FF);                     // G: bits 10-19
        int b =          rgb & 0x3FF;                      // B: bits  0-9

        analogWrite(LED_RED,   r);                         // write it to the LED output pins
        analogWrite(LED_GREEN, g);
        analogWrite(LED_BLUE,  b);
        */
        //added 180917
        Serial.print("payload[0] : "); Serial.print((char)payload[0]);
        Serial.print("payload[1] : "); Serial.print((char)payload[1]);
        Serial.print("payload[2] : "); Serial.print((char)payload[2]);
        Serial.print("payload[3] : "); Serial.print((char)payload[3]);
        Serial.print("payload[4] : "); Serial.print((char)payload[4]);
        Serial.print("payload[5] : "); Serial.print((char)payload[5]);
        
      } else if (payload[0] == '%') {                      // the browser sends an R when the rainbow effect is enabled
        rainbow = true;
        
        
      } else if (payload[0] == 'N') {                      // the browser sends an N when the rainbow effect is disabled
        rainbow = false;
      } else if (payload[0] == '*') {                      // the browser sends a * when its a message from den
        Serial.print("Hello Den it works !");
      } else if (payload[0] == 'S') {                      // the browser sends a * when its a message from den
        Serial.print("This is S !");
      } else if (payload[0] == 'Z') {                      // the browser sends a * when its a message from den
        Serial.print("Read spiffs requested");
        //call spiffs dir listing here
         Serial.println("READING SPIFFS started. Contents:");
  {
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {                      // List the file system contents
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      Serial.printf("\tFS File: %s, size: %s\r\n", fileName.c_str(), formatBytes(fileSize).c_str());
    }
    Serial.printf("\n");
  }
      }
      break;
  }
}//end webSocketEvent




/*__________________________________________________________HELPER_FUNCTIONS__________________________________________________________*/

String formatBytes(size_t bytes) { // convert sizes in bytes to KB and MB
  if (bytes < 1024) {
    return String(bytes) + "B";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + "KB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + "MB";
  }
}//end formatBytes

String getContentType(String filename) { // determine the filetype of a given filename, based on the extension
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".txt")) return "text/text";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}//end getContentType


