// Rotary encoder pins
#define PIN_A 12
#define PIN_B 11
#define PUSH_BTN 10
#include <list>

int middleWidth = display.width() / 2;
int middleHeight = display.height() / 2;

int rotationCounter = 0;
int btnPressCounter = 0;
int menuItemsCount;
byte numRows = 4;
int bottomRow;
int topRow;
static byte clicks;

bool buttonPressed = false;
unsigned long lastPress = 0;
unsigned long lastRelease = 0;

// Flag from interrupt routine (moved=true)
volatile bool rotaryEncoder = false;

// Interrupt routine just sets a flag when rotation is detected
void IRAM_ATTR rotary()
{
    rotaryEncoder = true;
}

bool debouncer() 
{
  static uint16_t state = 0;
  state = (state<<1) | digitalRead(PUSH_BTN) | 0xfe00;
  //Serial.println(state);
  return (state == 0xfe00);
}

void clearLine(byte lineNumber)
{
  display.fillRect(0, lineNumber*8, display.width(), 8, SSD1306_BLACK);
}

void setCursor(byte x, byte y)
{
  display.setCursor(x*8, y*8);
}

class MenuInterface {
     
  public:
  char* Name;
  MenuInterface(char* menuName){
    Name = menuName;
  }
  MenuInterface *parent;
  vector<MenuInterface*> pageList;
  
  void SetParent(MenuInterface *parent) {
    this->parent = parent;
  }
  MenuInterface *GetParent() const {
    return this->parent;
  }
  virtual bool IsComposite() const {
    return false;
  }
  virtual const char* GetName() {
    return this->Name;
  }
  virtual void Display(){}
    //virtual void Add(MenuInterface *menu) {}
};

MenuInterface* currentMenuObj = new MenuInterface("name");

void drawTitle()
{
  int w = sizeof(currentMenuObj->Name) * 8;
  
  
  display.setCursor(middleWidth - (w/2), 0);
  display.write(currentMenuObj->Name);
}

void drawMenuDown()
{ 
  rotationCounter +=1;
  if (rotationCounter >= menuItemsCount) { rotationCounter--; }
  
  display.clearDisplay();
  drawTitle();
  if (menuItemsCount > numRows)
  {
    int diff = abs(topRow - rotationCounter);
    if (diff > numRows) {bottomRow++; diff--;}
    topRow = bottomRow - numRows;
    //u8x8.drawString(0, 2+ diff, ">");
    display.setCursor(0, 16 + (diff * 8));
    display.write(">");
  
    for (int i=0; i<=numRows; i++)
    {
      //u8x8.drawString(2, 2+ i, currentMenuObj->pageList[topRow+i]->GetName());
      display.setCursor(16, 16 + (i * 8));
      display.write(currentMenuObj->pageList[topRow+i]->GetName());
    }
  } else {
    //if (rotationCounter > menuItemsCount) {rotationCounter--;}
    //u8x8.drawString(0, 2+ rotationCounter, ">");
    display.setCursor(0, 16 + (rotationCounter * 8));
    display.write(">");
    Serial.print("down ");Serial.println(currentMenuObj->pageList.size());
    for (int i=0; i<menuItemsCount; i++)
    {
      //u8x8.drawString(2, 2+ i, currentMenuObj->pageList[i]->GetName());
      display.setCursor(16, 16 + (i * 8));
      display.write(currentMenuObj->pageList[i]->GetName());
    }
  }
  display.display();
}

void drawMenuUp() 
{ 
  rotationCounter -=1;
  if (rotationCounter < 0) { rotationCounter = 0;}
  
  display.clearDisplay();
  drawTitle();
  if (menuItemsCount > numRows)
  {
    int diff = bottomRow - rotationCounter;
    if (diff > numRows) {bottomRow--; diff--;}
    topRow = bottomRow - numRows;
    //u8x8.drawString(0, 2+ numRows-diff, ">");
    display.setCursor(0, 16 + ((numRows- diff) * 8));
    display.write(">");
    
    for (int i=0; i<=numRows; i++)
    {
      //u8x8.drawString(2, 2+ i, currentMenuObj->pageList[topRow+i]->GetName());
      display.setCursor(16, 16 + (i * 8));
      display.write(currentMenuObj->pageList[topRow+i]->GetName());
    }
  } else {
    if (bottomRow >= numRows) {topRow = bottomRow - numRows;}
    //u8x8.drawString(0, 2+ rotationCounter, ">");
    display.setCursor(0, 16 + (rotationCounter * 8));
    display.write(">");
    
    for (int i=0; i<currentMenuObj->pageList.size(); i++)
    {
        //Serial.print("name ");Serial.println(currentMenuObj->pageList[i]->GetName());
      //u8x8.drawString(2, 2+ i, currentMenuObj->pageList[i]->GetName());
      display.setCursor(16, 16 + (i * 8));
      display.write(currentMenuObj->pageList[i]->GetName());
    }
  }
  display.display();
}

const char* drawNumbers(int** param) 
{
  std::stringstream ss;
  ss << std::setw ( 3 ) << param;
  auto x = ss.str();
  const char* count = x.c_str(); 
  return count;
  //u8x8.drawString(2, 2, count);
//  Serial.println(count);
}
const char* drawNumbers(int* param) 
{
  std::stringstream ss;
  ss << std::setw ( 3 ) << param;
  auto x = ss.str();
  const char* count = x.c_str(); 
  return count;
  //u8x8.drawString(2, 2, count);
//  Serial.println(count);
}

char* currentEditParamName = "";
int* currentEditParam;
int* currentEditParamMax;
int* currentEditParamIncrementAmt;

void displayCurrentEditParam()
{
  display.clearDisplay();
  //display.setCursor(4*8, 2*8);
  setCursor(4, 2);
  display.write(currentEditParamName);
  //display.setCursor(6*8, 4*8);
  setCursor(6, 4);
  display.write(to_string(*currentEditParam).c_str());
  display.display();
}

void increaseIntParam()
{
  *currentEditParam += *currentEditParamIncrementAmt;
  if (*currentEditParam > *currentEditParamMax) { *currentEditParam = *currentEditParamMax; }
  else { setStateCharacteristic(); stateCharacteristic->notify(); }
  displayCurrentEditParam();
}

void decreaseIntParam()
{
  *currentEditParam -= *currentEditParamIncrementAmt;
  if (*currentEditParam < 0) { *currentEditParam = 0; }
  else { setStateCharacteristic(); stateCharacteristic->notify(); }
  displayCurrentEditParam();
}


byte currentRotaryMethod = 0;
typedef void (*rotaryMethodList[])();
rotaryMethodList rotaryUpMethods= { 
                         drawMenuUp,
                         decreaseIntParam};
                         
rotaryMethodList rotaryDownMethods= { 
                         drawMenuDown,
                         increaseIntParam};
                         

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   /////////////////////////MENU////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class Page : public MenuInterface { //Leaf
public:
  using MenuInterface::MenuInterface;
  //int* parameterPointer;
  void Display() override
  {
    
  }
};

class ModePage : public MenuInterface { //Leaf
public:
  //int* parameterPointer;
  using MenuInterface::MenuInterface;
  void Display() override
  {
    opMode = rotationCounter - 1;
  }
};

class PalettePage : public MenuInterface { //Leaf
public:
  using MenuInterface::MenuInterface;
  void Display() override
  {
    currentPalette = rotationCounter - 1;
  }
};

class IntegerSettingsPage : public MenuInterface { //Leaf
public:
  //using MenuInterface::MenuInterface;
  int *parameterPointer;
  int parameterMax;
  int incrementAmount;
  IntegerSettingsPage(char* menuName, int *pointer, int paramMax, int incrementAmt) : MenuInterface{ menuName }, parameterPointer{ pointer }, parameterMax{ paramMax }, incrementAmount{ incrementAmt } {
     parameterPointer = pointer;
     parameterMax = paramMax;
     incrementAmount = incrementAmt;
  }
  const char* GetName() override
  {
    //return (this->Name);// + to_string(*parameterPointer)).c_str();
    return(Name);
  }
  void Display() override
  {
    display.clearDisplay();
    // save old rotary position?
    
    if (currentRotaryMethod == 0)
    {
      currentEditParamName = Name;
      currentEditParam = parameterPointer;
      currentEditParamMax = &parameterMax;
      currentEditParamIncrementAmt = &incrementAmount;
      //Serial.println(*parameterPointer);
      currentRotaryMethod = 1;
      displayCurrentEditParam();
    } 
    else if (currentRotaryMethod == 1)
    {
        currentRotaryMethod = 0;
        drawMenuDown();
    }
  }
};


class WifiSettingsPage : public MenuInterface { //Leaf
public:
  using MenuInterface::MenuInterface;
  char* GetName() override
  {
    if (wifiOn) 
    { 
      if (wifiConnected) 
      {
        display.setCursor(16, 32); display.write("Connected! iP:");
        //display.setCursor(0,  40); display.write("iP:");
        display.setCursor(32, 40); display.write(WiFi.localIP().toString().c_str());
      }
      else               
      {
        display.setCursor(8, 32); display.write("Not Connected"); 
      }
    }
    display.setCursor(0, 48); display.write("Network:");
    display.setCursor(16, 56); display.write(ssid.c_str());
    display.setCursor(16, 24); // set the cursor back to line 3!!
    display.display();
    if (wifiOn) { return "Wifi:   On"; }
    return "Wifi:   Off";
  }
  void Display() override
  {
    wifiOn = !wifiOn;
    if(wifiOn)
    {
      //clearLine(4);
      
      std::future<bool> future = mypromise.get_future();
      ConnectWifi();
      bool state = future.get();
      Serial.println(state);
    } else {
      DisconnectWifi();
      mypromise = std::promise<bool>();
    }
    drawMenuDown();
  }
};


class BluetoothSettingsPage : public MenuInterface { //Leaf
public:
  using MenuInterface::MenuInterface;
  //int* parameterPointer;
  char* GetName() override
  {
    setCursor(2, 6);
    display.write(DEVICE_NAME);
    if (bluetoothOn && deviceConnected)
    {
      //u8x8.drawString(2, 5, "Connected!"); 
      //display.setCursor(16, 40);
      setCursor(2, 5); 
      display.write("Connected!");
    } 
    else if (bluetoothOn)
    {
      //u8x8.drawString(1, 5, "Not Connected"); 
      //display.setCursor(8, 40);
      setCursor(1, 5);
      display.write("Scanning...");
    }
    //display.setCursor(16, 24); // set the cursor back to line 3!!
    setCursor(2, 3);
    display.display();
    if (bluetoothOn == true) { return "Bluetooth: On"; }
    return "Bluetooth: Off";
  }
  void Display() override
  {
    bluetoothOn = !bluetoothOn;
    drawMenuDown();
  }
};

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 ///////// spectrum analyzer patterns ///////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void topDown() 
{
  byte w = display.width() / 16;
  for (int i=0; i < 16; i++) 
  {
    display.fillRect(i*w, 0, w-1, fftResult[i], SSD1306_WHITE);
  }
}
void middleOut() 
{
  byte w = display.width() / 16;
  for (int i=0; i < 16; i++) 
  {
    double fillAmt = fftResult[i] / 2;
    double h = (middleHeight - (fillAmt / 2));
    display.fillRect(i*w, h, w-1, fillAmt, SSD1306_WHITE);
  }
}
typedef void (*spectrumPatternList[])();
spectrumPatternList spectrumPatterns = { topDown, middleOut };

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class SpectrumAnalyzerPage : public MenuInterface { //Leaf
public:
  using MenuInterface::MenuInterface;
  const char* GetName() override
  {
    return(Name);
  }
  void middleOut() 
  {
    byte w = display.width() / 16;
    for (int i=0; i < 16; i++) 
    {
      double fillAmt = fftResult[i] / 2;
      double h = (middleHeight - (fillAmt / 2));
      display.fillRect(i*w, h, w-1, fillAmt, SSD1306_WHITE);
    }
  }
  void Display() override
  {
    clicks = 0;
    display.clearDisplay();

    while (!rotaryEncoder)
    {
      display.clearDisplay();
      //if (debouncer()) { clicks++; }
      //if (clicks > 1) { clicks = 0; }
      fillFFT();
      //spectrumPatterns[clicks]();
      middleOut(); 
      display.display();  
    }
  }
};

// Composite
class Menu : public MenuInterface {
  
  public:
    using MenuInterface::MenuInterface;
    vector<MenuInterface*> pageList;
    
    void AddMenuItem(MenuInterface *component) {    
      component->SetParent(this);
      this->pageList.push_back(component);
    }
    bool IsComposite() const override {
      return true;
    } 
    void Display() override 
    {
      currentMenuObj->pageList = pageList;
      if (this->Name != "Home") { currentMenuObj->pageList.insert(currentMenuObj->pageList.begin(), this->parent); }
      currentMenuObj->Name = Name;
      rotationCounter = 0;
      menuItemsCount = currentMenuObj->pageList.size();      
      if (menuItemsCount >= numRows) { bottomRow = numRows; }
      else { bottomRow = menuItemsCount-1; }
      topRow = 0;
      drawMenuDown();
    }
};

void setupMenu()
{
  
  //Mode Menu
  Menu *modes = new Menu("Mode");

  for (int i=0; i<patternsListSize; i++)
  {
    modes->AddMenuItem(new ModePage(patternNames[i]));
    Serial.println(patternNames[i]);
  }

  //color menus
  Menu *color = new Menu("Color");
  
  Menu *rgbSettingsMenu = new Menu("RGB");
  IntegerSettingsPage *redSettingsPage   = new IntegerSettingsPage("Red ",   &red,  255, 1);
  IntegerSettingsPage *greenSettingsPage = new IntegerSettingsPage("Green ", &green,255, 1);
  IntegerSettingsPage *blueSettingsPage  = new IntegerSettingsPage("Blue ",  &blue, 255, 1);
  rgbSettingsMenu->AddMenuItem(redSettingsPage);
  rgbSettingsMenu->AddMenuItem(greenSettingsPage);
  rgbSettingsMenu->AddMenuItem(blueSettingsPage);

  Menu *paletteSelectMenu = new Menu("Palette Select");
  IntegerSettingsPage *palettePage = new IntegerSettingsPage("Palettes", &currentPalette, paletteSize, 1);
  //PalettePage *palettePage = new PalettePage("Palettes");
  paletteSelectMenu->AddMenuItem(palettePage);
  
  color->AddMenuItem(rgbSettingsMenu);
  color->AddMenuItem(paletteSelectMenu);

  //Settings Menus
  Menu *settings = new Menu("Settings");
  
  Menu *audioSettingsMenu = new Menu("Audio");
  IntegerSettingsPage *gainSettingsPage = new IntegerSettingsPage("Gain  ", &gain, 30, 1);
  audioSettingsMenu->AddMenuItem(gainSettingsPage);

  IntegerSettingsPage *squelchSettingsPage = new IntegerSettingsPage("Squelch  ", &squelch, 30, 1);
  audioSettingsMenu->AddMenuItem(squelchSettingsPage);
  
  Menu *wifiSettingsMenu = new Menu("Wifi");
  WifiSettingsPage *wifiSettingsPage = new WifiSettingsPage("Wifi");
  wifiSettingsMenu->AddMenuItem(wifiSettingsPage);

  Menu *bluetoothSettingsMenu = new Menu("Bluetooth");
  BluetoothSettingsPage *bluetoothSettingsPage = new BluetoothSettingsPage("Bluetooth");
  bluetoothSettingsMenu->AddMenuItem(bluetoothSettingsPage);

  IntegerSettingsPage *brightnessSettingsPage = new IntegerSettingsPage("Brightness ", &effectBrightness, 250, 10);
  settings->AddMenuItem(brightnessSettingsPage);

  IntegerSettingsPage *bpmSettingsPage = new IntegerSettingsPage("BPM ", &bpm, 250, 1);
  settings->AddMenuItem(bpmSettingsPage);
  
  settings->AddMenuItem(audioSettingsMenu);
  settings->AddMenuItem(wifiSettingsMenu);
  settings->AddMenuItem(bluetoothSettingsMenu);
  
  SpectrumAnalyzerPage *spectrumAnalyzerPage = new SpectrumAnalyzerPage("Spectrum");

  Menu *menu = new Menu("Home");
  menu->AddMenuItem(spectrumAnalyzerPage);
  menu->AddMenuItem(modes);
  menu->AddMenuItem(color);
  menu->AddMenuItem(palettePage);
  menu->AddMenuItem(settings);
  
  menu->Display();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int8_t checkRotaryEncoder()
{
    // Reset the flag that brought us here (from ISR)
    rotaryEncoder = false;
    static uint8_t lrmem = 3;
    static int lrsum = 0;
    static int8_t TRANS[] = {0, -1, 1, 14, 1, 0, 14, -1, -1, 14, 0, 1, 14, 1, -1, 0};
    // Read BOTH pin states to deterimine validity of rotation (ie not just switch bounce)
    int8_t l = digitalRead(PIN_A);
    int8_t r = digitalRead(PIN_B);
    // Move previous value 2 bits to the left and add in our new values
    lrmem = ((lrmem & 0x03) << 2) + 2 * l + r;
    // Convert the bit pattern to a movement indicator (14 = impossible, ie switch bounce)
    lrsum += TRANS[lrmem];
    
    if (lrsum % 4 != 0)/* encoder not in the neutral (detent) state */
    {
        return 0;
    }
    if (lrsum == 4)/* encoder in the neutral state - clockwise rotation*/
    {
        lrsum = 0;

        //drawMenuDown();
        rotaryDownMethods[currentRotaryMethod]();
        return 1;
    }
    if (lrsum == -4) /* encoder in the neutral state - anti-clockwise rotation*/
    {
        lrsum = 0;

        //drawMenuUp();
        rotaryUpMethods[currentRotaryMethod]();
        return -1;
    }
    // An impossible rotation has been detected - ignore the movement
    lrsum = 0;
    return 0;
}

void setupEncoder() 
{
  pinMode(PIN_A, INPUT);
  pinMode(PIN_B, INPUT);
  pinMode(PUSH_BTN, INPUT_PULLUP);
  Serial.println("");
  attachInterrupt(digitalPinToInterrupt(PIN_A), rotary, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_B), rotary, CHANGE);
}

void updateScreen()
{
  if (redrawScreen == true) 
  { 
    drawMenuDown(); 
    redrawScreen = false;
  }
  if (rotaryEncoder) // Has rotary encoder moved?
  {
    // Get the movement (if valid)
    int8_t rotationValue = checkRotaryEncoder();
  }
//
//  if (millis() - lastPress < 250 && clicks > 1)
//  {
//      clicks = 0;
//      Serial.println("double click");
//      currentMenuObj->parent->Display();
//  }
//  if (millis() - lastPress > 250 && clicks == 1)
//  {
//    clicks = 0;
//    currentMenuObj->pageList[rotationCounter]->Display(); 
//    Serial.println("single click");
//  }
  if (debouncer())
  {
    if (buttonPressed == false) 
    { 
      lastPress = millis();
      buttonPressed = true;
      Serial.println(rotationCounter);  
      clicks++;
      currentMenuObj->pageList[rotationCounter]->Display();      
    }
  } else {
    if (buttonPressed == true) 
    { 
      buttonPressed = false;
      lastRelease = millis();
      //Serial.println("Button released!");        
    }
  }
}
