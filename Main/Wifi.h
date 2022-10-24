const int numberOfChannels = NUM_LEDS * 3; // Total number of channels you want to receive (1 led = 3 channels)
const int startUniverse = 0; // CHANGE FOR YOUR SETUP most software this is 1, some software send out artnet first universe as 0.
byte startChannel = 0;
byte artnetBrightness = 0;
uint8_t artnetMode = 1;
int previousDataLength = 0;

bool wifiOn = false;
bool wifiConnected  =false;
std::promise<bool> mypromise;
ArtnetWifi artnet;

void ConnectWifi(void)
{
  WiFi.begin(ssid.c_str(), password.c_str());
  Serial.println("");
  Serial.println("Connecting to WiFi");
  unsigned long lastConnectionRequest = millis();
  // Wait for connection
  Serial.print("Connecting");
  while (millis() - lastConnectionRequest < 10000)
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      wifiConnected = true;
      break;
    }
  }
  if (wifiConnected == true)
  {
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    artnet.begin();
  }
  else
  {
    Serial.println("");
    Serial.println("Connection failed.");
  }
  mypromise.set_value(wifiConnected);
}

bool DisconnectWifi()
{
  try
  {
    WiFi.disconnect();
    wifiConnected = false;   
    return true;
  } 
  catch (...)
  {
    return false;
  }
}

void artnetPixelMap(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data)
{
  // read universe and put into the right part of the display buffer
//  for (int i = 0; i < length / 3; i++)
//  {
//    int led = i + (universe - startUniverse) * (previousDataLength / 3);
//    if (led < NUM_LEDS)
//    {
//      leds[led] = CRGB(data[i * 3], data[i * 3 + 1], data[i * 3 + 2]);
//    }
//  }
//  previousDataLength = length;  
//  FastLED.show();   
}

void artnetFourChannel(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data)
{
//  float brightness = float(data[startChannel+3])  / float(255); // dimmer on 4
//  float red   =      float(data[startChannel])   * brightness;  // red    on 1
//  float green =      float(data[startChannel+1]) * brightness;  // green  on 2
//  float blue  =      float(data[startChannel+2]) * brightness;  // blue   on 3
//  //color = CRGB(red, green, blue);  
//
//  // strobe on 5
//  if (data[startChannel+4] > 25)
//  {
//    //strobeAmt = data[startChannel+4];
//    if (currentPattern != 1) {currentPattern = 1;}
//    return;
//  }
//
//  // scroll through palettes and scale them to brightness slider
//  //if (data[] > 10)
////  {
////  
////  }
//
//  // check the rest of the channels
//  for (byte i = 2; i < patternsListSize; i++)
//  {
//    if (data[i + startChannel + 6] > 15)
//    {
//      if (currentPattern != i) {currentPattern = i;}
//      return;
//    }
//  }
//
//  for (int i = 0; i < NUM_LEDS; i++)
//  {
//    leds[i] = CRGB(red, green, blue);
//  }
//  currentPattern = 0;
//  FastLED.show();
}
