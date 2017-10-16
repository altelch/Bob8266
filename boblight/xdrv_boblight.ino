/*
  xdrv_boblight.ino - boblight support for Sonoff-Tasmota

  Copyright (C) 2017  Heiko Krupp

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

void pollBob()
{
  //check if there are any new clients
  if (bob.hasClient()){
    //find free/disconnected spot
    if (!bobClient || !bobClient.connected()){
      if(bobClient) bobClient.stop();
      bobClient = bob.available();
    }
    //no free/disconnected spot so reject
    WiFiClient bobClient = bob.available();
    bobClient.stop();
  }
  
  //check clients for data
  if (bobClient && bobClient.connected()){
    if(bobClient.available()){
      //get data from the client
      while(bobClient.available()){
        String input=bobClient.readStringUntil('\n');
        //Serial.print("Client: "); Serial.println(input);
        if (input.startsWith("hello")) {
          bobClient.print("hello\n");
        }
        else if (input.startsWith("ping")) {
          bobClient.print("ping 1\n");
        }
        else if (input.startsWith("get version")) {
          bobClient.print("version 5\n");
        }
        else if (input.startsWith("get lights")) {
/*          String answer="Lights ";
          answer.concat(sysCfg.led_pixels);
          answer.concat("\n");
          bobClient.print(answer);
          uint8_t x=sysCfg.led_pixels/4.0*(16.0/9.0); // 16:9
          uint8_t y=sysCfg.led_pixels/4.0*(9.0/16.0); // 16:9
          Serial.println(x);
          Serial.println(y);
          for(int bottom_r=ceil(x/2);bottom_r>0;bottom_r--) {
            answer=("light BottomRight");
            answer.concat(bottom_r);
            answer.concat("\n");
            bobClient.print(answer);
          } */
          String answer="lights 25\n";
          answer.concat("light 0 scan 83.33 100 33.33 44.44\n");
          answer.concat("light 1 scan 83.33 100 22.22 33.33\n");
          answer.concat("light 2 scan 83.33 100 11.11 22.22\n");
          answer.concat("light 3 scan 83.33 100 0 11.11\n");
          answer.concat("light 4 scan 66.67 83.33 0 11.11\n");
          answer.concat("light 5 scan 50 66.67 0 11.11\n");
          answer.concat("light 6 scan 33.33 50 0 11.11\n");
          answer.concat("light 7 scan 16.67 33.33 0 11.11\n");
          answer.concat("light 8 scan 0 16.67 0 11.11\n");
          answer.concat("light 9 scan 0 16.67 11.11 22.22\n");
          answer.concat("light 10 scan 0 16.67 22.22 33.33\n");
          answer.concat("light 11 scan 0 16.67 33.33 44.44\n");
          answer.concat("light 12 scan 0 16.67 44.44 55.56\n");
          answer.concat("light 13 scan 0 16.67 55.56 66.67\n");
          answer.concat("light 14 scan 0 16.67 66.67 77.78\n");
          answer.concat("light 15 scan 0 16.67 77.78 88.89\n");
          answer.concat("light 16 scan 0 16.67 88.89 100\n");
          answer.concat("light 17 scan 16.67 33.33 88.89 100\n");
          answer.concat("light 18 scan 33.33 50 88.89 100\n");
          answer.concat("light 19 scan 50 66.67 88.89 100\n");
          answer.concat("light 20 scan 66.67 83.33 88.89 100\n");
          answer.concat("light 21 scan 83.33 100 88.89 100\n");
          answer.concat("light 22 scan 83.33 100 77.78 88.89\n");
          answer.concat("light 23 scan 83.33 100 66.67 77.78\n");
          answer.concat("light 24 scan 83.33 100 55.56 66.67\n");
          bobClient.print(answer);
        }
        else if (input.startsWith("set priority")) {
          sysCfg.led_scheme=0;
          Serial.println(input);
        }
        else if (input.startsWith("set light ")) { // <id> <cmd in rgb, speed, interpolation> <value> ...
          input.remove(0,10);
          String tmp=input.substring(0,input.indexOf(' '));
          uint16_t light_id=tmp.toInt();
          input.remove(0,input.indexOf(' ')+1);
          if (input.startsWith("rgb ")) {
            input.remove(0,4);
            tmp=input.substring(0,input.indexOf(' '));
            uint8_t red=(uint8_t)(255.0f*tmp.toFloat());
            input.remove(0,input.indexOf(' ')+1);        // remove first float value
            tmp=input.substring(0,input.indexOf(' '));
            uint8_t green=(uint8_t)(255.0f*tmp.toFloat());
            input.remove(0,input.indexOf(' ')+1);        // remove second float value
            tmp=input.substring(0,input.indexOf(' '));
            uint8_t blue=(uint8_t)(255.0f*tmp.toFloat());
            ws2812_setBobColor(light_id, red, green, blue);
          } // currently no support for interpolation or speed, we just ignore this
        }
        else if (input.startsWith("sync")) {
          ws2812_BobSync();
        }        
        else {
          // Client sent gibberish
          bobClient.stop();
          bobClient = bob.available();
        }
      }
    }
  }
}
