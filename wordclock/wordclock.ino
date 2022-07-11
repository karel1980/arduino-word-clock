/* Components used: trinket 5V, 3 shift registers (CD4094BE), clock module (TBD)
 *  
 *  Arduino IDE settings: 
 *  Register Adafruit boards manager url: https://adafruit.github.io/arduino-board-index/package_adafruit_index.json
 *  - board: adafruit pro trinket 5v/16MHz (USB)https://adafruit.github.io/arduino-board-index/package_adafruit_index.json
 *  - programmer = USBtinyISP
 */

#include <DS3231.h>
#include <Wire.h>

DS3231 my_clock;

bool century = false;
bool h12Flag;
bool pmFlag;

// shift registers
int strobePin = 8;
int dataPin = 6;
int clockPin= 5;
// output enable pin is connected to 5V (always on)

// clockModule
// clock uses i2c, can be connected to a battery to remember time on power outage, but is optional

// these are in dutch; There are 22 words, each word's address is made up of the register (0-2) and the bit offset (0 - 7)
// HET IS TIJD MVIJF MTIEN KWART NA VOOR HALF EEN TWEE DRIE VIER VIJF ZES ZEVEN ACHT NEGEN TIEN ELF TWAALF UUR
#define HET (1L<<0)
#define TWEE (1L<<1)
#define M_TIEN (1L<<2)
#define ZEVEN (1L<<3)
#define X1 (1L<<4)
#define X2 (1L<<5)
#define ZES (1L<<6)
#define ELF (1L<<7)
#define TIJD (1L<<8)
#define DRIE (1L<<9)
#define VIJF (1L<<10)
#define TWAALF (1L<<11)
#define ACHT (1L<<12)
#define HALF (1L<<13)
#define VOOR (1L<<14)
#define IS (1L<<15)
#define NA (1L<<16)
#define VIER (1L<<17)
#define NEGEN (1L<<18)
#define UUR (1L<<19)
#define KWART (1L<<20)
#define EEN (1L<<21)
#define M_VIJF (1L<<22)
#define TIEN (1L<<23)

unsigned long hour[] = { TWAALF, EEN, TWEE, DRIE, VIER, VIJF, ZES, ZEVEN, ACHT, NEGEN, TIEN, ELF };

// button parameters
int buttonPin = 12;
#define LONG_PRESS_THRESHOLD 500
int last_state = 0;
int long_press = 0;
long state_change_time = 0;

void setup() {
  pinMode(strobePin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  pinMode(clockPin, OUTPUT);

  pinMode(buttonPin, INPUT_PULLUP);

  // Start the I2C interface
  Wire.begin();
}

int mode = 0; // 0 = show time, 1 = set time
long lastRefresh = 0;
long lastHourIncrement = 0;

int set_m = 0;
int set_h = 0;
int set_blink = 0; // during 'set time, we flash the current selected 5 minute increment; this is used to control the blinking

void loop() {
  if (millis() > state_change_time + 20) { // simple debounce
    int btnRead = digitalRead(buttonPin);
    if (btnRead == LOW && last_state == HIGH) {
      // button down event, record start of press
      long_press = 0;
      state_change_time = millis();
      if (mode == 1) {
        set_m += 1;
        if (set_m >= 60) {
          set_m %= 60;
          set_h += 1;
          set_h %= 12;
        }
      }
    } else if (btnRead == HIGH && last_state == LOW) {
      // button up event    
      state_change_time = millis();
      if (mode == 0 && long_press == 1) {
        mode = 1;
        set_h = my_clock.getHour(h12Flag, pmFlag);
        set_m = my_clock.getMinute();
      }
    } else if (btnRead == LOW) {
      // handle sustained button down
      if (millis() > state_change_time + LONG_PRESS_THRESHOLD) {
        if (long_press == 0) {
          // long press started
//          long_press_start = state_change_time();
          long_press = 1;
          if (mode == 0) { // go to set-time mode
            mode = 1;
          }
        }
        
        if (mode == 1) { // in set-time mode, each 500 ms, increment the our during a long press
          if (millis() > lastHourIncrement + LONG_PRESS_THRESHOLD) {
            set_h += 1;
            set_h %= 12;
            lastHourIncrement = millis();
          }
        }
      }
    }
    last_state = btnRead;
  }

  updateDisplay();
}

void updateDisplay() {
  if (mode == 0) {
    writeLeds(TWAALF);
  } else writeLeds(EEN);
  return;
  if (mode == 1) {
    // TODO: if button is untouched for 30 seconds, set the time to the marked time + 30 seconds + go to show-time mode
    
    if (millis() > lastRefresh + 300) {
      if (set_blink == 0) {
        showSetTime();
      } else {
        clearLeds();
      }
      set_blink = 1 - set_blink;
      lastRefresh = millis();
    }
  } else {
    // update the time every second
    if (millis()  > lastRefresh + 1000) {      
      showCurrentTime();
      lastRefresh = millis();
    }
  }
}

void handle_short_press() {
  if (mode == 1) { // we are in set-time mode, increment time by 1 minute;
    set_m += 1;
    if (set_m >= 60) {
      set_h += 1;
      set_m %= 60;
    }
  } else {
    // short press in show-time mode does nothing
  }
}

void show(unsigned long pattern) {
  writeLeds(pattern);
}

void showSetTime() {
  writeLeds(get_time_bits(set_h, set_m));
}

void showCurrentTime() {
  unsigned long result = HET | IS;

  int h = my_clock.getHour(h12Flag, pmFlag);
  int m = my_clock.getMinute();
  int s = my_clock.getSecond();

  // when reading the clock, always round to the nearest 5 minute increment
  // 5:57:30 to 6:02:30 should be presented as '6 o'clock'
  // add 2.5 minutes
  m += 2;
  s += 30;
  if (s >= 30) { m += 1; }
  if (m >= 60) { m %= 60; h += 1; }
  
  result |= get_time_bits(h, m);

  show(result);
}

unsigned long get_time_bits(int h, int m) {
    // set the resulting time (add 1 hour if it's more than 15 after the hour - this is locale dependent, and works for dutch).
  if (m >= 55) return M_VIJF | VOOR | hour[(h+1)%12];
  if (m >= 50) return M_TIEN | VOOR | hour[(h+1)%12];
  if (m >= 45) return KWART | VOOR | hour[(h+1)%12];
  if (m >= 40) return M_TIEN | NA | HALF | hour[(h+1)%12];
  if (m >= 35) return M_VIJF| NA | HALF | hour[(h+1)%12];
  if (m >= 30) return HALF | hour[(h+1)%12];
  if (m >= 25) return M_VIJF | VOOR | HALF | hour[(h+1)%12];
  if (m >= 20) return M_TIEN | VOOR | HALF | hour[(h+1)%12];
  if (m >= 15) return KWART | NA | hour[h%12];
  if (m >= 10) return M_TIEN | NA| hour[h%12];
  if (m >= 5) return M_VIJF | NA | hour[h%12];
  return hour[h%12] | UUR;
}

void clearLeds() {
  writeLeds(0L);
}

void writeLeds(unsigned long pattern) {
  digitalWrite(strobePin, LOW);
  // note we're using inverse logic on the leds, so high = off
  shiftOut(dataPin, clockPin, LSBFIRST, ~(pattern));
  shiftOut(dataPin, clockPin, LSBFIRST, ~(pattern>>8));
  shiftOut(dataPin, clockPin, LSBFIRST, ~(pattern>>16));
  digitalWrite(strobePin, HIGH);
}
