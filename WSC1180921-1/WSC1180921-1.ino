/*
 * WebSocketClient.ino
 *
 *  Created on: 24.05.2015
 *
 */

#include <Arduino.h>


//#include <ESP8266WiFiMulti.h>
#include <ESP8266WiFi.h>
#include <WebSocketsClient.h>

#include <Hash.h>
const char* ssid = "solo2";
const char* password = "risky00699";
//Static IP address configuration
IPAddress staticIP(192, 168, 1, 121); //ESP static ip
IPAddress gateway(192, 168, 1, 1);   //IP Address of your WiFi Router (Gateway)
IPAddress subnet(255, 255, 255, 0);  //Subnet mask
IPAddress dns(192, 168, 1, 1);  //DNS

const char *deviceName = "ESP-slave";

//ESP8266WiFiMulti WiFiMulti;
WebSocketsClient webSocket;

#define USE_SERIAL Serial1

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {

	switch(type) {
		case WStype_DISCONNECTED:
			//USE_SERIAL.printf("[WSc] Disconnected!\n");
			Serial.printf("[WSc] Disconnected!\n");
			break;
		case WStype_CONNECTED: {
			//USE_SERIAL.printf("[WSc] Connected to url: %s\n", payload);
Serial.printf("[WSc] Connected to url: %s\n", payload);
			// send message to server when Connected
			webSocket.sendTXT("Connected");
		}
			break;
		case WStype_TEXT:
		//USE_SERIAL.printf("[WSc] get text: %s\n", payload);
Serial.printf("[WSc] get text: %s\n", payload);
			// send message to server
			// webSocket.sendTXT("message here");
      webSocket.sendTXT("# pool garden 1537531200 1537531200 Normal 1234 12");
			break;
		case WStype_BIN:
			//USE_SERIAL.printf("[WSc] get binary length: %u\n", length);
			Serial.printf("[WSc] get binary length: %u\n", length);
			hexdump(payload, length);

			// send data to server
			// webSocket.sendBIN(payload, length);
			break;
	}

}

void setup() {
	// USE_SERIAL.begin(921600);
	//USE_SERIAL.begin(115200);

	//Serial.setDebugOutput(true);
	//USE_SERIAL.setDebugOutput(false);
/*
	USE_SERIAL.println();
	USE_SERIAL.println();
	USE_SERIAL.println();

	for(uint8_t t = 4; t > 0; t--) {
		USE_SERIAL.printf("[SETUP] BOOT WAIT %d...\n", t);
		USE_SERIAL.flush();
		delay(1000);
	}
*/
Serial.begin(115200);
  delay(1000);
  WiFi.hostname(deviceName);      // DHCP Hostname (useful for finding device for static lease)
  WiFi.config(staticIP, dns , gateway, subnet);
  WiFi.begin ( ssid, password );
  Serial.println ( "" );

  // Wait for connection
  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 500 );
    Serial.print ( "." );
	Serial.println ( "" );
  Serial.print ( "Connected to " );
  Serial.println ( ssid );
  Serial.print ( "IP address: " );
  Serial.println ( WiFi.localIP() );
  Serial.print("Subnet Mask ");
  Serial.println(WiFi.subnetMask());
  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());
  Serial.print("Gateway ");
  Serial.println(WiFi.gatewayIP());
  Serial.print("DNS: ");
  Serial.println(WiFi.dnsIP());
  Serial.print("Channel: ");
  Serial.println(WiFi.channel());
  Serial.print("Status : ");
  Serial.println(WiFi.status());
  
//  server.begin();


	}

	// server address, port and URL
	webSocket.begin("192.168.1.179", 81, "/");

	// event handler
	webSocket.onEvent(webSocketEvent);

	// use HTTP Basic Authorization this is optional remove if not needed
	webSocket.setAuthorization("user", "Password");

	// try ever 5000 again if connection has failed
	webSocket.setReconnectInterval(5000);

}

void loop() {
	webSocket.loop();
}
