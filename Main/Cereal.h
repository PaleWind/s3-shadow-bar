
//void setssid(string ssid)
//{
//  preferences.putString("ssid", ssid); 
//  strcpy(ssid, ssid.c_str());//to,from
//}
//
//void setpassword(String )
//{
//  strcpy(password, cerealBuffer);
//}

void executeCommand(int command)
{
  try 
  {
    byte commandGroup = command / 1000; 
    byte commandVal = command % 1000;
    Serial.print("commandGroup: ");Serial.println(commandGroup);
    if (commandGroup == 17)
    {
      WiFi.mode(WIFI_OFF);
      return;
    }
    if (commandGroup == 18)
    {
      WiFi.mode(WIFI_OFF);
      return;
    }
    if (commandGroup == 19)
    {
      ConnectWifi(); //maybe a proomise to alert the phone app? or jusut notify? idk im tired
      return;
    }

    //Serial.println(commandGroup);
    //Serial.println(commandVal);
    switch (commandGroup)
    {
      case 1: // gain control
        gain = commandVal;
        Serial.print("gain: ");Serial.println(gain);
        break;
        
      case 2: // squelch control
        squelch = commandVal;
        Serial.print("squelch: ");Serial.println(squelch);
        break;
        
      case 3: // mode control
        Serial.print("commandVal: ");Serial.println(commandVal);

        if (commandVal <= patternsListSize) 
        { 
          FastLED.clear();
          FastLED.show();
          opMode = commandVal; 
        }
        Serial.print("opMode: ");Serial.println(opMode);
        break;
        
      case 4: // brightness control
      Serial.println("brightness commanmd");
      Serial.println(commandVal);
        effectBrightness = commandVal;
        FastLED.show();
        break;
        
      case 5: // bpm control
        bpm = commandVal;
        break;
//      case 14: // color control
//            if (currentPalette != paletteSize && commandVal == 1) {currentPalette++;}
//       else if (currentPalette != 0           && commandVal == 0) {currentPalette--;}
//        break;
//        

    }
    
  }
  catch (...) 
  { 
    Serial.println("error, captain");
  }
}
