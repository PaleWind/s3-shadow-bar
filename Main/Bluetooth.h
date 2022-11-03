 ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define STATE_SERVICE_UUID            "172b0aa9-9d49-42c3-a5f7-5eec258b7342"
#define SSID_SERVICE_UUID             "172b0aa9-9723-45c6-94bc-78102bbc9961"
#define PASSWORD_SERVICE_UUID         "0d603309-3610-457e-abdd-b0e12057bdab"

#define WRITE_CHARACTERISTIC_UUID     "814b9ce9-3114-4eed-96d4-f90b5b6155fd"
#define SSID_CHARACTERISTIC_UUID      "ecc6ba40-b056-4836-a81b-f2543977caa1"
#define PASSWORD_CHARACTERISTIC_UUID  "b85b5cc4-22cf-4210-87f6-e26a6706ca83"
#define STATE_CHARACTERISTIC_UUID     "724bfada-d204-4d47-ac9c-77cb60c12011"

 ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool bluetoothOn = true;
bool deviceConnected = false;
bool oldDeviceConnected = false;

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
BLECharacteristic* ssidCharacteristic = NULL;
BLECharacteristic* passwordCharacteristic = NULL;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("connected!");
      redrawScreen = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
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
          executeCommand(numVal);
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
  BLEDevice::stopAdvertising();  //pAdvertising->start();
}
