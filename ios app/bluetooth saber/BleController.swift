import CoreBluetooth
import CoreData
import SwiftUI

public var txCharacteristic: CBCharacteristic!
public var rxCharacteristic: CBCharacteristic!
public var periphDevice: CBPeripheral!
public var connected = false;

class Saber: Identifiable, ObservableObject {
    let id = UUID()
    @Published var state = []
    @Published var brightness = 100
    @Published var opMode = 0
    @Published var gain = 0.0
    @Published var squelch = 0.0
    @Published var isConnected = false
    @Published var periph: CBPeripheral!
    var name: String
    @Published var ssid = ""
    
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

}

class BleManager: NSObject, ObservableObject, CBCentralManagerDelegate, CBPeripheralDelegate
{
    var centralManager: CBCentralManager!             //the ios device
    @Published var myPeripherals = [Saber]()         //the ble peripheral device
    
    override init()
    {
        super.init()
        centralManager = CBCentralManager(delegate: self, queue: nil)
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
                
                //self.myPeripherals.last!.periph.delegate = self
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
        print("Value Recieved: \((characteristicASCIIValue as String))")
        
        let valueReceived: String = characteristicASCIIValue as String
        if characteristic.service?.uuid == UUIDs.STATE_SERVICE_UUID {
            print("a state notification was sent")
            //unwrap state characterstic here
            let state = getNumbers(valueReceived)
            
            if state.count == 4 {
                saber?.opMode = state[0]
                saber?.gain   = Double(state[1])
                saber?.squelch = Double(state[2])
                saber?.brightness = state[3]
            }
        }
        
        //print(characteristic)
        self.myPeripherals.first(where: {$0.name == peripheral.name})?.objectWillChange.send()
    }
    
    func ScanAndConnect() {
        print("scanning..")
        myPeripherals.removeAll()
        centralManager.scanForPeripherals(withServices: nil, options: nil)
    }
    
    func StopScan() {
        print("stopping scan..")
        centralManager.stopScan()
    }
    
    func connectPeriph(_ saber: Saber) {
        print("connecting peroheral")
        centralManager.connect(saber.periph, options: nil)
        myPeripherals[myPeripherals.count-1].periph.delegate = self
        myPeripherals[myPeripherals.count-1].isConnected = true
        //saber.isConnected = true
        //myPeripherals[saber.index-1].ssid = getCharValue(myPeripherals[saber.index-1].periph.services?.first?.characteristics?[1]);
        //myPeripherals[saber.index-1].periph.discoverServices(nil)
    }
    
    func disconnectPeriph(_ saber: Saber) {
        print("disconnecting peroheral")
        centralManager.cancelPeripheralConnection(saber.periph)
        myPeripherals[myPeripherals.count-1].isConnected = false
        //myPeripherals.remove(at: saber.index-1)
    }
    /*func writeOutgoingValue(_ data: String)
    {
        let valueString = (data as NSString).data(using: String.Encoding.utf8.rawValue)
        if let myPeripheral = myPeripheral
        {
          if let txCharacteristic = txCharacteristic
            {
              myPeripheral.writeValue(valueString!, for: txCharacteristic, type: CBCharacteristicWriteType.withResponse)
              }
          }
      }*/
    

}


struct UUIDs
{
    static let STATE_SERVICE_UUID = CBUUID(string: "4fafc201-1fb5-459e-8fcc-c5c9c331914b")
    static let SSID_SERVICE_UUID = CBUUID(string: "3c662598-3367-489d-ad3f-484ec8970642")
    static let PASSWORD_SERVICE_UUID = CBUUID(string: "2d804258-a9ee-4f2d-afe6-eb005aae21ad")
}
