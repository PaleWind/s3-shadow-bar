//
////void setssid(string ssid)
////{
////  preferences.putString("ssid", ssid); 
////  strcpy(ssid, ssid.c_str());//to,from
////}
////
////void setpassword(String )
////{
////  strcpy(password, cerealBuffer);
////}
//
//void executeCommand(int command)
//{
//  try 
//  {
//    byte commandGroup = command / 1000; 
//    byte commandVal = command % 1000;
//    Serial.print("commandGroup: ");Serial.println(commandGroup);
//    if (commandGroup == 17)
//    {
//      WiFi.mode(WIFI_OFF);
//      return;
//    }
//    if (commandGroup == 18)
//    {
//      WiFi.mode(WIFI_OFF);
//      return;
//    }
//    if (commandGroup == 19)
//    {
//      ConnectWifi(); //maybe a proomise to alert the phone app? or jusut notify? idk im tired
//      return;
//    }
//
//    //Serial.println(commandGroup);
//    //Serial.println(commandVal);
//    switch (commandGroup)
//    {
//      case 11: // gain control
//        gain = commandVal;
//        Serial.print("gain: ");Serial.println(gain);
//        break;
//        
//      case 12: // squelch control
//        squelch = commandVal;
//        Serial.print("squelch: ");Serial.println(squelch);
//        break;
//        
//      case 13: // mode control
//        Serial.print("commandVal: ");Serial.println(commandVal);
//
//        if (commandVal == 15) 
//        { 
//          FastLED.clear();
//          FastLED.show();
//          opMode = commandVal;
//          artnet.setArtDmxCallback(artnetPixelMap); 
//        }
//        else if (commandVal == 16) 
//        { 
//          FastLED.clear();
//          FastLED.show(); 
//          opMode = commandVal;
//          artnet.setArtDmxCallback(artnetFourChannel);
//        }
//        else if (commandVal <= numModes) 
//        { 
//          FastLED.clear();
//          FastLED.show();
//          opMode = commandVal; 
//        }
//        Serial.print("opMode: ");Serial.println(opMode);
//        break;
//        
//      case 14: // color control
//            if (currentPalette != paletteSize && commandVal == 1) {currentPalette++;}
//       else if (currentPalette != 0           && commandVal == 0) {currentPalette--;}
//        break;
//        
//      case 15: // brightness control
//        effectBrightness = commandVal;
//        FastLED.show();
//        break;
//        
//      case 16: // bpm control
//        bpm = commandVal;
//        break;
//    }
//    
//  }
//  catch (...) 
//  { 
//    Serial.println("error, captain");
//  }
//}
