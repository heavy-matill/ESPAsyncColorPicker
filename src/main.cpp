#include <Arduino.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h> //https://github.com/esp8266/Arduino
#else
#include <WiFi.h>
#endif

//needed for library
#include <ESPAsyncWiFiManager.h> //https://github.com/lbussy/AsyncWiFiManager
#define WEBSERVER_H              // https://github.com/me-no-dev/ESPAsyncWebServer/issues/418#issuecomment-667976368
#include <ESPAsyncWebServer.h>   //https://github.com/me-no-dev/ESPAsyncWebServer
#include <FS.h>                  // Include the SPIFFS library
#include <sstream>
#define RedLED 15
#define GreenLED 12
#define BlueLED 13

//global current colors
uint8_t r, g, b, w;

//AsyncWebServer wifiManagerServer(80);
AsyncWebServer server(80);
DNSServer dns;
AsyncWiFiManager wifiManager(&server, &dns);

// guard non interruptable functions
volatile boolean guard = true;

/* Processor example does not work with variable strings
// constant website with dummy placeholders
//const char *html = "<p>Replacement 1: %HELLO_FROM_TEMPLATE% °C</p><p>Number 2: %STH_ELSE_TEMPLATE% %</p><p>Manual</p>";
String processor(const String &var)
{
  if (var == "HELLO_FROM_TEMPLATE")
    return F("FF00a0");
  else if (var == "VAR_W")
    return F("STH_ELSE_TEMPLATE");
  return String();
}*/

String rgb2hex(uint8_t _r, uint8_t _g, uint8_t _b)
{
  char hex[7] = {0};
  sprintf(hex, "%02X%02X%02X", _r, _g, _b); //convert to an hexadecimal string. Lookup sprintf for what %02X means.
  return hex;
}
String getRGB()
{
  return rgb2hex(r, g, b);
}
String w2str(uint8_t _w)
{
  char buffer[3];
  sprintf(buffer, "%d", _w);
  return buffer;
}
String getW()
{
  return w2str(w);
}
void setColor()
{
  analogWrite(RedLED, r);
  analogWrite(GreenLED, g);
  analogWrite(BlueLED, b);
}

void setup()
{
  Serial.begin(115200);

  // Start the SPI Flash File System
  SPIFFS.begin();

  // start wifi before server
  // and server portal without blocking (dont use wifiManager.autoConnect())
  wifiManager.startConfigPortalModeless("AutoConnectAP", "Password");

  // Manually server responses under certain urls (overwrites static SPIFFS serving as found below)
  server.on("/getRGB", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", getRGB().c_str());
  });
  server.on("/getW", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", getW().c_str());
  });

  // Manually server custom functions
  server.on("/set", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("r"))
      r = request->arg("r").toInt();
    if (request->hasParam("g"))
      g = request->arg("g").toInt();
    if (request->hasParam("b"))
      b = request->arg("b").toInt();
    if (request->hasParam("w"))
      w = request->arg("w").toInt();
    setColor();
    //setWhite();
    request->send_P(200, "text/plain", "");
  });

  // Manually server custom functions
  //server.on("/setW", HTTP_GET, handleColorArgs);

  // For all other urls server whatever can be found in data directory
  server.serveStatic("/", SPIFFS, "/") //map root url [arg0] to root of "/data" [arg2]
      .setDefaultFile("index.html");
  //.setTemplateProcessor(processor); // optional processor

  Serial.println("Finished setup.");
  server.begin();
}

void loop()
{
  wifiManager.safeLoop();

  guard = true;
  wifiManager.criticalLoop();
  guard = false;
}