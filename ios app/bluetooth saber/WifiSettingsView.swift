//
//  WifiSettingsView.swift
//  bluetooth saber
//
//  Created by paul wandrei on 10/31/22.
//

import SwiftUI

struct WifiSettingsView: View {
    
    @ObservedObject var data: Saber
    
    var body: some View {
        VStack {
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
            Text(getCharValue(data.periph.services?.first(where: {$0.uuid == UUIDs.PASSWORD_SERVICE_UUID})?.characteristics?.first?.value))
            
            HStack {
                Text("Password")
                Spacer()
                TextField(
                    getCharValue(data.periph.services?.first(where: {$0.uuid == UUIDs.PASSWORD_SERVICE_UUID})?.characteristics?.first?.value),
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
            Text(getCharValue(data.periph.services?.first(where: {$0.uuid == UUIDs.PASSWORD_SERVICE_UUID})?.characteristics?.first?.value))
        }
    }
}

