/*
* A version of my word-clock software for esp8266 without MQTT
*
* Wiring:
* Momentary button between D1 and GND
* NeoPixel Data on GPIO3/D9
*/
#include <FS.h>
#include <time.h>

// to set up wifi connectivity to keep time updated
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>

// to run the light show!
#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>
#include <words.hpp>
#include <SimpleTimer.h>

// DEFINITIONS FOR WIFI

// var to save the timezone config from the WiFiManager UI
// default timezone
char timezone[3] = "-5"; // not sure what format to save in - probably should be a signed int
bool shouldSaveConfig = false; // this flag is true if we have new values we need to save to file system
WiFiManager wifiManager; // must be global so we can perform a reset later

// DEFINITIONS FOR NEOPIXELS

NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(100,2); // set up how many lights our strip has
NeoTopology<RowMajorAlternating180Layout> topo(10, 10); // set up a grid topography so we can address as x,y
NeoPixelAnimator animator(1, NEO_MILLISECONDS); // create an animator with 1 channel and a timescale

RgbColor clockColor(90, 80, 60); // set a color. TODO: make it customizable

ClockPhrase lastPhrase; // initialized in the setup routine
ClockPhrase currentPhrase; // init in the setup routine

SimpleTimer timer; // timer so we can track when to update the clock
int timerId; // stores the timer ID so that we can restart it as needed.

// FUNCTIONS FOR WIFI

// callback to set a flag to tell the setup() to save the config.
void saveConfigCallback() {
  Serial.println("shouldSaveConfig = true");
  shouldSaveConfig = true;
}

// FUNCTIONS FOR NEOPIXELS

// update a word to current color in animation. Is called by phraseAnimUpdate for each word in the phrase
void wordAnimUpdate(ClockWord theWord, RgbColor startColor, RgbColor endColor, float progress) {
  RgbColor setColor = RgbColor::LinearBlend(startColor, endColor, progress);
  for (size_t i = theWord.start; i <= theWord.end; i++) {
    strip.SetPixelColor(topo.Map(theWord.row, i), setColor);
  }
}

// update the phrase to current color in animation. Is called by clockAnimUpdate to animate the old or new time phrase
void phraseAnimUpdate(ClockPhrase phrase, float progress){
// for each word in the phrase
  for (size_t i = 0; i < phrase.size; i++) {
    wordAnimUpdate(phrase.words[i], phrase.startColor, phrase.endColor, progress);
  }
}

// update the clock face in the animation. Called on every tick of the animator
void clockAnimUpdate(AnimationParam params){
  if (params.progress < .5f) { // IF progress < 50%, fade out last words
    float progress = NeoEase::ExponentialIn(params.progress * 2.0f);
    phraseAnimUpdate(lastPhrase, progress);
  } else if (params.progress > .5f) { // IF progress > 50%, fade in new words
    float progress = NeoEase::ExponentialOut( ( params.progress - .5f) * 2.0f );
    phraseAnimUpdate(currentPhrase, progress);
  }
}

// called every 30 seconds by the timer
void timeTick() {
  Serial.println("TICK TOCK");

  Serial.print("Now: ");

  time_t current_time = 0;
  time(&current_time);
  Serial.print(ctime(&current_time));

  Serial.print("Expires: ");
  Serial.println(currentPhrase.expires);

  if (current_time >= currentPhrase.expires) {
    Serial.println("NEW TIME");
    lastPhrase = currentPhrase;
    lastPhrase.startColor = clockColor;
    lastPhrase.endColor = RgbColor(0);

    currentPhrase.expires = current_time + (5 * 60); //new time will be due to be replaced in 5 minutes;
    Serial.print("New expires: ");
    Serial.println(currentPhrase.expires);


    short i = 0;
    currentPhrase.words[i++] = WORD_PREFIX::IT;
    currentPhrase.words[i++] = WORD_PREFIX::IS;

    struct tm *current_time_tm = localtime(&current_time);

    // first the minutes...
    switch (current_time_tm->tm_min) {
      case 58 ... 60:
      case 0 ... 2:
        // no minute word around o'clock
        break;
      case 3 ... 7:
        currentPhrase.words[i++] = WORD_PREFIX::FIVE;
        currentPhrase.words[i++] = WORD_PREFIX::PAST;
        break;
      case 8 ... 12:
        currentPhrase.words[i++] = WORD_PREFIX::TEN;
        currentPhrase.words[i++] = WORD_PREFIX::PAST;
        break;
      case 13 ... 17:
        currentPhrase.words[i++] = WORD_PREFIX::QUARTER;
        currentPhrase.words[i++] = WORD_PREFIX::PAST;
        break;
      case 18 ... 22:
        currentPhrase.words[i++] = WORD_PREFIX::TWENTY;
        currentPhrase.words[i++] = WORD_PREFIX::PAST;
        break;
      case 23 ... 27:
        currentPhrase.words[i++] = WORD_PREFIX::TWENTYFIVE;
        currentPhrase.words[i++] = WORD_PREFIX::FIVE; // ONLY BECAUSE SOMEONE MADE A MISTAKE WITH THE CLOCK FACE LAYOUT!!!
        currentPhrase.words[i++] = WORD_PREFIX::PAST;
        break;
      case 28 ... 32:
        currentPhrase.words[i++] = WORD_PREFIX::HALF;
        currentPhrase.words[i++] = WORD_PREFIX::PAST;
        break;
      case 33 ... 37:
        currentPhrase.words[i++] = WORD_PREFIX::TWENTYFIVE;
        currentPhrase.words[i++] = WORD_PREFIX::FIVE; // AGAIN DUE TO THE CLOCK FACE LAYOUT MISTAKE
        currentPhrase.words[i++] = WORD_PREFIX::TO;
        break;
      case 38 ... 42:
        currentPhrase.words[i++] = WORD_PREFIX::TWENTY;
        currentPhrase.words[i++] = WORD_PREFIX::TO;
        break;
      case 43 ... 47:
        currentPhrase.words[i++] = WORD_PREFIX::QUARTER;
        currentPhrase.words[i++] = WORD_PREFIX::TO;
        break;
      case 48 ... 52:
        currentPhrase.words[i++] = WORD_PREFIX::TEN;
        currentPhrase.words[i++] = WORD_PREFIX::TO;
        break;
      case 53 ... 57:
        currentPhrase.words[i++] = WORD_PREFIX::FIVE;
        currentPhrase.words[i++] = WORD_PREFIX::TO;
        break;
    }

    // then the hour. tricky tricky because it is actually based on the  minutes!
    switch (current_time_tm->tm_min) {

      case 0 ... 32: // in first part of hour, clock refers to current hour (PAST 5 oclock)
        switch (current_time_tm->tm_hour) {
          case 1:
          case 13:
            currentPhrase.words[i++] = WORD_HOUR::ONE;
            break;
          case 2:
          case 14:
            currentPhrase.words[i++] = WORD_HOUR::TWO;
            break;
          case 3:
          case 15:
            currentPhrase.words[i++] = WORD_HOUR::THREE;
            break;
          case 4:
          case 16:
            currentPhrase.words[i++] = WORD_HOUR::FOUR;
            break;
          case 5:
          case 17:
            currentPhrase.words[i++] = WORD_HOUR::FIVE;
            break;
          case 6:
          case 18:
            currentPhrase.words[i++] = WORD_HOUR::SIX;
            break;
          case 7:
          case 19:
            currentPhrase.words[i++] = WORD_HOUR::SEVEN;
            break;
          case 8:
          case 20:
            currentPhrase.words[i++] = WORD_HOUR::EIGHT;
            break;
          case 9:
          case 21:
            currentPhrase.words[i++] = WORD_HOUR::NINE;
            break;
          case 10:
          case 22:
            currentPhrase.words[i++] = WORD_HOUR::TEN;
            break;
          case 11:
          case 23:
            currentPhrase.words[i++] = WORD_HOUR::ELEVEN;
            break;
          case 0:
          case 12:
            currentPhrase.words[i++] = WORD_HOUR::TWELVE;
            break;
        }

        break;

      case 33 ... 60: // second half of the hour, time refers to
        switch (current_time_tm->tm_hour) {
          case 1:
          case 13:
            currentPhrase.words[i++] = WORD_HOUR::TWO;
            break;
          case 2:
          case 14:
            currentPhrase.words[i++] = WORD_HOUR::THREE;
            break;
          case 3:
          case 15:
            currentPhrase.words[i++] = WORD_HOUR::FOUR;
            break;
          case 4:
          case 16:
            currentPhrase.words[i++] = WORD_HOUR::FIVE;
            break;
          case 5:
          case 17:
            currentPhrase.words[i++] = WORD_HOUR::SIX;
            break;
          case 6:
          case 18:
            currentPhrase.words[i++] = WORD_HOUR::SEVEN;
            break;
          case 7:
          case 19:
            currentPhrase.words[i++] = WORD_HOUR::EIGHT;
            break;
          case 8:
          case 20:
            currentPhrase.words[i++] = WORD_HOUR::NINE;
            break;
          case 9:
          case 21:
            currentPhrase.words[i++] = WORD_HOUR::TEN;
            break;
          case 10:
          case 22:
            currentPhrase.words[i++] = WORD_HOUR::ELEVEN;
            break;
          case 11:
          case 23:
            currentPhrase.words[i++] = WORD_HOUR::TWELVE;
            break;
          case 12:
          case 0:
            currentPhrase.words[i++] = WORD_HOUR::ONE;
            break;
        }
        break;
    }

    currentPhrase.size = i;

    animator.StartAnimation(0, 50000, clockAnimUpdate);
  }

}

void setup() {
  Serial.begin(115200);
  Serial.println();

  // BEGIN LIGHT SETUP

  Serial.println("Beginning light setup...");
  strip.Begin();
  Serial.println("Strip begun!");
  strip.Show();
  Serial.println("Strip shown");

  timerId = timer.setInterval(30 * 1000, timeTick); // call to check if we need to update or not, every 30 seconds

  // set up the current and last phrases
  lastPhrase.startColor = RgbColor(0);
  lastPhrase.endColor = RgbColor(0);
  lastPhrase.size = 0;
  lastPhrase.expires = 0;

  currentPhrase.startColor = RgbColor(0);
  currentPhrase.endColor = clockColor;
  currentPhrase.size = 0;
  lastPhrase.expires = 0;

  // END LIGHT SETUP

  // Set up the reset button
  pinMode(D1, INPUT_PULLUP);

  //read configuration from FileSystem json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");

          strcpy(timezone, json["timezone"]);

        } else {
          Serial.println("failed to load json config");
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  //end read

  // start the WiFi manager - will start an access point to configure device if needed
  WiFiManagerParameter custom_timezone("timezone", "timezone offset (e.g. -5 for EST, -4 for EDT)", timezone, 3);

  wifiManager.addParameter(&custom_timezone);

  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect("Word Clock Setup")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }

  Serial.println("WiFi connected");

  strcpy(timezone, custom_timezone.getValue());

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["timezone"] = timezone;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
  }

  // set up time
  int timezone_int = strtol(timezone, NULL, 0);
  time_t this_second = 0;

  // set the clock. UTC offset, Daylight savings time offset (don't know if/how it works), up to three time servers
  configTime(timezone_int * 3600, timezone_int * 3600, "pool.ntp.org", "time.nist.gov");
  while(!this_second)
  {
     time(&this_second);
     Serial.print("-");
     delay(100);
  }
  Serial.print(ctime(&this_second));

  Serial.println("Done setup!");

}

void loop(){
  // If the button is pressed, reset the WiFiManager
  if(digitalRead(D1) == LOW) {
    Serial.println("BUTTON PRESSED");
    wifiManager.resetSettings();
    ESP.reset();
  }

  timer.run();
  animator.UpdateAnimations();
  strip.Show();
}
