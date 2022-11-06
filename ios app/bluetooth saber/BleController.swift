import CoreBluetooth
import CoreData
import SwiftUI

public var txCharacteristic: CBCharacteristic!
public var rxCharacteristic: CBCharacteristic!
public var periphDevice: CBPeripheral!
public var connected = false;

class Saber: Identifiable, ObservableObject {
    var name: String
    let id = UUID()
    
    @Published var currentFirmwareVersion = ""
    @Published var state = []
    @Published var brightness = 100.0
    @Published var opMode = 0
    @Published var gain = 0.0
    @Published var squelch = 0.0
    @Published var artnetMode = 0.0
    @Published var bpm = 0
    @Published var currentPalette = 0
    @Published var redValue = 0.0
    @Published var greenValue = 0.0
    @Published var blueValue = 0.0
    @Published var isConnected = false
    @Published var periph: CBPeripheral!
    @Published var ssid = ""
    @Published var password = ""
    @Published var connectionTimeOut = 4
    @Published var modes = ["Solid Color",
                            "Make Noise",
                            "Bounce",
                            "Two Bars",
                            "Strobe",
                            "Breathe",
                            "Scrl Up HP",
                            "Scrl Up LP",
                            "Scrl Down HP",
                            "Scrl Down LP",
                            "Scrl Out HP",
                            "scrollLowsDown",
                            "scrollOut",
                            "scrollOutTwo",
                            "artnetMap",
                            "artnetDMX"]
    
    init (periph:CBPeripheral!, name: String) {
        self.periph = periph
        self.name = name
    }
    
    func writeOutgoingValue(_ data: String) {
        let valueString = (data as NSString).data(using: String.Encoding.utf8.rawValue)
        if let myPeripheral = periph
        {
            if let txCharacteristic = myPeripheral.services?.first?.characteristics?.first //fix this 
            {
                myPeripheral.writeValue(valueString!, for: txCharacteristic, type: CBCharacteristicWriteType.withResponse)
                print(txCharacteristic.value as Any)
            }
        }
    }
    
    func writeOutgoingSsid(_ data: String) {
        let valueString = (data as NSString).data(using: String.Encoding.utf8.rawValue)
        if let myPeripheral = periph
        {
            if let txCharacteristic = myPeripheral.services?.first(where: {$0.uuid == UUIDs.SSID_SERVICE_UUID})?.characteristics?.first //fix this
            {
                print(valueString as Any)
                myPeripheral.writeValue(valueString!, for: txCharacteristic, type: CBCharacteristicWriteType.withResponse)
            }
        }
    }
    
    func writeOutgoingPassword(_ data: String) {
        let valueString = (data as NSString).data(using: String.Encoding.utf8.rawValue)
        if let myPeripheral = periph
        {
            if let txCharacteristic = myPeripheral.services?.first(where: {$0.uuid == UUIDs.PASSWORD_SERVICE_UUID})?.characteristics?.first //fix this
            {
                print(valueString as Any)
                myPeripheral.writeValue(valueString!, for: txCharacteristic, type: CBCharacteristicWriteType.withResponse)
            }
        }
    }
    
    @discardableResult func getCurrentFirmwareVersion() -> String {
        if let myPeripheral = periph
        {
            if let rxCharacteristic = myPeripheral.services?.first(where: {$0.uuid == UUIDs.VERSION_SERVICE_UUID})?.characteristics?.first?.value //fix this
            {
                self.currentFirmwareVersion = String(decoding: rxCharacteristic, as: UTF8.self)
            } else {
                print("error getting version # from saber")
                return ""
            }
        }
        print("current version \(self.currentFirmwareVersion)")
        return self.currentFirmwareVersion
    }
    
    func resetConnectionExpiration() {
        self.connectionTimeOut = 4;
    }

}

class BleManager: NSObject, ObservableObject, CBCentralManagerDelegate, CBPeripheralDelegate
{
    var centralManager: CBCentralManager!             //the ios device
    @Published var myPeripherals = [Saber]()         //the ble peripheral device
    @Published var latestFirmwareVersion: FirmwareVersion
        
    //Used for ota
    @Published var name = ""
    @Published var connected = false
    @Published var transferProgress : Double = 0.0
    @Published var chunkCount = 2 // number of chunks to be sent before peripheral needs to accknowledge.
    @Published var elapsedTime = 0.0
    @Published var kBPerSecond = 0.0
    
    //transfer varibles
    var dataToSend = Data()
    var dataBuffer = Data()
    var chunkSize = 0
    var dataLength = 0
    var transferOngoing = true
    var sentBytes = 0
    var packageCounter = 0
    var startTime = 0.0
    var stopTime = 0.0
    var firstAcknowledgeFromESP32 = false
    
    override init()
    {
        latestFirmwareVersion = try! getLatestFirmwareVersionD()
        super.init()
        centralManager = CBCentralManager(delegate: self, queue: nil)
        
        //latestFirmwareVersion = v as? String ?? ""
    }
    
    func getCharValue(_ data: Data?)  -> String {
        var characteristicASCIIValue = NSString()
        guard let characteristicValue = data else {
              print("no data")
              return "no data"
          }

          let ASCIIstring = NSString(data: characteristicValue, encoding: String.Encoding.utf8.rawValue)

          characteristicASCIIValue = ASCIIstring ?? ""
        print("Value converted: \((characteristicASCIIValue as String))")
        return characteristicASCIIValue as String
    }
    
    func centralManagerDidUpdateState(_ central: CBCentralManager)
    {
        if central.state == CBManagerState.poweredOn {
            print("BLE powered on")
//            central.scanForPeripherals(withServices: nil, options: nil)
        }
        else {
            print("Something wrong with BLE")
        }
    }
    
    func centralManager(_ central: CBCentralManager, didDiscover peripheral: CBPeripheral, advertisementData: [String : Any], rssi RSSI: NSNumber)
    {
        if let pname = peripheral.name
        {
            print(pname)
            let results = myPeripherals.filter { $0.name == pname }
            if pname.contains("ShadowBox") && results.isEmpty
            {
                print("found one!")
                //self.centralManager.connect(peripheral, options: nil)
                self.myPeripherals.append(Saber(periph: peripheral, name: pname))
                
                self.myPeripherals.last!.periph.delegate = self
            }
        }
    }
    
    func centralManager(_ central: CBCentralManager, didConnect peripheral: CBPeripheral)
    {
        peripheral.discoverServices(nil)
    }
    
    func centralManager(_ central: CBCentralManager, didDisconnectPeripheral peripheral: CBPeripheral,
                        error: Error?)
    {
        
    }
    
    func peripheral(_ peripheral: CBPeripheral, didDiscoverServices error: Error?)
    {
        print("*******************************************************")

        if ((error) != nil)
        {
            print("Error discovering services: \(error!.localizedDescription)")
            return
        }
        guard let services = peripheral.services else
        {
            return
        }
        //We need to discover the all characteristic
        for service in services
        {
            peripheral.discoverCharacteristics(nil, for: service)
        }
        print("Discovered Services: \(services)")
    }
    
    func peripheral(_ peripheral: CBPeripheral, didDiscoverCharacteristicsFor service: CBService, error: Error?)
    {
        guard let serviceCharacteristics = service.characteristics else
        { return }

        print("Found \(serviceCharacteristics.count) characteristics.")

        for characteristic in serviceCharacteristics
        {
            //characteristics?.append(characteristic)

            peripheral.setNotifyValue(true, for: characteristic)
            peripheral.readValue(for: characteristic)

            print("Added Characteristic: \(characteristic)")
            //print(characteristics as Any)
        }
        
    }
    

    func peripheral(_ peripheral: CBPeripheral, didUpdateValueFor characteristic: CBCharacteristic, error: Error?)
    {
        var characteristicASCIIValue = NSString()
        guard let characteristicValue = characteristic.value else {
            print("no data")
            return
        }
        let saber = self.myPeripherals.first(where: {$0.name == peripheral.name})
        let ASCIIstring = NSString(data: characteristicValue, encoding: String.Encoding.utf8.rawValue)
        characteristicASCIIValue = ASCIIstring ?? ""
        print("Value Recieved: \(characteristicASCIIValue as String) ")
        
        let valueReceived: String = characteristicASCIIValue as String
        if characteristic.service?.uuid == UUIDs.STATE_SERVICE_UUID {
            
            //unwrap state characterstic here
            let state = getNumbers(valueReceived)
            print("periph state: \(state.count)")
            if state.count == 4 {
                saber?.opMode = state[0]
                saber?.gain   = Double(state[1])
                saber?.squelch = Double(state[2])
                saber?.brightness = Double(state[3])
               // saber?.artnetMode = Double(state[4])
               // saber?.bpm = state[5]
               // saber?.currentPalette = state[6]
//                saber?.blueValue = Double(state[6])

                saber?.resetConnectionExpiration()
                return
            }
        }
        if characteristic.service?.uuid == UUIDs.OTA_SERVICE_UUID {
            if let error = error {
                    print(error)
                    return
                }
            if let data = characteristic.value {
                // deal with incoming data
                // First check if the incoming data is one byte length?
                // if so it's the peripheral acknowledging and telling
                // us to send another batch of data
                if data.count == 1 {
                    if !firstAcknowledgeFromESP32 {
                        firstAcknowledgeFromESP32 = true
                        startTime = CFAbsoluteTimeGetCurrent()
                    }
                    //print("\(Date()) -X-")
                    if transferOngoing {
                        packageCounter = 0
                        writeDataToPeriheral(periphName: saber?.name)
                    }
                }
            }
        }
        if (characteristic.service?.uuid == UUIDs.VERSION_SERVICE_UUID) {
            saber?.getCurrentFirmwareVersion()
        }
        
        
        //print(characteristic)
        self.myPeripherals.first(where: {$0.name == peripheral.name})?.objectWillChange.send()
    }
    
    func ScanAndConnect() {
        print("scanning..")
        centralManager.scanForPeripherals(withServices: nil, options: nil)
    }
    
    func StopScan() {
        print("stopping scan..")
        centralManager.stopScan()
    }
    
    func connectPeriph(_ saber: Saber) {
        print("connecting peroheral")
        centralManager.connect(saber.periph, options: nil)
       // myPeripherals[myPeripherals.count-1].periph.delegate = self
        //myPeripherals[myPeripherals.count-1].isConnected = true
        myPeripherals.first(where: {$0.name == saber.name})?.isConnected = true
        //myPeripherals.first(where: {$0.name == saber.name})?.periph.delegate = self
        //saber.isConnected = true
        //myPeripherals[saber.index-1].ssid = getCharValue(myPeripherals[saber.index-1].periph.services?.first?.characteristics?[1]);
        //myPeripherals[saber.index-1].periph.discoverServices(nil)
    }
    
    func disconnectPeriph(_ saber: Saber) {
        print("disconnecting peroheral")
        centralManager.cancelPeripheralConnection(saber.periph)
        myPeripherals.first(where: {$0.name == saber.name})?.isConnected = false
        //myPeripherals[myPeripherals.count-1].isConnected = false
        
    }
    
    func removePeriph(_ saber: Saber) {
        print("disconnecting peroheral")
        myPeripherals.removeAll(where: {$0.name == saber.name})
        //myPeripherals.remove(at: saber.index-1)
    }
    
    
    func sendFile(name: String) {
        print("\(Date()) FUNC SendFile")
        
        // 1. Get the data from the file(name) and copy data to dataBUffer
        guard let data: Data = try? getBinFileToData(name: name) else {
            print("\(Date()) failed to open file")
            return
        }
        dataBuffer = data
        dataLength = dataBuffer.count
        print("size: \(dataLength)")
        transferOngoing = true
        packageCounter = 0
        // Send the first chunk
        elapsedTime = 0.0
        sentBytes = 0
        firstAcknowledgeFromESP32 = false
        startTime = CFAbsoluteTimeGetCurrent()
        writeDataToPeriheral(periphName: name)
    }
    
    func writeDataToPeriheral(periphName: String?) {
           
           // 1. Get the peripheral and it's transfer characteristic
           guard let discoveredPeripheral = myPeripherals.first(where: {$0.name == periphName}) else {return}
           // ATT MTU - 3 bytes
           chunkSize = discoveredPeripheral.periph.maximumWriteValueLength (for: .withoutResponse) - 3
           // Get the data range
           var range:Range<Data.Index>
           // 2. Loop through and send each chunk to the BLE device
           // check to see if number of iterations completed and peripheral can accept more data
           // package counter allow only "chunkCount" of data to be sent per time.
           while transferOngoing && discoveredPeripheral.periph.canSendWriteWithoutResponse && packageCounter < chunkCount {

               // 3. Create a range based on the length of data to return
               range = (0..<min(chunkSize, dataBuffer.count))
                
               // 4. Get a subcopy copy of data
               let subData = dataBuffer.subdata(in: range)
               
               // 5. Send data chunk to BLE peripheral, send EOF when buffer is empty.
               let characteristic = discoveredPeripheral.periph.services?.first(where: {$0.uuid == UUIDs.OTA_SERVICE_UUID})?.characteristics?.first(where: {$0.uuid == UUIDs.CHARACTERISTIC_OTA_UUID})
               if !dataBuffer.isEmpty {
                   discoveredPeripheral.periph.writeValue(subData, for: characteristic!, type: .withoutResponse)
                   packageCounter += 1
                   print(" Packages: \(packageCounter) bytes: \(subData.count)")
               } else {
                   transferOngoing = false
               }
               
               if discoveredPeripheral.periph.canSendWriteWithoutResponse {
                   print("BLE peripheral ready?: \(discoveredPeripheral.periph.canSendWriteWithoutResponse)")
               }
               
               // 6. Remove already sent data from buffer
               dataBuffer.removeSubrange(range)
               
               // 7. calculate and print the transfer progress in %
               transferProgress = (1 - (Double(dataBuffer.count) / Double(dataLength))) * 100
               print("file transfer: \(transferProgress)%")
               sentBytes = sentBytes + chunkSize
               elapsedTime = CFAbsoluteTimeGetCurrent() - startTime
               let kbPs = Double(sentBytes) / elapsedTime
               kBPerSecond = kbPs / 1000
           }
    }
    
    func getLatestFirmwareVersion() throws -> Data? {
        guard let version = try? Data(contentsOf: URL(string: "https://raw.githubusercontent.com/PaleWind/ota-test/main/versions.json")!) else { return nil }
        let json = try JSONDecoder().decode(FirmwareVersion.self, from: version)
        print("latest version : \(json)")
        return version
    }
    
    func getBinFileToData(name: String) throws -> Data? {
        guard let blob = try? Data(contentsOf: URL(string: "https://github.com/PaleWind/ota-test/raw/main/1.1.1.bin")!) else { return nil }
        let version = try JSONDecoder().decode(FirmwareVersion.self, from: blob)

        print("latest version : \(version)")
//        guard let fileURL = Bundle.main.url(forResource: "update", withExtension: "text") else { return nil }
//        do {
//            let fileData = try Data(contentsOf: fileURL)
//            return Data(fileData)
//        } catch {
//            print("Error loading file: \(error)")
//            return nil
//        }
        return blob
    }
    
}

func getLatestFirmwareVersionD() throws -> FirmwareVersion {
    guard let result = try? Data(contentsOf: URL(string: "https://raw.githubusercontent.com/PaleWind/ota-test/main/versions.json")!) else { return FirmwareVersion(version: "0") }
    print("result: \(result)")
    //let json = try JSONDecoder().decode(FirmwareVersion.self, from: result)
    let latestFirmware = FirmwareVersion(version: "0")

    //print("latest version : \(json)")
    return latestFirmware
}

struct FirmwareVersion : Codable
{
    let version: String
}

struct UUIDs
{
    static let STATE_SERVICE_UUID = CBUUID(string: "172b0aa9-9d49-42c3-a5f7-5eec258b7342")
    static let SSID_SERVICE_UUID = CBUUID(string: "172b0aa9-9723-45c6-94bc-78102bbc9961")
    static let PASSWORD_SERVICE_UUID = CBUUID(string: "0d603309-3610-457e-abdd-b0e12057bdab")
    static let OTA_SERVICE_UUID = CBUUID(string: "0dc6ee5c-9002-412c-8af4-a97aaa994602")
    static let VERSION_SERVICE_UUID = CBUUID(string: "e5166f27-4c4d-4429-88e4-64dae8efc38b")
    
    static let CHARACTERISTIC_TX_UUID = CBUUID(string: "62ec0272-3ec5-11eb-b378-0242ac130003")
    static let CHARACTERISTIC_OTA_UUID = CBUUID(string: "62ec0272-3ec5-11eb-b378-0242ac130005")
}
