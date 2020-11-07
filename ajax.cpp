#include <Arduino.h>
/*
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*/

// Import required libraries
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>

// Replace with your network credentials
const char* ssid = "istdochegal";
const char* password = "123sagichnicht";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
uint8_t r,g,b;

String rgb2hex(uint8_t r, uint8_t g, uint8_t b) {
  char hex[7] = {0};
  sprintf(hex, "%02X%02X%02X", r, g, b); //convert to an hexadecimal string. Lookup sprintf for what %02X means.
  Serial.println(hex); //Print the string.
  return hex;
}

void setup(){
  // Serial port for debugging purposes
  Serial.begin(9600);

  // Initialize SPIFFS
  if(!SPIFFS.begin()){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  // Print ESP32 Local IP Address
  Serial.println(WiFi.localIP());
  
  // Route to set GPIO to LOW
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html");
  });

  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request){
    //request->send("html/text", rgb2hex(255,0,128));
    request->send_P(200, "text/plain", rgb2hex(255,0,128).c_str());
  });

  // Start server
  server.begin();
}
 
void loop(){
  
}