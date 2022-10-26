// Rotary encoder pins
#define PIN_A 12
#define PIN_B 11
#define PUSH_BTN 10
#include <list>

int rotationCounter = 0;
int btnPressCounter = 0;
int menuItemsCount;
byte numRows = 4;
int bottomRow;
int topRow;

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

class MenuInterface {
     
  public:
  const char* Name;
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


void drawMenuDown()
{ 
  rotationCounter +=1;
  if (rotationCounter >= menuItemsCount) { rotationCounter--; }
  u8x8.clear();
  u8x8.drawString(5, 0, currentMenuObj->Name);
  if (menuItemsCount > numRows)
  {
    int diff = abs(topRow - rotationCounter);
    if (diff > numRows) {bottomRow++; diff--;}
    topRow = bottomRow - numRows;
    u8x8.drawString(0, 2+ diff, ">");
  
    for (int i=0; i<=numRows; i++)
    {
      u8x8.drawString(2, 2+ i, currentMenuObj->pageList[topRow+i]->GetName());
    }
  } else {
    //if (rotationCounter > menuItemsCount) {rotationCounter--;}
    u8x8.drawString(0, 2+ rotationCounter, ">");
    Serial.print("down ");Serial.println(currentMenuObj->pageList.size());
    for (int i=0; i<menuItemsCount; i++)
    {
      u8x8.drawString(2, 2+ i, currentMenuObj->pageList[i]->GetName());
    }
  }
}

void drawMenuUp() 
{ 
  rotationCounter -=1;
  if (rotationCounter < 0) { rotationCounter = 0; }
  u8x8.clear();
  u8x8.drawString(5, 0, currentMenuObj->Name);

  if (menuItemsCount > numRows)
  {
    int diff = bottomRow - rotationCounter;
    if (diff > numRows) {bottomRow--; diff--;}
    topRow = bottomRow - numRows;
    u8x8.drawString(0, 2+ numRows-diff, ">");
    
    for (int i=0; i<=numRows; i++)
    {
      u8x8.drawString(2, 2+ i, currentMenuObj->pageList[topRow+i]->GetName());
    }

  } else {
    if (bottomRow >= numRows) {topRow = bottomRow - numRows;}
    u8x8.drawString(0, 2+ rotationCounter, ">");
    
    for (int i=0; i<currentMenuObj->pageList.size(); i++)
    {
        //Serial.print("name ");Serial.println(currentMenuObj->pageList[i]->GetName());
      u8x8.drawString(2, 2+ i, currentMenuObj->pageList[i]->GetName());
    }
  }
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

int* currentEditParam;
int* currentEditParamMax;
int* currentEditParamIncrementAmt;

void increaseIntParam()
{
  *currentEditParam += *currentEditParamIncrementAmt;
  if (*currentEditParam > *currentEditParamMax) { *currentEditParam = *currentEditParamMax; }
  u8x8.clearLine(4);
  u8x8.drawString(6, 4, to_string(*currentEditParam).c_str());
  Serial.println((long)*currentEditParam);
}

void decreaseIntParam()
{
  *currentEditParam -= *currentEditParamIncrementAmt;
  if (*currentEditParam < 0) { *currentEditParam = 0; }
  u8x8.clearLine(4);
  u8x8.drawString(6, 4, to_string(*currentEditParam).c_str());
  Serial.println((long)*currentEditParam);
}


byte currentRotaryMethod = 0;
typedef void (*rotaryMethodList[])();
rotaryMethodList rotaryUpMethods= { 
                         drawMenuUp,
                         decreaseIntParam};
                         
rotaryMethodList rotaryDownMethods= { 
                         drawMenuDown,
                         increaseIntParam};
                         
void drawParamEdit(/*int* parameterPointer,*/ const char* parameterName)
{ 
  u8x8.clear();
  u8x8.drawString(5, 0, parameterName);

}


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   /////////////////////////MENU////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class Page : public MenuInterface { //Leaf
public:
  using MenuInterface::MenuInterface;
  //int* parameterPointer;
  void Display() override
  {
    drawParamEdit(Name);
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
    u8x8.clear();
    // save old rotary position?
    
    if (currentRotaryMethod == 0)
    {
      currentEditParam = parameterPointer;
      currentEditParamMax = &parameterMax;
      currentEditParamIncrementAmt = &incrementAmount;
      //Serial.println(*parameterPointer);
      currentRotaryMethod = 1;
      u8x8.drawString(4, 2, this->Name);
      u8x8.drawString(6, 4, to_string(*currentEditParam).c_str());
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
  //int* parameterPointer;
  char* GetName() override
  {
    if (wifiOn) 
    { 
      if (wifiConnected) { u8x8.drawString(2, 5, "Connected!"); }
      else               { u8x8.drawString(1, 5, "Not Connected"); }
    }
    u8x8.drawString(0, 6, "Network:"); u8x8.drawString(2, 7, ssid.c_str());
    //u8x8.drawString(0, 7, "Pass"); u8x8.drawString(6, 7, "****");
    if (wifiOn) { return "Wifi:   On"; }
    return "Wifi:   Off";
  }
  void Display() override
  {
    wifiOn = !wifiOn;
    if(wifiOn)
    {
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
    if (bluetoothOn && deviceConnected)
    {
      u8x8.drawString(2, 5, "Connected!"); 
    } 
    else if (bluetoothOn)
    {
      u8x8.drawString(1, 5, "Not Connected"); 
    }
    if (bluetoothOn) { return "Bluetooth: On"; }
    return "Bluetooth: Off";
  }
  void Display() override
  {
    bluetoothOn = !bluetoothOn;
    drawMenuDown();
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
      rotationCounter = 1;
      menuItemsCount = currentMenuObj->pageList.size();      
      if (menuItemsCount >= numRows) { bottomRow = numRows; }
      else { bottomRow = menuItemsCount-1; }
      topRow = 0;
      drawMenuDown();
    }
};

void setupMenu()
{
  Menu *menu = new Menu("Home");
  Menu *modes = new Menu("Mode");
  Menu *color = new Menu("Color");
  Menu *settings = new Menu("Settings");

  for (int i=0; i<patternsListSize; i++)
  {
    modes->AddMenuItem(new ModePage(patternNames[i]));
    Serial.println(patternNames[i]);
  }

  //color menus
  Menu *rgbSettingsMenu = new Menu("RGB");
  IntegerSettingsPage *redSettingsPage   = new IntegerSettingsPage("Red ",   &red,  255, 1);
  IntegerSettingsPage *greenSettingsPage = new IntegerSettingsPage("Green ", &green,255, 1);
  IntegerSettingsPage *blueSettingsPage  = new IntegerSettingsPage("Blue ",  &blue, 255, 1);
  rgbSettingsMenu->AddMenuItem(redSettingsPage);
  rgbSettingsMenu->AddMenuItem(greenSettingsPage);
  rgbSettingsMenu->AddMenuItem(blueSettingsPage);

  color->AddMenuItem(rgbSettingsMenu);
  
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

  menu->AddMenuItem(modes);
  menu->AddMenuItem(color);
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

bool debouncer() 
{
  static uint16_t state = 0;
  state = (state<<1) | digitalRead(PUSH_BTN) | 0xfe00;
  //Serial.println(state);
  return (state == 0xfe00);
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
    
    if (debouncer())
    {
      if (buttonPressed == false) 
      { 
        buttonPressed = true;
        Serial.println(rotationCounter);  
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
