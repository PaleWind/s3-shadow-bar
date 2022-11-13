//
//  ModeSelectView.swift
//  bluetooth saber
//
//  Created by paul wandrei on 10/31/22.
//

import SwiftUI

struct ModeSelectView: View {
    
    @ObservedObject var data: Saber

    var body: some View {
        ScrollView {
                VStack {
                    ForEach(Array(data.modes.enumerated()), id: \.element) { index, element in

                        Button(element) {
                            data.writeOutgoingValue(String(3000 + index))
                        }
                        .padding()
                        .background(.blue)
                        .foregroundColor(.white)
                        .clipShape(Capsule())
                    }
                }
        }
    }
}

