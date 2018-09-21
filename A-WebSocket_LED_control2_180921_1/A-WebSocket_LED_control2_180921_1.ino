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

//pin outputs
const int output13 = 13;

int length;         //Used to hold data length

//packet vars
// Define number of pieces
const int numberOfPieces = 4;
String pieces[numberOfPieces];


// Keep track of current position in array
int counter = 0;

// Keep track of the last comma so we know where to start the substring
int lastIndex = 0;

//end packet vars

const char* mdnsName = "esp8266"; // Domain name for the mDNS responder

/*__________________________________________________________SETUP__________________________________________________________*/

void setup() {


  Serial.begin(115200);        // Start the Serial communication to send messages to the computer
  delay(10);
  Serial.println("\r\n");
  //outputs
   pinMode(output13, OUTPUT);
  digitalWrite(output13, HIGH);

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
      for (int i = 0; i < length; i++) Serial.print((char) payload[i]);
      Serial.println();
      //***********************Get payload as a String****************
      String payload_str = String((char*) payload);
      Serial.println("This is payload as a string");
      Serial.println(payload_str);
      
      /*
        USE_SERIAL.printf("[WSc] get text: %s\n", payload);
        memcpy(arrPattern, arrRightOn, sizeof arrPattern);
               // send data to back to Server
               webSocket.sendTXT(payload, lenght);
               break;
      */
      
      Serial.printf("[%u] here it is: %s\n", num, payload);
      webSocket.sendTXT(0, "pong"); //send message back to client
      Serial.println("Handling files now ...ROCKOFF");
      //add spiffss den
      // Assign a file name e.g. 'names.dat' or 'data.txt' or 'data.dat' try to use the 8.3 file naming convention format could be 'data.d'
      char filename [] = "datalog.txt";                     // Assign a filename or use the format e.g. SD.open("datalog.txt",...);

      // if (SPIFFS.exists(filename)) SPIFFS.remove(filename); // First in this example check to see if a file already exists, if so delete it

      File myDataFile = SPIFFS.open(filename, "a+");        // Open a file for reading and writing (appending)
      if (!myDataFile)Serial.println("file open failed");   // Check for errors

      //den add
      for (int i = 0; i < length; i++) myDataFile.println((char) payload[i]);
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
      delay(1000);                                         // wait and then do it all again
      //end add spiffs den
      server.send(200, "text/html", "Handling files now ROCKOFF"); //Send ADC value only to client ajax request
      
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
        //
        //char buffers for packet
        char A[2] = {'\0' };
        char B[16] = {'\0' };
        char C[16] = {'\0' };
        char D[11] = {'\0' };
        char E[11] = {'\0' };
        char F[7] = {'\0' };
        char G[5] = {'\0' };
        char H[3] = {'\0' };
        sscanf((char *)payload, "%s %s %s %s %s %s %s %s", A, B, C, D, E, F, G, H);//working with spaces in instead of commas
        //sscanf((char *)payload, "%s,%s,%s,%s,%s,%s,%s,%s", A, B, C, D, E, F, G, H); //commas instead of spaces

        //print the char arrays
        
        Serial.printf("Here's A: %s\n", A);
        Serial.println(A[0]);
        Serial.println(A[1]);

        
        Serial.printf("Here's B: %s\n", B);
        Serial.println(B[0]);
        Serial.println(B[1]);
        Serial.println(B[2]);
        Serial.println(B[3]);
        Serial.println(B[4]);
        Serial.println(B[5]);
        Serial.println(B[6]);
        Serial.println(B[7]);
        Serial.println(B[8]);
        Serial.println(B[9]);
        Serial.println(B[10]);
        Serial.println(B[11]);
        Serial.println(B[12]);
        Serial.println(B[13]);
        Serial.println(B[14]);
        Serial.println(B[15]);
        Serial.println(B[16]);
        

        Serial.printf("Here's C: %s\n", C);
        Serial.println(C[0]);
        Serial.println(C[1]);
        Serial.println(C[2]);
        Serial.println(C[3]);
        Serial.println(C[4]);
        Serial.println(C[5]);
        Serial.println(C[6]);
        Serial.println(C[7]);
        Serial.println(C[8]);
        Serial.println(C[9]);
        Serial.println(C[10]);
        Serial.println(C[11]);
        Serial.println(C[12]);
        Serial.println(C[13]);
        Serial.println(C[14]);
        Serial.println(C[15]);
        Serial.println(C[16]);
        
        Serial.printf("Here's D: %s\n", D);
        Serial.println(D[0]);
        Serial.println(D[1]);
        Serial.println(D[2]);
        Serial.println(D[3]);
        Serial.println(D[4]);
        Serial.println(D[5]);
        Serial.println(D[6]);
        Serial.println(D[8]);
        Serial.println(D[9]);
        Serial.println(D[10]);
        
        Serial.printf("Here's E: %s\n", E);
        Serial.println(E[0]);
        Serial.println(E[1]);
        Serial.println(E[2]);
        Serial.println(E[3]);
        Serial.println(E[4]);
        Serial.println(E[5]);
        Serial.println(E[6]);
        Serial.println(E[8]);
        Serial.println(E[9]);
        Serial.println(E[10]);
        

Serial.printf("Here's F: %s\n", F);
        Serial.println(F[0]);
        Serial.println(F[1]);
        Serial.println(F[2]);
        Serial.println(F[3]);
        Serial.println(F[4]);
        Serial.println(F[5]);
        Serial.println(F[6]);
               
Serial.printf("Here's G: %s\n", G);
        Serial.println(G[0]);
        Serial.println(G[1]);
        Serial.println(G[2]);
        Serial.println(G[3]);
        Serial.println(G[4]);
        
if(strcmp(G, "1234") == 0){
            
            Serial.println("Homecode correct");
            digitalWrite(output13, !digitalRead(output13));
            }
        
Serial.printf("Here's H: %s\n", H);
        Serial.println(H[0]);
        Serial.println(H[1]);
        Serial.println(H[2]);
        
        //clear the arrays
        memset(A, 0, sizeof(A));
        memset(B, 0, sizeof(B));
        memset(C, 0, sizeof(C));
        memset(D, 0, sizeof(D));
        memset(E, 0, sizeof(E));
        memset(F, 0, sizeof(F));
        memset(G, 0, sizeof(G));
        memset(H, 0, sizeof(H));
/*
        //added 180917
        Serial.print("payload[0] : "); Serial.print((char)payload[0]);
        Serial.print("payload[1] : "); Serial.print((char)payload[1]);
        Serial.print("payload[2] : "); Serial.print((char)payload[2]);
        Serial.print("payload[3] : "); Serial.print((char)payload[3]);
        Serial.print("payload[4] : "); Serial.print((char)payload[4]);
        Serial.print("payload[5] : "); Serial.print((char)payload[5]);
        Serial.print("payload[6] : "); Serial.print((char)payload[6]);
        Serial.print("payload[7] : "); Serial.print((char)payload[7]);
        Serial.print("payload[8] : "); Serial.print((char)payload[8]);
        Serial.print("payload[9] : "); Serial.print((char)payload[9]);
        Serial.print("payload[10] : "); Serial.print((char)payload[10]);
        Serial.print("payload[11] : "); Serial.print((char)payload[11]);
        Serial.print("payload[12] : "); Serial.print((char)payload[12]);
        Serial.print("payload[13] : "); Serial.print((char)payload[13]);
        Serial.print("payload[14] : "); Serial.print((char)payload[14]);
        Serial.print("payload[15] : "); Serial.print((char)payload[15]);
        Serial.print("payload[16] : "); Serial.print((char)payload[16]);
        Serial.print("payload[17] : "); Serial.print((char)payload[17]);
        Serial.print("payload[18] : "); Serial.print((char)payload[18]);
        Serial.print("payload[19] : "); Serial.print((char)payload[19]);
        Serial.print("payload[20] : "); Serial.print((char)payload[20]);
        Serial.print("payload[21] : "); Serial.print((char)payload[21]);
        Serial.print("payload[22] : "); Serial.print((char)payload[22]);
        Serial.print("payload[23] : "); Serial.print((char)payload[23]);
        Serial.print("payload[24] : "); Serial.print((char)payload[24]);
        Serial.print("payload[25] : "); Serial.print((char)payload[25]);
        Serial.print("payload[26] : "); Serial.print((char)payload[26]);
        Serial.print("payload[27] : "); Serial.print((char)payload[27]);
        Serial.print("payload[28] : "); Serial.print((char)payload[28]);
        Serial.print("payload[29] : "); Serial.print((char)payload[29]);
        Serial.print("payload[30] : "); Serial.print((char)payload[30]);
*/
        for (byte idx = 0; idx < sizeof(payload) / sizeof(payload[0]); idx++) {
    Serial.print("payload[");
    Serial.print(idx);
    Serial.print("] = '");
    Serial.print((char)payload[idx]);
    Serial.println("'");
  }





      } else if (payload[0] == 'M') {                      // the browser sends an M for manual device update (on/off)
        //update calendar
        //send manual entry from putty maybe
        //to central calendar
        
            digitalWrite(output13, !digitalRead(output13));
            Serial.println("Manual Calendar Update");

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


