//
//  ContentView.swift
//  bluetooth saber
//
//  Created by paul wandrei on 7/30/22.
//

import SwiftUI
import CoreBluetooth

struct ContentView: View {
    
    @StateObject var bleController = BleManager()
    @State var scanning = false
    
    var body: some View {
        
        VStack {
                //Spacer()
                Button(action: {
                  if scanning {
                      bleController.StopScan()
                  }
                  else {
                      bleController.ScanAndConnect()
                  }
                  scanning.toggle()
                  }) {
                    Image(systemName: scanning ? "antenna.radiowaves.left.and.right.slash" : "antenna.radiowaves.left.and.right")
                        //.renderingMode(.original)
                        .resizable()
                        .aspectRatio(contentMode: .fit)
                        .frame(width: 50, height: 50)
                        //.padding(.trailing, 30)
                      Text(scanning ? "Stop" : "Scan")
                }
      /*      Button("Scan") {
                bleController.ScanAndConnect()
            }
            Spacer()
            Button("Stop Scan") {
                bleController.StopScan()
            }*/
            NavigationView {
                List(bleController.myPeripherals) { saber in
                    NavigationLink (destination: listedPeripheralsView(data: saber)) {
                        Text(saber.name)
                    }
                }
            }
            .environmentObject(bleController)
        }
    }
}

struct listedPeripheralsView: View {

    @EnvironmentObject var bleController: BleManager
    @StateObject var data: Saber	
    
    var body: some View {
        VStack {
            HStack {
                Button(action: {
                    if data.periph.state == CBPeripheralState.connected{
                        bleController.disconnectPeriph(data)
                    } else {
                        bleController.connectPeriph(data)
                    }
                    })  {
                        Text(data.isConnected ? "Disconnect" : "Connect")
                    }
            }
            if data.periph.state == CBPeripheralState.connected{
                
                Text("Name: \(data.periph.name!)")
//                if let state = getNumbers(data.periph.services?.first(where: {$0.uuid == UUIDs.STATE_SERVICE_UUID})?.characteristics?.first?.value){
//                    ForEach(0 ..< state.count, id: \.self) { value in
//                        Text("\(value) : \(state[value])")
//                    }
//                }
                VStack {
                    HStack {
                        Text("Gain ")
                        Slider(value: $data.gain, in: 0...30, step: 1) { editing in
                            data.writeOutgoingValue("\(1000 + data.gain)")
                        }
                    }
                    HStack {
                        Text("Squelch ")
                        Slider(value: $data.squelch, in: 0...30, step: 1) { editing in
                            data.writeOutgoingValue("\(2000 + data.squelch)")
                        }
                    }
                }
                HStack {
                    Button("Mode") {
                        data.writeOutgoingValue("3000")
                    }
                    Button("Color") {
                        data.writeOutgoingValue("4000")
                    }
                }
                HStack {
                    Text("SSID")
                    Spacer()
                    TextField(
                        getCharValue(data.periph.services?.first(where: {$0.uuid == UUIDs.SSID_SERVICE_UUID})?.characteristics?.first?.value),
                        text: $data.ssid
                    )
                    .onSubmit {
                        print(data.ssid)
                        data.writeOutgoingSsid(data.ssid)
                    }
                    .textInputAutocapitalization(.never)
                    .disableAutocorrection(true)
                    .border(.secondary)
                }
                Text(getCharValue(data.periph.services?.first(where: {$0.uuid == UUIDs.SSID_SERVICE_UUID})?.characteristics?.first?.value))
            }
        }
    }
}

func getCharValue(_ data: Data?)  -> String {
    var characteristicASCIIValue = NSString()
    guard let characteristicValue = data else {
          return "Loading.."
      }
    let ASCIIstring = NSString(data: characteristicValue, encoding: String.Encoding.utf8.rawValue)
    characteristicASCIIValue = ASCIIstring ?? ""
    print("Value converted: \((characteristicASCIIValue as String))")
    return characteristicASCIIValue as String
}

func getNumbers(_ csv_string: String) -> Array<Int> {
    let result =  csv_string
        .components(separatedBy: ",")
        .compactMap {
            Int($0.trimmingCharacters(in: .whitespaces))
        }
    print("result: \(result)" )
    return result
}

struct ContentView_Previews: PreviewProvider {
    static var previews: some View {
        ContentView().environment(\.managedObjectContext, PersistenceController.preview.container.viewContext)
    }
}
