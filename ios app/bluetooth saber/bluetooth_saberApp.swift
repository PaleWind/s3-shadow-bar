//
//  bluetooth_saberApp.swift
//  bluetooth saber
//
//  Created by paul wandrei on 7/30/22.
//

import SwiftUI

@main
struct bluetooth_saberApp: App {
    let persistenceController = PersistenceController.shared

    var body: some Scene {
        WindowGroup {
            ContentView()
                .environment(\.managedObjectContext, persistenceController.container.viewContext)
        }
    }
}
