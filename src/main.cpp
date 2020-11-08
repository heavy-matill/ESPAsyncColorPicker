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
#include <ArduinoJson.h>

#include <FastLED.h>
#include "FastLED_RGBW.h"

#define RedLED 15
#define GreenLED 12
#define BlueLED 13

#define LED_PIN 5
#define COLOR_ORDER RGB
#define CHIPSET WS2812B
#define BRIGHTNESS 255
#define SERPENTINE true
#define NUM_LEDS 10

CRGBW leds[NUM_LEDS];
CRGB *ledsRGB = (CRGB *)&leds[0];

//global current colors
uint8_t r, g, b, w;

//AsyncWebServer wifiManagerServer(80);
AsyncWebServer server(80);
DNSServer dns;
AsyncWiFiManager wifiManager(&server, &dns);

struct Config
{
  uint8_t r, g, b, w;
};
Config storedConfig;
const char *configFilename = "/config.json";

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
String rgbw2hex(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _w)
{
  char hex[9] = {0};
  sprintf(hex, "%02X%02X%02X%02X", _r, _g, _b, _w); //convert to an hexadecimal string. Lookup sprintf for what %02X means.
  return hex;
}
String getRGBW()
{
  return rgbw2hex(r, g, b, w);
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

  fill_solid(leds, NUM_LEDS, CRGBW(r, g, b, w));
}

// Loads the configuration from a file
void readConfiguration(const char *filename)
{
  // Open file for reading
  File file = SPIFFS.open(filename, "r");
  // Allocate the memory pool on the stack.
  // Don't forget to change the capacity to match your JSON document.
  // Use arduinojson.org/assistant to compute the capacity.
  StaticJsonDocument<72> doc;
  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, file);
  if (error)
    Serial.println(F("Failed to read file, using default configuration"));
  // Load values
  storedConfig.r = doc["r"] | 0;
  storedConfig.g = doc["g"] | 0;
  storedConfig.b = doc["b"] | 0;
  storedConfig.w = doc["w"] | 0;
  // Close the file (File's destructor doesn't close the file)
  file.close();
}

// Saves the configuration to a file
void saveConfiguration(const char *filename)
{
  if (storedConfig.r == r && storedConfig.g == g && storedConfig.b == b && storedConfig.w == w)
  {
    Serial.println(F("Config is the same as previous and will not be stored"));
    return;
  }
  // Delete existing file, otherwise the configuration is appended to the file
  SPIFFS.remove(filename);
  // Open file for writing
  File file = SPIFFS.open(filename, "w");
  if (!file)
  {
    Serial.println(F("Failed to create file"));
    return;
  }
  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/assistant to compute the capacity.
  StaticJsonDocument<72> doc;
  // Set the values in the document
  doc["r"] = r;
  doc["g"] = g;
  doc["b"] = b;
  doc["w"] = w;
  // Serialize JSON to file
  if (serializeJson(doc, file) == 0)
  {
    Serial.println(F("Failed to write to file"));
  }
  else
  {
    // update configLoaded
    storedConfig.r = r;
    storedConfig.g = g;
    storedConfig.b = b;
    storedConfig.w = w;
  }
  // Close the file (File's destructor doesn't close the file)
  file.close();
}

// apply loaded to current values
void restoreColors()
{
  r = storedConfig.r;
  g = storedConfig.g;
  b = storedConfig.b;
  w = storedConfig.w;
  setColor();
}

void setup()
{
  Serial.begin(115200);

  // RGBW Setup
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(ledsRGB, getRGBWsize(NUM_LEDS)); //.setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(BRIGHTNESS);
    fill_solid(leds, NUM_LEDS, CRGBW(r, g, b, w));
    FastLED.show();


  // Start the SPI Flash File System
  SPIFFS.begin();
  // load stored config
  readConfiguration(configFilename);
  restoreColors();

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
  server.on("/get", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", getRGBW().c_str());
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

  server.on("/store", HTTP_GET, [](AsyncWebServerRequest *request) {
    saveConfiguration(configFilename);
    request->send_P(200, "text/plain", "");
  });
  server.on("/restore", HTTP_GET, [](AsyncWebServerRequest *request) {
    restoreColors();
    request->send_P(200, "text/plain", getRGBW().c_str());
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
  EVERY_N_MILLIS(50)
  {
    fill_solid(leds, NUM_LEDS, CRGBW(r, g, b, w));
    FastLED.show();
  } // slowly cycle the "base color" through the rainbow
  wifiManager.criticalLoop();
  guard = false;
}