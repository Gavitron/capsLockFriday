#include <Arduino.h>
#include <ss_oled.h>
#include <HID-Project.h>
#include <arduino-timer.h>

//  need to install Karabiner on osx to solve, per https://github.com/NicoHood/HID/issues/142  
//   https://karabiner-elements.pqrs.org/


// else try: https://github.com/NicoHood/HID/issues/97



/// Setup for the OLED library
//--------------
//  this is for displays that don't wrap on their own.
//   #define BAD_DISPLAY

#define USE_BACKBUFFER
static uint8_t ucBackBuffer[512];

// Use -1 for the Wire library default pins
// or specify the pin numbers to use with the Wire library or bit banging on any GPIO pins
// These are the pin numbers for the M5Stack Atom default I2C
//#define SDA_PIN 2
//#define SCL_PIN 3
// Set this to -1 to disable or the GPIO pin number connected to the reset
// line of your display if it requires an external reset
#define RESET_PIN -1
// let ss_oled figure out the display address
#define OLED_ADDR 0x3C
// don't rotate the display
#define FLIP180 0
// don't invert the display
#define INVERT 0
// Bit-Bang the I2C bus
#define USE_HW_I2C 1

// Change these if you're using a different OLED display
#define MY_OLED OLED_128x32
#define OLED_WIDTH 128
#define OLED_HEIGHT 32

SSOLED ssoled;

/*
  Generate this file with the following command:
  ./tcomp --top 0 --left 0 --in img/capslock_anim.gif --out img/capslock_anim.ino --c
  #include "../img/capslock_anim.ino"
*/
#include "../img/rawbytes.ino"

// ----------

const int pinSDA = 2;
const int pinSCL = 3;
const int pinLed = 5;
const int pinButton = 6;

// ----------

Timer<1, millis> timer; // create a timer with 1 task and microsecond resolution

/// =============================

bool capsAnim(bool restart=false) {
  static int frameCounter=0;

  if (restart) {
    frameCounter = 0;
  }

  switch (frameCounter++) {
    case 1:
    case 3:
    case 5:
    case 7:
    case 9:
    case 11:
    case 13:
    case 15:
    case 17:
    case 19:
    case 21:
      oledFill(&ssoled, 0x00, 1);
      break;
    case 0:
    case 2:
    case 4:
    case 6:
      oledLoadBYTES(&ssoled, (uint8_t *)bImgCaps, 1, 1);
      break;
    case 8:
    case 10:
    case 12:
    case 14:
      oledLoadBYTES(&ssoled, (uint8_t *)bImgLock, 1, 1);
      break;
    case 16:
    case 18:
    case 20:
    case 22:
      oledLoadBYTES(&ssoled, (uint8_t *)bImgCpLo, 1, 1);
      break;
    case 23:
    default:
    frameCounter = 0;
    return false;
  }
  return true;
}

/// =============================

void setup() {
  pinMode(pinLed, OUTPUT);
  pinMode(pinButton, INPUT_PULLUP);

  // Sends a clean report to the host. This is important on any Arduino type.
  BootKeyboard.begin();

  int rc;
  rc = oledInit(&ssoled, MY_OLED, OLED_ADDR, FLIP180, INVERT, USE_HW_I2C, pinSDA, pinSCL, RESET_PIN, 400000L); // use standard I2C bus at 400Khz
  if (rc != OLED_NOT_FOUND) {
    char *msgs[] = {(char *)"SSD1306 @ 0x3C", (char *)"SSD1306 @ 0x3D",(char *)"SH1106 @ 0x3C",(char *)"SH1106 @ 0x3D"};
    oledFill(&ssoled, 0, 1);
    oledWriteString(&ssoled, 0,0,0,msgs[rc], FONT_NORMAL, 0, 1);
    oledSetBackBuffer(&ssoled, ucBackBuffer);
    delay(50);

    oledFill(&ssoled, 0x0, 1);
    oledWriteString(&ssoled, 0,0,0,(char *)"CAPSLOCK", FONT_STRETCHED, 0, 1);
    oledWriteString(&ssoled, 0,0,2,(char *)"2021 by Gavitron", FONT_NORMAL, 0, 1);
    oledWriteString(&ssoled, 0,0,3,(char *)"    <me@gavitron.com>  ", FONT_SMALL, 0, 1);
    delay(2000);
  }

  oledFill(&ssoled, 0, 1);
}

/// =============================

void loop() {
  //static uint8_t *pAnim = (uint8_t *)bAnimation; // point to first frame
  static uint8_t prev_led_state = -1;
  static uint8_t led_state = -1;

  // Trigger caps lock manually via button
  if (!digitalRead(pinButton)) {
    BootKeyboard.write(KEY_CAPS_LOCK);
    delay(250);  // lazy debouncer
  }

  // Update Led equal to the caps lock state.
  // Keep in mind that on a 16u2 and Arduino Micro HIGH and LOW for TX/RX Leds are inverted.
  prev_led_state = led_state;
  led_state = BootKeyboard.getLeds();

  if (led_state != prev_led_state) {
    if (led_state & LED_CAPS_LOCK) {
      // set CAPS LED
      digitalWrite(pinLed, HIGH);
      // do a little CAPSLOCK dance, bumping a frame every 100 milliseconds
      capsAnim(true);
      timer.every(100, capsAnim);
    } else {
      // unset CAPS LED
      digitalWrite(pinLed, LOW);
      // stop any animation
      timer.cancel();
      // blank the screen
      oledFill(&ssoled, 0x0, 1);
    }  // end evaluate CAPS state
  }  // end LED changed

  timer.tick(); // tick the timer
}

























