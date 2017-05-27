# ESP8266 WiFi-enabled Word Clock with ws2812b LED strips
## Key features
* 10x10 grid made of 10 strips of 10 LEDs
* NTP time synchronization
* Reset button to enter timezone and wifi configuration mode
* Words are not hardcoded (although references are) - words (and their grid positions) can be edited in the [words.hpp](src/words.hpp) file.

## How to construct the word clock

1. Buy a small square Ikea frame, just over an inch deep.
2. Create a vinyl sticker for the clock face. Mine was cut by [Sticker Genius](https://stickergenius.com)
  * The white sticker works well but is still a bit too translucent.
3. Buy a strip of ws2812b LEDs.
  * Cut into 10-led lengths
  * Solder the strips together with wires to form a grid. The wires should flow through the grid in an S-shape.
  * Make sure the strips will line up with the letters from your sticker, then glue to a piece of cardboard (not the frame backing - this makes it easier to change it up later.)
4. Wire up a momentary button between D1 and GND
5. Wire up the light strip to +5v, GND, and the data wire to RX0
6. Flash the code to the ESP8266. Reset the ESP8266 by the standard reset button if needed.
7. When the ESP8266 reboots it will load the WiFi connection configuration mode. It will launch a WiFi access point, which you can then use to set your WiFi network SSID and password. You can also set your timezone offset relative to UTC.
  * Use numbers like -5 (EST) or 1 (CET)
  * Use the momentary switch to reset the WiFi+timezone configuration, such as when you need to change the UTC offset due to daylight savings time.

## Current issues
* The time displayed may be up to five minutes off from where it should be.

## Credits
* [NeoPixelBus](https://github.com/Makuna/NeoPixelBus) library used to control the LED strip
* The setup and WiFi+timezone configuration storage is largely based on an example from the [WiFiManager](https://github.com/tzapu/WiFiManager) library, which of course is also the library providing the captive portal.
