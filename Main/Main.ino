 
  ////////////////////////////////////////////////////////
      #define DEVICE_NAME       "ShadowBox_bar-01"
      #define SOFTWARE_VERSION  "1.1.1"
      #define HARDWARE_VERSION  "1"
////////////////////////////////////////////////////////
                                 

#include <Preferences.h>
Preferences preferences;
#include <esp_task_wdt.h>
#include <deque> 
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip> 
#include <cstdlib> 
#include <future>
using namespace std;

#include "esp_ota_ops.h"
#include "nvs_flash.h"
#include "nvs.h"

//OLED
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <Wire.h>
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
 #define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

bool redrawScreen = false;

//Bluetooth
#include "NimBLEDevice.h" 
#include "esp_ota_ops.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <esp_task_wdt.h>
//#include <BLEDevice.h>
//#include <BLEUtils.h>
//#include <BLEServer.h>
//#include <BLE2902.h>

//Wifi 
#include <ArtnetWifi.h>
#include <WiFi.h>

//LEDs
#include <FastLED.h>
#define NUM_LEDS  144
#define HALF      NUM_LEDS/2
CRGB leds[NUM_LEDS];
int patternsListSize;
int paletteSize = 0;

#define WDT_TIMEOUT 999999

//State variables
string stateStr = "";
String ssid = "";
String password = "";
char* ipAddress = "";
int opMode = 0;
int artnetMode = 0;
int effectBrightness = 0;
int squelch = 10;              // Squelch, cuts out low level sounds
int gain = 10;                 // Gain, boosts input level
int bpm = 35;
int red = 0;
int green = 0;
int blue = 0;
int currentPalette = 6;
int currentArtnetMode = 0;

  /////////////////////////////////////////////////////////////
 //////////// State machine //////////////////////////////////
/////////////////////////////////////////////////////////////
NimBLECharacteristic* stateCharacteristic = NULL; 
void setStateCharacteristic()
{
  // bundle the state params into a delimited string
  stateStr = to_string(opMode) 
     + "," + to_string(gain)
     + "," + to_string(squelch)
     + "," + to_string(effectBrightness)
     + "," + to_string(artnetMode)
     + "," + to_string(bpm)
     + "," + to_string(currentPalette);
           //add more state variables
  Serial.println(stateStr.c_str());
  Serial.println(stateStr.length());
  std::vector<uint8_t> vec(stateStr.begin(), stateStr.end());
  stateCharacteristic->setValue(vec);
}

class stateCharacteristicCallbacks: public BLECharacteristicCallbacks 
{
    void onNotify(NimBLECharacteristic *pCharacteristic) 
    {
      try 
      {
        setStateCharacteristic();
      }
      catch (...) 
      { 
        Serial.println("error, captain");
      }
    }
};
/////////////////////////////////////////////////////////\\\\\\\\\
/////////////////////////////////////////////////////////\\\\\\\\
/////////////////////////////////////////////////////////\\\\\\\\\

//Audio variables
#define M_HEIGHT 8
uint8_t numBands = 16;
uint8_t prevFFTValue[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
uint8_t barHeights[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

#include "Audio.h"
#include "Wifi.h"
#include "Cereal.h"
#include "Bluetooth.h"
#include "Colors.h"
#include "Patterns.h"
#include "Encoder.h"

//Second core setup
TaskHandle_t task_loop1;
void esploop1(void* pvParameters) 
{
  setup2();

  for (;;)
    loop2();
}


///////////////////////////////////////////////////////////////
void setup(void)
{
  esp_task_wdt_init(WDT_TIMEOUT, false); //disable watchdog panic so ESP32 restarts
  Serial.begin(115200);
  Serial.println("core 0 setup");

//gfx
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
    display.setTextSize(1); // Draw 2X-scale text
    display.setTextColor(SSD1306_WHITE);
    // Clear the buffer
    display.clearDisplay();
//    for(int16_t i=0; i<display.height()/2; i+=2) {
//    display.drawRect(i, i, display.width()-2*i, display.height()-2*i, SSD1306_WHITE);
//    display.display(); // Update screen with each newly-drawn rectangle
//    delay(1);
//  }
//
//  delay(1000);
  
  preferences.begin("credentials", false); 
  ssid = preferences.getString("ssid", "NETGEAR72");
  password = preferences.getString("password", "brightfinch278");

  FastLED.addLeds<LED_TYPE, DATA_PIN, CLK_PIN, COLOR_ORDER, DATA_RATE_MHZ(12)>(leds, NUM_LEDS);
  FastLED.setBrightness(200);
  
  patternsListSize = sizeof(patterns)/sizeof(patterns[0]);
  paletteSize = sizeof(palettes)/sizeof(palettes[0]);

  setupEncoder();
  setupMenu();
  setupAudio();
  setupBluetooth();
  //ConnectWifi();

  Serial.println(SOFTWARE_VERSION);
   
  xTaskCreatePinnedToCore( /* Enable second core*/
  esploop1,               /* Task function. */
  "loop2",                /* name of task. */
  10000,                  /* Stack size of task */
  NULL,                   /* parameter of the task */
  1,                      /* priority of the task */
  &task_loop1,            /* Task handle to keep track of created task */
  !ARDUINO_RUNNING_CORE); /* pin task to core */  
  
}

void setup2() 
{
}

void loop() 
{
//  if (lightOn == true)
//  {
    patterns[opMode]();
//  }
}

void loop2(void)
{
    //ble testing

  EVERY_N_MILLISECONDS (2000)
  {
    if (bluetoothOn && deviceConnected)
    {
      setStateCharacteristic();
      stateCharacteristic->notify();
    }
  }

  static const unsigned long REFRESH_INTERVAL = 2000; // ms
  static unsigned long lastRefreshTime = 0;
  
  if(bluetoothOn && millis() - lastRefreshTime >= REFRESH_INTERVAL)
  {
    lastRefreshTime += REFRESH_INTERVAL;
    if (!deviceConnected)
    {
      NimBLEDevice::startAdvertising();
      pServer->startAdvertising(); // restart advertising
      Serial.printf("smoke some weeeeeed!! on core %d", xPortGetCoreID());Serial.println(); 
      oldDeviceConnected = deviceConnected;
    }  
  }
  else if(!bluetoothOn)
  {
    NimBLEDevice::stopAdvertising();
    deviceConnected = false;
  }
  
  updateScreen();
}
