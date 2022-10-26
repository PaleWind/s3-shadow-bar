#define STATE_SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define SSID_SERVICE_UUID         "3c662598-3367-489d-ad3f-484ec8970642"
#define PASSWORD_SERVICE_UUID     "2d804258-a9ee-4f2d-afe6-eb005aae21ad"

#define WRITE_CHARACTERISTIC_UUID "opb5483e-36e1-4688-b7f5-ea07361b26a8"
#define SSID_CHARACTERISTIC_UUID  "id5fb0c0-0df9-11ed-861d-0242ac120002"
#define PASSWORD_CHARACTERISTIC_UUID "6a7b8a24-4594-42eb-b721-62d0275123b5"
#define STATE_CHARACTERISTIC_UUID "85c2a1b1-c6ae-47dd-8793-cd84f4f0a745"

bool bluetoothOn = false;
bool deviceConnected = false;
bool oldDeviceConnected = false;

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
BLECharacteristic* ssidCharacteristic = NULL;
BLECharacteristic* passwordCharacteristic = NULL;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      //bleconnected = 1;
      //      getBoxStatus(); // make a control group for getting info from the box
      Serial.println("connected!");
      redrawScreen = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      //bleconnected = 0;
      Serial.println("bye bye!");
      redrawScreen = true;
    }
};

class MyWriteCallbacks: public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pCharacteristic)
    {
      std::string stringValue = pCharacteristic->getValue();
      if (stringValue.length() > 0)
      {
        try
        {
          int numVal = stoi(stringValue);
          Serial.println(numVal);
          byte cmdGroup = numVal / 1000;
          byte cmdVal = numVal % 1000;
          Serial.println(cmdGroup);
          //executeCommand(numVal);
          setStateCharacteristic();
          stateCharacteristic->notify();
        }
        catch (...)
        {
          Serial.println("error, captain");
        }
      }
    }
};

class ssidCallbacks: public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *ssidCharacteristic)
    {
      std::string stringValue = ssidCharacteristic->getValue();
      try
      {
        ssid = stringValue.c_str();
        Serial.println(stringValue.c_str());
        //strcpy(ssid, stringValue.c_str());//to,from
        preferences.putString("ssid", ssid);
        Serial.println("ssid " + ssid);
        ssidCharacteristic->notify();
      }
      catch (...)
      {
        Serial.println("error, captain");
      }
    }
};

class passwordCallbacks: public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *passwordCharacteristic)
    {
      std::string stringValue = passwordCharacteristic->getValue();
      try
      {
        password = stringValue.c_str();
        //strcpy(ssid, stringValue.c_str());//to,from
        preferences.putString("password", password);
        Serial.println("password " + password);
        passwordCharacteristic->notify();
      }
      catch (...)
      {
        Serial.println("error, captain");
      }
    }
};

void setupBluetooth()
{
  BLEDevice::init(DEVICE_NAME);
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *stateService =    pServer->createService(STATE_SERVICE_UUID);
  BLEService *ssidService =     pServer->createService(SSID_SERVICE_UUID);
  BLEService *passwordService = pServer->createService(PASSWORD_SERVICE_UUID);

  pCharacteristic = stateService->createCharacteristic(
                      WRITE_CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ  |
                      BLECharacteristic::PROPERTY_WRITE |
                      BLECharacteristic::PROPERTY_NOTIFY);

  pCharacteristic->addDescriptor(new BLE2902());
  pCharacteristic->setCallbacks(new MyWriteCallbacks());
  pCharacteristic->setValue("You found the light! You're cleaver");

  stateCharacteristic = stateService->createCharacteristic(
                          STATE_CHARACTERISTIC_UUID,
                          BLECharacteristic::PROPERTY_READ  |
                          BLECharacteristic::PROPERTY_WRITE |
                          BLECharacteristic::PROPERTY_NOTIFY);
  stateCharacteristic->addDescriptor(new BLE2902());
  stateCharacteristic->setCallbacks(new stateCharacteristicCallbacks());
  setStateCharacteristic();
  //stateCharacteristic->setValue(stateStr.c_str());

  ssidCharacteristic = ssidService->createCharacteristic(
                         SSID_CHARACTERISTIC_UUID,
                         BLECharacteristic::PROPERTY_READ  |
                         BLECharacteristic::PROPERTY_WRITE |
                         BLECharacteristic::PROPERTY_NOTIFY);

  ssidCharacteristic->addDescriptor(new BLE2902());
  ssidCharacteristic->setCallbacks(new ssidCallbacks());
  ssidCharacteristic->setValue(ssid.c_str());

  passwordCharacteristic = passwordService->createCharacteristic(
                             PASSWORD_CHARACTERISTIC_UUID,
                             BLECharacteristic::PROPERTY_READ  |
                             BLECharacteristic::PROPERTY_WRITE |
                             BLECharacteristic::PROPERTY_NOTIFY);

  passwordCharacteristic->addDescriptor(new BLE2902());
  passwordCharacteristic->setCallbacks(new passwordCallbacks());
  passwordCharacteristic->setValue(password.c_str());

  stateService->start();
  ssidService->start();
  passwordService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(STATE_SERVICE_UUID);
  pAdvertising->addServiceUUID(SSID_SERVICE_UUID);
  pAdvertising->addServiceUUID(PASSWORD_SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();  //pAdvertising->start();
}
