/*

CAPSLOCK FRIDAY
Cruise Control for Cool
©2021 Gavin McDonald <me@gavitron.com>

Basically a one-key keyboard that presses CAPS LOCK.  has a status LED and an OLED screen for feedback.

note: on OSX, keyboard states like CAPSLOCK are managed independently, per device, so this keyboard would
only capitalize the letters it sends, but it sends none. In order to fix this, you can install Karabiner:
    https://karabiner-elements.pqrs.org/

more information available at https://github.com/NicoHood/HID/issues/142
or else try the solution here: https://github.com/NicoHood/HID/issues/97

*/

#include <Arduino.h>
#include <ss_oled.h>
#include <HID-Project.h>
#include <arduino-timer.h>


/// Setup for the OLED library
//--------------

// Change these if you're using a different size OLED display
#define MY_OLED OLED_128x32
#define OLED_WIDTH 128
#define OLED_HEIGHT 32

// use a backbuffer for faster redraw
#define USE_BACKBUFFER
static uint8_t ucBackBuffer[512];  // 512 == 128x32 pixels

// Bit-Bang the I2C bus
#define USE_HW_I2C 1

// let ss_oled figure out the display address
#define OLED_ADDR 0x3C

// Use -1 for the Wire library default pins
// or specify the pin numbers to use with the Wire library or bit banging on any GPIO pins
// These are the pin numbers for the M5Stack Atom default I2C
//#define SDA_PIN 2
//#define SCL_PIN 3

// Set this to -1 to disable or the GPIO pin number connected to the reset
// line of your display if it requires an external reset
#define RESET_PIN -1

// don't rotate the display
#define FLIP180 0

// don't invert the display
#define INVERT 0

// this is for displays that don't wrap memory writes to the next line
//   #define BAD_DISPLAY

// init the display
SSOLED ssoled;

// load the animation frames
#include "rawbytes.cpp"

// ----------

// declare the pin assignements
const int pinSDA = 2;
const int pinSCL = 3;
const int pinLed = 5;
const int pinButton = 6;

// ----------

// create a timer with 1 task and microsecond resolution
Timer<1, millis> timer;

/// =============================

// animation routine, meant to be called once per frame.
bool capsAnim(bool restart=false) {
  static int frameCounter=0;  // static for obvious reasons

  // reset the frame counter
  if (restart) {
    frameCounter = 0;
  }

  // animation via case statement
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
    // when we run out of frames, reset the counter and return false to stop the timer callback
    case 23:
    default:
      frameCounter = 0;
      return false;
  }
  return true;
}

/// =============================

void setup() {
  // configure the IO pins
  pinMode(pinLed, OUTPUT);
  pinMode(pinButton, INPUT_PULLUP);

 // setup the OLED screen and print some info if found
  int rc;
  rc = oledInit(&ssoled, MY_OLED, OLED_ADDR, FLIP180, INVERT, USE_HW_I2C, pinSDA, pinSCL, RESET_PIN, 400000L); // use standard I2C bus at 400Khz
  if (rc != OLED_NOT_FOUND) {
    char *msgs[] = {(char *)"SSD1306 @ 0x3C", (char *)"SSD1306 @ 0x3D",(char *)"SH1106 @ 0x3C",(char *)"SH1106 @ 0x3D"};
    oledFill(&ssoled, 0x0, 1);
    oledWriteString(&ssoled, 0,0,0,(char *)"CAPSLOCK", FONT_STRETCHED, 0, 1);
    oledWriteString(&ssoled, 0,0,2,(char *)" me@gavitron.com", FONT_NORMAL, 0, 1);
    oledWriteString(&ssoled, 0,0,3,(char *)"                v1.03", FONT_SMALL, 0, 1);
    oledWriteString(&ssoled, 0,0,3,msgs[rc], FONT_SMALL, 0, 1);
    oledSetBackBuffer(&ssoled, ucBackBuffer);
    delay(3000);
    oledFill(&ssoled, 0, 1);
  }

  // Sends a clean USB report to the host. This is important on any Arduino type.
  BootKeyboard.begin();
}

/// =============================

void loop() {
  // state trackers so we're only acting on state changes
  static uint8_t prev_led_state = -1;
  static uint8_t led_state = -1;

  // when the button is pressed, send a caps-lock key event to the host
  if (!digitalRead(pinButton)) {
    BootKeyboard.write(KEY_CAPS_LOCK);
    delay(250);  // lazy debouncer
  }

  // get the current state of CAPSLOCK
  prev_led_state = led_state;
  led_state = BootKeyboard.getLeds();

  // did it change since we last looked?
  if (led_state != prev_led_state) {
    // test to see what state CAPS is in:
    if (led_state & LED_CAPS_LOCK) {
      // set CAPS LED
      digitalWrite(pinLed, HIGH);
      // reset the caps animation
      capsAnim(true);
      // and then start animating, one frame every 100 ms
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

