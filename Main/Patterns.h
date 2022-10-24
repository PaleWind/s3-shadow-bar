
/*************************EFFECTS AND LED SETUP****************/
/*===========================================================*/

#define LED_TYPE      SK9822
#define COLOR_ORDER   BGR
#define DATA_PIN      16
#define CLK_PIN       17

std::deque<uint8_t> scroll(NUM_LEDS, 0);

CRGB leds[NUM_LEDS];

bool lightOn = true;
byte strobeAmt = 0;
CRGB color = CRGB::Black;
volatile uint8_t style = 0;
volatile long lastRun = 0;

int getLows()
{
  return barHeights[0] + barHeights[1] + barHeights[3] + barHeights[2] + 
         barHeights[4] + barHeights[5] + barHeights[6] + barHeights[7];
}

double getHighs()
{
  return (barHeights[8] +  barHeights[9] +  barHeights[10] + barHeights[11] 
        + barHeights[12] + barHeights[13] + barHeights[14] + barHeights[15]) * 1.3;
}

void showScroll()
{
  for (uint16_t i=0; i<NUM_LEDS; i++)
  {
    if (scroll[i]>1) 
    {
      uint8_t index = inoise8(i*20, millis()/5+i*20);                  
      leds[i] = ColorFromPalette(palettes[currentPalette], index, effectBrightness, LINEARBLEND);  
    }
    else 
    {
      leds[i] = CRGB::Black;
    }
  }
  FastLED.show(); 
}

void strobe()
{
  if (millis() - lastRun > strobeAmt) 
  {
    if (style)
    {
      LEDS.showColor(color);
    }
    else
    {
      LEDS.showColor(CRGB::Black);
    }
    style = !style;
    lastRun = millis();
  }
}

void twoBars() 
{
  fillFFT();
  //FastLED.clear();
  fadeToBlackBy(leds, NUM_LEDS, 30);
  uint16_t low = barHeights[0] + barHeights[1] + barHeights[3] + barHeights[2] + barHeights[4] + barHeights[5] + barHeights[6] + barHeights[7]; 
  uint8_t lows = (uint16_t)low > HALF ? HALF-1 : (uint16_t)low;
  double high = (barHeights[8] + barHeights[9] + barHeights[10] + barHeights[11] + barHeights[12] + barHeights[13] + barHeights[14] + barHeights[15]) * 1.3;
  uint8_t highs = (uint16_t)high > HALF ? HALF-1 : (uint16_t)high;
  
  for (int i=0; i<lows; i++)
  { 
    uint8_t index = inoise8(i*20, millis()/5+i*20);               
    leds[i] = ColorFromPalette(palettes[currentPalette], index, 250, LINEARBLEND);   
  }
  for (int i=highs; i>0; i--)
  {
    uint8_t index = inoise8(i*20, millis()/5+i*20);                
    leds[NUM_LEDS - i] = ColorFromPalette(palettes[currentPalette], index, 250, LINEARBLEND);
  }
  FastLED.show(); 
}

void breathe()
{
  uint8_t wave = beatsin8(bpm, 10, 250);
  for(int i = 0; i < NUM_LEDS; i++) 
  {                                       // Just ONE loop to fill up the LED array as all of the pixels change.
      uint8_t index = inoise8(i*20, millis()/5+i*20);                   // Get a value from the noise function. I'm using both x and y axis.
      leds[i] = ColorFromPalette(palettes[currentPalette], index, wave, LINEARBLEND);    // With that value, look up the 8 bit colour palette value and assign it to the current LED.
  }
  //fill_palette(leds, NUM_LEDS, color, 0, palettes[currentPalette], wave, LINEARBLEND);
  FastLED.show();
}

void bounce()
{
  fadeToBlackBy(leds, NUM_LEDS, 20);
  uint8_t wave = beatsin8(bpm, 0, NUM_LEDS);
  uint8_t index = inoise8(wave*20, millis()/5+wave*20);
  leds[wave] = ColorFromPalette(palettes[currentPalette], index, effectBrightness, LINEARBLEND);
  FastLED.show();
}

void makeNoise()
{
  for(int i = 0; i < NUM_LEDS; i++) 
  {                                      
      uint8_t index = inoise8(i*20, millis()/5+i*20);            
      leds[i] = ColorFromPalette(palettes[currentPalette], index, effectBrightness, LINEARBLEND);  
  }
  FastLED.show();
}

//void scrollHighsUp(byte range)
//{
//  double frequencies = !range ? getHighs() : getLows();
//  scroll.pop_back();
//  scroll.push_front((uint16_t)getHighs());
//  showScroll();
//}

void scrollHighsUp()
{
  fillFFT();
  double highs = getHighs();
  scroll.pop_back();
  scroll.push_front((uint16_t)highs);
  showScroll();
}

void scrollHighsDown()
{
  fillFFT();
  double highs = getHighs();
  scroll.pop_front();
  scroll.push_back((uint16_t)highs);
  showScroll();
}

void scrollHighsOut()
{
  fillFFT();
  double highs = getHighs();
  auto count = scroll.begin()+HALF;
  scroll.pop_back();
  scroll.pop_front();
  scroll.insert(count+1, (int)highs);
  scroll.insert(count, (int)highs);
  showScroll();
}

void scrollLowsUp()
{
  fillFFT();
  uint16_t lows = getLows();
  scroll.pop_back();
  scroll.push_front(lows);
  showScroll();
}

void scrollLowsDown()
{
  fillFFT();
  uint16_t lows = getLows();
  scroll.pop_front();
  scroll.push_back(lows);
  showScroll();
}

void scrollLowsOut()
{
  fillFFT();
  uint16_t lows = getLows();
  auto count = scroll.begin()+HALF;
  scroll.pop_back();
  scroll.pop_front();
  scroll.insert(count+1, lows);
  scroll.insert(count, lows);
  showScroll();
}

void scrollOut()
{
  fillFFT();
  double highs = getHighs();
  uint16_t lows = getLows();
  auto count = scroll.begin()+HALF;
  scroll.pop_back();
  scroll.pop_front();
  scroll.insert(count+1, (int)highs);
  scroll.insert(count, lows);
  showScroll();
}
void scrollOutTwo()
{
  fillFFT();
  double highs = getHighs();
  uint16_t lows = getLows();
  auto count = scroll.begin()+HALF;
  scroll.pop_back();
  scroll.pop_front();
  scroll.insert(count+1, lows);
  scroll.insert(count, (int)highs);
  showScroll();
}

void artnetDMX()
{
  artnet.read();
}

void artnetMap()
{
  artnet.read();
}

  /////////////////////////////////////////////////////////////
 //////////////////////////Pattern List///////////////////////
/////////////////////////////////////////////////////////////

byte currentPattern = 0;
typedef void (*patternList[])();
patternList patterns = { 
                         makeNoise, 
                         bounce, 
                         twoBars,
                         strobe,
                         breathe, 
                         makeNoise,
                         scrollHighsUp,
                         scrollLowsUp,
                         scrollHighsDown,
                         scrollLowsDown,
                         scrollHighsOut, 
                         scrollLowsDown,
                         scrollOut, 
                         scrollOutTwo,
                         artnetMap,
                         artnetDMX};

char *patternNames[] = {
                        "makeNoise", 
                        "bounce", 
                        "twoBars",
                        "strobe",
                        "breathe", 
                        "makeNoise",
                        "scrollHighsUp",
                        "scrollLowsUp",
                        "scrollHighsDown",
                        "scrollLowsDown",
                        "scrollHighsOut", 
                        "scrollLowsDown",
                        "scrollOut", 
                        "scrollOutTwo",
                        "artnetMap",
                        "artnetDMX"};

byte patternsListSize = sizeof(patterns);
