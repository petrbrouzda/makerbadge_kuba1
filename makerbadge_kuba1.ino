/**
FQBN: esp32:esp32:esp32s2:CDCOnBoot=cdc,CPUFreq=80,FlashFreq=40

esp32s2 dev module
80 mhz
40 mhz
usb-cdc enabled

*/

// na startu ceka, az se seriovy port urcite spoji - to trva cca 2 sec
// #define LOGUJ_DEBUG

#include "driver/rtc_io.h"
#include <Adafruit_NeoPixel.h>

#define ENABLE_GxEPD2_GFX 0 // we won't need the GFX base class
#include <GxEPD2_BW.h>

// Online tool for converting images to byte arrays:
// https://javl.github.io/image2cpp/

#include "Bitmap.h"

// kompenzovane cteni z ADC, presnejsi
#include <ESP32AnalogRead.h>



// ESP32-S2 e-ink pinout
#define BUSY 42
#define RST  39
#define DC   40
#define CS   41
#define PWR  16

// Instantiate the GxEPD2_BW class for our display type
GxEPD2_BW<GxEPD2_213_B74, GxEPD2_213_B74::HEIGHT> display(GxEPD2_213_B74(CS, DC, RST, BUSY)); // GDEM0213B74 128x250, SSD1680


// neopixels
#define ledPin    18
// zapina/vypina napajeni pro LED; LOW=zapnuto
#define ledPower  21
// How many NeoPixels are attached to the board?
#define NUMPIXELS 4

Adafruit_NeoPixel strip(NUMPIXELS, ledPin, NEO_GRB + NEO_KHZ800);




// boot pin
#define BOOT_PIN GPIO_NUM_0

// LOW=zapnuto
#define MERENI_BATERKY_ZAP 14
// pripojeno delicem 10k/10k
#define MERENI_BATERKY_ANALOG 6

ESP32AnalogRead adc6;


// touch
#define Threshold 5000    /* Lower the value, more the sensitivity */
RTC_DATA_ATTR int bootCount = 0;
touch_pad_t touchPin;


#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  180        /* Time ESP32 will go to sleep (in seconds) */
#define TIME_TO_SLEEP_LOWBATT  1800L


RTC_DATA_ATTR int activePage = 1;


bool wakeupTimer = false;
bool wakeupTouch = false;
bool wakeupPin0 = false;

void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : 
      wakeupPin0=true;
      Serial.println("Wakeup caused by external signal using RTC_IO"); 
      break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER :
      Serial.println("Wakeup caused by timer"); 
      wakeupTimer=true;
      break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : 
      Serial.println("Wakeup caused by touchpad"); 
      wakeupTouch=true;
      print_wakeup_touchpad();
      break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}

void print_wakeup_touchpad(){
  touchPin = esp_sleep_get_touchpad_wakeup_status();
  Serial.printf("Touch detected on %d\n", touchPin );
}


float mapF(float x, float in_min, float in_max, float out_min, float out_max) {
  if( x > in_max ) return out_max;
  if( x < in_min ) return out_min;
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

float kompBaterka = 4.15/4.17;
float vBaterka = 0;
int stavBaterky = 0;


void zmerNapeti(void)
{
  delay(50);
  digitalWrite( MERENI_BATERKY_ZAP, LOW );
  zmerNapetiInt();
  digitalWrite( MERENI_BATERKY_ZAP, HIGH );
}

void zmerNapetiInt(void)
{
  // int x = analogRead(MERENI_BATERKY_ANALOG);
  // Serial.printf( "Baterka: %d\n", x );

  vBaterka = adc6.readVoltage() / (10.0/20.0) * kompBaterka;   
  Serial.printf( "Baterka: %.2f V\n", vBaterka );

  stavBaterky = (int) mapF( vBaterka, 3.2, 4.2, 0, 100 );
  Serial.printf( "Baterka: %d %%\n", stavBaterky );

}



void setup() {
  Serial.begin(115200);

  //+++ DEBUG ONLY
  #ifdef LOGUJ_DEBUG
    delay(2000);
    Serial.println("----------------------");
    Serial.println("Startuju");
  #endif
  
  //---- DEBUG ONLY


  rtc_gpio_pullup_dis(BOOT_PIN);
  rtc_gpio_pulldown_dis(BOOT_PIN);
  pinMode(BOOT_PIN, INPUT_PULLUP);
  pinMode(ledPower, OUTPUT);
  pinMode(PWR, OUTPUT);
  pinMode(MERENI_BATERKY_ZAP, OUTPUT);
  digitalWrite( MERENI_BATERKY_ZAP, HIGH );



  adc6.attach(MERENI_BATERKY_ANALOG);
  zmerNapeti();
  if( stavBaterky < 10 ) {
    napisASpi();
  }


  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));

  //Print the wakeup reason for ESP32 and touchpad too
  print_wakeup_reason();

  if( wakeupPin0 ) {
    activePage = 0;
  }



  if( wakeupTimer ) {
    digitalWrite( ledPower, LOW);
    strip.begin(); // Initialize NeoPixel strip object (REQUIRED)
    strip.show();  // Initialize all pixels to 'off'    
    theaterChaseRainbow(30);
    digitalWrite( ledPower, HIGH );
  }

  switch( activePage ) {
    case 0: default: drawBitmaps(page0, false); break;
    case 1: drawBitmaps(page1, true); break;
    case 2: drawBitmaps(page2, true); break;
    case 3: drawBitmaps(page3, true); break;
    case 4: drawBitmaps(page4, true); break;
    case 5: drawBitmaps(page5, true); break;
  }

  activePage++;
  if( activePage>5 ) activePage=1;


  // ---- nastaveni dalsiho probuzeni

  // ESP32-S2 and ESP32-S3 only support one sleep wake up touch pad.
  touchSleepWakeUpEnable(3,Threshold);

  rtc_gpio_pullup_en(BOOT_PIN);
  rtc_gpio_pulldown_dis(BOOT_PIN);
  esp_sleep_enable_ext0_wakeup(BOOT_PIN, LOW);

  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.printf("Setup ESP32 to sleep for %d Seconds\n", TIME_TO_SLEEP);

  //+++ DEBUG ONLY
  #ifdef LOGUJ_DEBUG
    Serial.println("Going to sleep now");
    Serial.flush(); 
    delay(1000);
  #endif
  //---- DEBUG ONLY
  esp_deep_sleep_start();  
  
}


void napisASpi() {
  digitalWrite( PWR, LOW );
  display.init(115200);
  // Configure the display according to our preferences
  display.setRotation(2);
  display.setFullWindow();
  // Display the bitmap image
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setRotation(3);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor( 10, 50 );
    display.setTextSize(2);
    display.printf( "%.2fV %d%%", vBaterka, stavBaterky ); 
    display.setCursor( 10, 100 );
    display.setTextSize(2);
    display.printf( "Vybita baterka" ); 
  } while (display.nextPage());
  digitalWrite( PWR, HIGH );

  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP_LOWBATT * uS_TO_S_FACTOR);
  esp_deep_sleep_start();  
}


// z examplu pro Adafruit Neopixel
// Rainbow-enhanced theater marquee. Pass delay time (in ms) between frames.
void theaterChaseRainbow(int wait) {
  int firstPixelHue = 0;     // First pixel starts at red (hue 0)
  for(int a=0; a<30; a++) {  // Repeat 30 times...
    for(int b=0; b<3; b++) { //  'b' counts from 0 to 2...
      strip.clear();         //   Set all pixels in RAM to 0 (off)
      // 'c' counts up from 'b' to end of strip in increments of 3...
      for(int c=b; c<strip.numPixels(); c += 3) {
        // hue of pixel 'c' is offset by an amount to make one full
        // revolution of the color wheel (range 65536) along the length
        // of the strip (strip.numPixels() steps):
        int      hue   = firstPixelHue + c * 65536L / strip.numPixels();
        uint32_t color = strip.gamma32(strip.ColorHSV(hue)); // hue -> RGB
        strip.setPixelColor(c, color); // Set pixel 'c' to value 'color'
      }
      strip.show();                // Update strip with new contents
      delay(wait);                 // Pause for a moment
      firstPixelHue += 65536 / 90; // One cycle of color wheel over 90 frames
    }
  }
}


void drawBitmaps(const unsigned char *bitmap, bool displayAccu  ) {
  digitalWrite( PWR, LOW);
  display.init(115200);
  // Configure the display according to our preferences
  display.setRotation(2);
  display.setFullWindow();
  // Display the bitmap image
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.drawBitmap(0, 0, bitmap, display.epd2.WIDTH, display.epd2.HEIGHT, GxEPD_BLACK);

    if( displayAccu ) {
      display.setRotation(3);
      display.setCursor( 0, 113 );
      display.setTextSize(1);
      display.setTextColor(GxEPD_BLACK);
      display.printf( "%.2fV %d%%", vBaterka, stavBaterky ); 
    }
  } while (display.nextPage());
  digitalWrite( PWR, HIGH );
}

void loop() {
  // sem se nikdy nedostaneme
}

/*
Using library Adafruit NeoPixel at version 1.11.0 in folder: C:\Users\brouzda\Documents\Arduino\libraries\Adafruit_NeoPixel 
Using library GxEPD2 at version 1.5.2 in folder: C:\Users\brouzda\Documents\Arduino\libraries\GxEPD2 
Using library Adafruit GFX Library at version 1.11.8 in folder: C:\Users\brouzda\Documents\Arduino\libraries\Adafruit_GFX_Library 
Using library Adafruit BusIO at version 1.14.4 in folder: C:\Users\brouzda\Documents\Arduino\libraries\Adafruit_BusIO 
Using library Wire at version 2.0.0 in folder: C:\Users\brouzda\AppData\Local\Arduino15\packages\esp32\hardware\esp32\2.0.11\libraries\Wire 
Using library SPI at version 2.0.0 in folder: C:\Users\brouzda\AppData\Local\Arduino15\packages\esp32\hardware\esp32\2.0.11\libraries\SPI 
Using library ESP32AnalogRead at version 0.2.1 in folder: C:\Users\brouzda\Documents\Arduino\libraries\ESP32AnalogRead 
*/

