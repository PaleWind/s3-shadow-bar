//
//  ColorSelectView.swift
//  bluetooth saber
//
//  Created by paul wandrei on 11/11/22.
//

import SwiftUI

struct ColorSelectView: View {
    
    @ObservedObject var saber: Saber
    
    var body: some View {
        
        NavigationView {
            VStack   {
                NavigationLink (destination: RgbSelectView(saber: saber)) {
                    Text("RGB")
                }
                NavigationLink (destination: PaletteSelectView(saber: saber)) {
                    Text("Palette")
                }
            }
        }
        
    }
}

struct RgbSelectView: View {
    
    @ObservedObject var saber: Saber
    
    var body: some View {
        
        HStack {
            Text("Brightness ")
            Slider(value: $saber.brightness, in: 0...250, step: 1) { editing in
                saber.writeOutgoingValue("\(4000 + saber.brightness)")
            }
        }
        
        HStack {
            Text("Red ")
            Slider(value: $saber.redValue, in: 0...255, step: 1).onChange(of: saber.redValue) { val in
                saber.writeOutgoingValue("\(6000 + saber.redValue)")
            }
                
            
        }

        HStack {
            Text("Green ")
            Slider(value: $saber.greenValue, in: 0...255, step: 1) { editing in
                saber.writeOutgoingValue("\(6256 + saber.greenValue)")
            }
        }

        HStack {
            Text("Blue ")
            Slider(value: $saber.blueValue, in: 0...255, step: 1) { editing in
                saber.writeOutgoingValue("\(6512 + saber.blueValue)")
            }
        }
        
        ScrollView {
            VStack   {
                ForEach(Array(saber.colors.enumerated()), id: \.element) { index, element in

                    Button(element) {
                        saber.writeOutgoingValue(String(5000 + index))
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

struct PaletteSelectView: View {
    
    @ObservedObject var saber: Saber
    
    var body: some View {
        
        ScrollView {
            VStack   {
                ForEach(Array(saber.palettes.enumerated()), id: \.element) { index, element in

                    Button(element) {
                        saber.writeOutgoingValue(String(    5000 + index))
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


extension Color {
    var components: (red: CGFloat, green: CGFloat, blue: CGFloat, opacity: CGFloat) {

        var r: CGFloat = 0
        var g: CGFloat = 0
        var b: CGFloat = 0
        var o: CGFloat = 0

        guard UIColor(self).getRed(&r, green: &g, blue: &b, alpha: &o) else {
            // You can handle the failure here as you want
            return (0, 0, 0, 0)
        }

        return (r, g, b, o)
    }
}

extension Double {
    /// Rounds the double to decimal places value
    func rounded(toPlaces places:Int) -> Double {
        let divisor = pow(10.0, Double(places))
        return (self * divisor).rounded() / divisor
    }
}


