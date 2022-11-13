//
//  Saber.swift
//  bluetooth saber
//
//  Created by paul wandrei on 11/11/22.
//

import Foundation
import CoreBluetooth
import SwiftUI

class Saber: Identifiable, ObservableObject {
    var name: String
    var id: UUID
    @Published var periph: CBPeripheral!
    
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
    @Published var stripColor = Color(.sRGB, red: 0.98, green: 0.9, blue: 0.2)
    @Published var isConnected = false
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
    
    @Published var palettes = ["Solid Color",
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
    
    @Published var colors = ["Red",
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
    init (periph:CBPeripheral!, name: String, id: UUID) {
        self.periph = periph
        self.name = name
        self.id = id
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

    func writeOutgoingDeviceName(_ data: String) {
        let valueString = (data as NSString).data(using: String.Encoding.utf8.rawValue)
        if let myPeripheral = periph
        {
            if let txCharacteristic = myPeripheral.services?.first(where: {$0.uuid == UUIDs.SSID_SERVICE_UUID})?.characteristics?.first(where: {$0.uuid == UUIDs.NAME_CHARACTERISTIC_UUID})
            {
                print(valueString as Any)
                myPeripheral.writeValue(valueString!, for: txCharacteristic, type: CBCharacteristicWriteType.withResponse)
            }
        }
    }
    
    func writeOutgoingSsid(_ data: String) {
        let valueString = (data as NSString).data(using: String.Encoding.utf8.rawValue)
        if let myPeripheral = periph
        {
            if let txCharacteristic = myPeripheral.services?.first(where: {$0.uuid == UUIDs.SSID_SERVICE_UUID})?.characteristics?.first(where: {$0.uuid == UUIDs.SSID_CHARACTERISTIC_UUID})
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
