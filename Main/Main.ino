 
     ////////////////////////////////////////////////////////
   /*///////*/#define DEVICE_NAME "ShadowBox_bar-02"///////
  ////////////////////////////////////////////////////////

String ssid = "";
String password = "";

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
using namespace std;

//OLED
#include <SPI.h>
#include <Wire.h>
#include <U8x8lib.h>
U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(/* reset=*/ U8X8_PIN_NONE);   

//Bluetooth
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

//Wifi 
#include <ArtnetWifi.h>
#include <WiFi.h>

//LEDs
#include <FastLED.h>
#define NUM_LEDS  32
#define HALF      NUM_LEDS/2

#define WDT_TIMEOUT 999999

//State variables
string stateStr = "";
uint8_t opMode = 0;
uint8_t effectBrightness = 200;
int squelch = 10;                           // Squelch, cuts out low level sounds
int gain = 10;                              // Gain, boosts input level
int bpm = 35;

  /////////////////////////////////////////////////////////////
 //////////// State machine //////////////////////////////////
/////////////////////////////////////////////////////////////
BLECharacteristic* stateCharacteristic = NULL; 
void setStateCharacteristic()
{
  // bundle the state params into a delimited string
  stateStr = to_string(opMode) + "," + to_string(gain) + "," + to_string(squelch) + "," + to_string(effectBrightness);
           //add more state variables
  Serial.println(stateStr.c_str());
  stateCharacteristic->setValue(stateStr.c_str());
}

class stateCharacteristicCallbacks: public BLECharacteristicCallbacks 
{
    void onNotify(BLECharacteristic *stateCharacteristic) 
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
#include "Bluetooth.h"
#include "Wifi.h"
#include "Colors.h"
#include "Patterns.h"
#include "Encoder.h"

//Second core setup
TaskHandle_t task_loop1;
void esploop1(void* pvParameters) 
{
  setup1();

  for (;;)
    loop1();
}


///////////////////////////////////////////////////////////////
void setup(void)
{
  esp_task_wdt_init(WDT_TIMEOUT, false); //disable watchdog panic so ESP32 restarts
  Serial.begin(115200);
  Serial.println("core 0 setup");
  
  u8x8.begin();
  u8x8.setFont(u8x8_font_amstrad_cpc_extended_f);
  
  preferences.begin("credentials", false); 
  ssid = preferences.getString("ssid", "NETGEAR72");
  password = preferences.getString("password", "brightfinch278");

  FastLED.addLeds<LED_TYPE, DATA_PIN, CLK_PIN, COLOR_ORDER, DATA_RATE_MHZ(12)>(leds, NUM_LEDS);
  FastLED.setBrightness(200);
  
  setupEncoder();
  setupMenu();
  setupAudio();
  setupBluetooth();
  //ConnectWifi();
  
  xTaskCreatePinnedToCore( /* Enable second core*/
  esploop1,               /* Task function. */
  "loop1",                /* name of task. */
  10000,                  /* Stack size of task */
  NULL,                   /* parameter of the task */
  1,                      /* priority of the task */
  &task_loop1,            /* Task handle to keep track of created task */
  !ARDUINO_RUNNING_CORE); /* pin task to core */  
  
}

void setup1() 
{
}

void loop() 
{
//  if (lightOn == true)
//  {
    patterns[opMode]();
//  }
}

void loop1(void)
{
    //ble testing
//  if (deviceConnected)
//  {
//    EVERY_N_MILLISECONDS (3000)
//    {
//      gain++;
//      setStateCharacteristic();
//      stateCharacteristic->notify();
//    }
//  }

  static const unsigned long REFRESH_INTERVAL = 1000; // ms
  static unsigned long lastRefreshTime = 0;
  
  if(bluetoothOn && millis() - lastRefreshTime >= REFRESH_INTERVAL)
  {
    lastRefreshTime += REFRESH_INTERVAL;
//    u8x8.drawString(0, 3, std::to_string(xPortGetCoreID()).c_str());
//    u8x8.clearLine(4);
//    u8x8.drawString(0, 4, deviceConnected ? "connected" : "not connected");
    if (!deviceConnected)
    {
      pServer->startAdvertising(); // restart advertising
      Serial.printf("hey, wanna smoke some weed? on core %d", xPortGetCoreID());Serial.println();
      oldDeviceConnected = deviceConnected;
    }  
  }
  
  updateScreen();
  //u8x8.drawString(0, 5, std::to_string(rotationCounter).c_str());
}
