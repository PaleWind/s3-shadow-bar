 ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define STATE_SERVICE_UUID            "172b0aa9-9d49-42c3-a5f7-5eec258b7342"
#define STATE_CHARACTERISTIC_UUID     "724bfada-d204-4d47-ac9c-77cb60c12011"
#define WRITE_CHARACTERISTIC_UUID     "814b9ce9-3114-4eed-96d4-f90b5b6155fd"

#define SSID_SERVICE_UUID             "172b0aa9-9723-45c6-94bc-78102bbc9961"
#define SSID_CHARACTERISTIC_UUID      "ecc6ba40-b056-4836-a81b-f2543977caa1"

#define PASSWORD_SERVICE_UUID         "0d603309-3610-457e-abdd-b0e12057bdab"
#define PASSWORD_CHARACTERISTIC_UUID  "b85b5cc4-22cf-4210-87f6-e26a6706ca83"

#define OTA_SERVICE_UUID              "0dc6ee5c-9002-412c-8af4-a97aaa994602"
#define OTA_CHARACTERISTIC_UUID       "62ec0272-3ec5-11eb-b378-0242ac130005"
#define TX_CHARACTERISTIC_UUID        "62ec0272-3ec5-11eb-b378-0242ac130003"

#define VERSION_SERVICE_UUID          "e5166f27-4c4d-4429-88e4-64dae8efc38b"
#define VERSION_CHARACTERISTIC_UUID   "c8659212-af91-4ad3-a995-a58d6fd26145"

 ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//OTA 
static esp_ota_handle_t otaHandler = 0;
static const esp_partition_t *update_partition = NULL;

uint8_t     txValue = 0;
int         bufferCount = 0;
bool        downloadFlag = false;

bool bluetoothOn = true;
bool deviceConnected = false;
bool oldDeviceConnected = false;

NimBLEServer* pServer = NULL;
NimBLECharacteristic* pCharacteristic = NULL;
NimBLECharacteristic* ssidCharacteristic = NULL;
NimBLECharacteristic* passwordCharacteristic = NULL;
NimBLECharacteristic* txCharacteristic = NULL;
NimBLECharacteristic* otaCharacteristic = NULL;
NimBLECharacteristic* versionCharacteristic = NULL;

class MyServerCallbacks: public BLEServerCallbacks {

  void onConnect(NimBLEServer* pServer, ble_gap_conn_desc* desc) {
      Serial.println("*** App connected");
      /*----------------------------------------
       * BLE Power settings. P9 = max power +9db
       ---------------------------------------*/
      esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_CONN_HDL0, ESP_PWR_LVL_P9);
      esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_CONN_HDL1, ESP_PWR_LVL_P9);
      esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT, ESP_PWR_LVL_P9);
      esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, ESP_PWR_LVL_P9);

      Serial.println(NimBLEAddress(desc->peer_ota_addr).toString().c_str());
      /*    We can use the connection handle here to ask for different connection parameters.
            Args: connection handle, min connection interval, max connection interval
            latency, supervision timeout.
            Units; Min/Max Intervals: 1.25 millisecond increments.
            Latency: number of intervals allowed to skip.
            Timeout: 10 millisecond increments, try for 5x interval time for best results.
      */
      pServer->updateConnParams(desc->conn_handle, 12, 12, 2, 100);
      deviceConnected = true;
      redrawScreen = true;
    }

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      downloadFlag    = false;
      Serial.println("*** App disconnected");
      redrawScreen = true;
    }
};

class MyWriteCallbacks: public BLECharacteristicCallbacks
{
    void onWrite(NimBLECharacteristic *pCharacteristic)
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
    void onWrite(NimBLECharacteristic *ssidCharacteristic)
    {
      std::string stringValue = ssidCharacteristic->getValue();
      try
      {
        ssid = stringValue.c_str();
        Serial.println(stringValue.c_str());
        //strcpy(ssid, stringValue.c_str());//to,from
        preferences.putString("ssid", ssid);
        Serial.println("ssid " + ssid);
        std::vector<uint8_t> ssidvec(ssid.begin(), ssid.end());
        ssidCharacteristic->setValue(ssidvec);
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
    void onWrite(NimBLECharacteristic *passwordCharacteristic)
    {
      std::string stringValue = passwordCharacteristic->getValue();
      try
      {
        password = stringValue.c_str();
        //strcpy(ssid, stringValue.c_str());//to,from
        preferences.putString("password", password);
        Serial.println("password " + password);
        std::vector<uint8_t> passwordvec(password.begin(), password.end());
        passwordCharacteristic->setValue(passwordvec);
        passwordCharacteristic->notify();
      }
      catch (...)
      {
        Serial.println("error, captain");
      }
    }
};


class otaCallback: public BLECharacteristicCallbacks {
    
    void onWrite(NimBLECharacteristic *pCharacteristic) 
    {
      std::string rxData = pCharacteristic->getValue();
      bufferCount++;
     
      if (!downloadFlag) 
      {
        //-----------------------------------------------
        // First BLE bytes have arrived
        //-----------------------------------------------
        
        Serial.println("1. BeginOTA");
        const esp_partition_t *configured = esp_ota_get_boot_partition();
        const esp_partition_t *running = esp_ota_get_running_partition();

        if (configured != running) 
        {
          Serial.printf("ERROR: Configured OTA boot partition at offset 0x%08x, but running from offset 0x%08x", configured->address, running->address);
          Serial.println("(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
          downloadFlag = false;
          esp_ota_end(otaHandler);
        } else {
          Serial.printf("2. Running partition type %d subtype %d (offset 0x%08x) \n", running->type, running->subtype, running->address);
        }

        update_partition = esp_ota_get_next_update_partition(NULL);
        assert(update_partition != NULL);

        Serial.printf("3. Writing to partition subtype %d at offset 0x%x \n", update_partition->subtype, update_partition->address);
        
        //------------------------------------------------------------------------------------------
        // esp_ota_begin can take a while to complete as it erase the flash partition (3-5 seconds) 
        // so make sure there's no timeout on the client side (iOS) that triggers before that. 
        //------------------------------------------------------------------------------------------
        esp_task_wdt_init(10, false);
        vTaskDelay(5);
        
        if (esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &otaHandler) != ESP_OK) {
          downloadFlag = false;
          return;
        }
        downloadFlag = true;
      }
      
      if (bufferCount >= 1 || rxData.length() > 0) 
      { 
        if(esp_ota_write(otaHandler, (uint8_t *) rxData.c_str(), rxData.length()) != ESP_OK) {
          Serial.println("Error: write to flash failed");
          downloadFlag = false;
          return;
        } else {
          bufferCount = 1;
          Serial.println("--Data received---");
          //Notify the iOS app so next batch can be sent
          txCharacteristic->setValue(&txValue, 1);
          txCharacteristic->notify();
        }
        
        //-------------------------------------------------------------------
        // check if this was the last data chunk? (normaly the last chunk is 
        // smaller than the maximum MTU size). For improvement: let iOS app send byte 
        // length instead of hardcoding "510"
        //-------------------------------------------------------------------
        if (rxData.length() < 510) // TODO Asumes at least 511 data bytes (@BLE 4.2). 
        {
          Serial.println("4. Final byte arrived");
          //-----------------------------------------------------------------
          // Final chunk arrived. Now check that
          // the length of total file is correct
          //-----------------------------------------------------------------
          if (esp_ota_end(otaHandler) != ESP_OK) 
          {
            Serial.println("OTA end failed ");
            downloadFlag = false;
            return;
          }
          
          //-----------------------------------------------------------------
          // Clear download flag and restart the ESP32 if the firmware
          // update was successful
          //-----------------------------------------------------------------
          Serial.println("Set Boot partion");
          if (ESP_OK == esp_ota_set_boot_partition(update_partition)) 
          {
            esp_ota_end(otaHandler);
            downloadFlag = false;
            Serial.println("Restarting...");
            esp_restart();
            return;
          } else {
            //------------------------------------------------------------
            // Something whent wrong, the upload was not successful
            //------------------------------------------------------------
            Serial.println("Upload Error");
            downloadFlag = false;
            esp_ota_end(otaHandler);
            return;
          }
        }
      } else {
        downloadFlag = false;
      }
    }
};


void setupBluetooth()
{
  NimBLEDevice::init(DEVICE_NAME);
  NimBLEDevice::setMTU(517);

  pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  NimBLEService *stateService    = pServer->createService(STATE_SERVICE_UUID);
  NimBLEService *ssidService     = pServer->createService(SSID_SERVICE_UUID);
  NimBLEService *passwordService = pServer->createService(PASSWORD_SERVICE_UUID);
  NimBLEService *otaService      = pServer->createService(OTA_SERVICE_UUID);
  NimBLEService *versionService  = pServer->createService(VERSION_SERVICE_UUID);
  
  txCharacteristic = otaService->createCharacteristic(TX_CHARACTERISTIC_UUID,
                      NIMBLE_PROPERTY:: NOTIFY);

  otaCharacteristic = otaService->createCharacteristic(OTA_CHARACTERISTIC_UUID,
                       NIMBLE_PROPERTY:: WRITE_NR | NIMBLE_PROPERTY::NOTIFY);
  otaCharacteristic->setCallbacks(new otaCallback());

  pCharacteristic = stateService->createCharacteristic(
                      WRITE_CHARACTERISTIC_UUID,
                      NIMBLE_PROPERTY::READ  | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY);
  pCharacteristic->setCallbacks(new MyWriteCallbacks());
  pCharacteristic->setValue("You found the light! You're cleaver");

  stateCharacteristic = stateService->createCharacteristic(
                          STATE_CHARACTERISTIC_UUID,
                          NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY);
  stateCharacteristic->setCallbacks(new stateCharacteristicCallbacks());
  setStateCharacteristic();
  std::vector<uint8_t> statevec(stateStr.begin(), stateStr.end());
  stateCharacteristic->setValue(statevec);

  ssidCharacteristic = ssidService->createCharacteristic(
                         SSID_CHARACTERISTIC_UUID, 
                         NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY);
  ssidCharacteristic->setCallbacks(new ssidCallbacks());
  std::vector<uint8_t> ssidvec(ssid.begin(), ssid.end());
  ssidCharacteristic->setValue(ssidvec);

  passwordCharacteristic = passwordService->createCharacteristic(
                             PASSWORD_CHARACTERISTIC_UUID, 
                             NIMBLE_PROPERTY::READ  | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY);
  passwordCharacteristic->setCallbacks(new passwordCallbacks());
  std::vector<uint8_t> passwordvec(password.begin(), password.end());
  passwordCharacteristic->setValue(passwordvec);

  versionCharacteristic = versionService->createCharacteristic(
                             VERSION_CHARACTERISTIC_UUID,
                             NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
  versionCharacteristic->setValue(SOFTWARE_VERSION);
   
  stateService->start();
  ssidService->start();
  passwordService->start();
  otaService->start();
  versionService->start();
    
  NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(STATE_SERVICE_UUID);
  pAdvertising->addServiceUUID(SSID_SERVICE_UUID);
  pAdvertising->addServiceUUID(PASSWORD_SERVICE_UUID);
  pAdvertising->addServiceUUID(OTA_SERVICE_UUID);
  pAdvertising->addServiceUUID(VERSION_SERVICE_UUID);

  NimBLEDevice::stopAdvertising();  //pAdvertising->start();
}
