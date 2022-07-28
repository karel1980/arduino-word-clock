/* Components used: trinket 5V, 3 shift registers (CD4094BE), clock module (TBD)

    Arduino IDE settings:
    Register Adafruit boards manager url: https://adafruit.github.io/arduino-board-index/package_adafruit_index.json
    - board: adafruit pro trinket 5v/16MHz (USB)https://adafruit.github.io/arduino-board-index/package_adafruit_index.json
    - programmer = USBtinyISP
*/

#include <DS3231.h>
#include <Wire.h>

// shift registers
int strobe_pin = 8;
int data_pin = 6;
int clock_pin = 5;
// output enable pin is connected to 5V (always on)

// clock variables
// clock uses i2c, can be connected to a battery to remember time on power outage, but could be made optional
DS3231 my_clock;
bool century = false;
bool h12_flag;
bool pm_flag;


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

unsigned long hour[] = { TWAALF, EEN, TWEE, DRIE, VIER, VIJF, ZES, ZEVEN, ACHT, NEGEN, TIEN, ELF, TWAALF };

// button parameters
int buttonPin = 12;
#define LONG_PRESS_THRESHOLD 1000
long last_button_state_change = 0;
int last_state = HIGH;
int long_press = 0;

int mode = 0; // 0 = show time, 1 = set minute, 2 = set hour # TODO: flip 1 and 2, it's not as intuitive as I would like

unsigned long lastBtnRead = 0;
const long btnReadInterval = 20;

unsigned long lastDisplayUpdate = 0;
const long displayUpdateInterval = 100;

int set_m = 0;
int set_h = 0;
int set_blink = 0; // during 'set time, we flash the current selected 5 minute increment; this is used to control the blinking
int last_blink_toggle = 0;

int hit = 0;

void setup() {
  pinMode(strobe_pin, OUTPUT);
  pinMode(data_pin, OUTPUT);
  pinMode(clock_pin, OUTPUT);

  pinMode(buttonPin, INPUT_PULLUP);

  // Start the I2C interface
  Wire.begin();
  delay(500);
  setTime(4, 55, 0);
  showCurrentTime();
  delay(500);
}

void loop() {
  handleInputs();
  updateDisplay();
}

void handleInputs() {
  unsigned long currentTime = millis();
  if (currentTime - lastBtnRead >= btnReadInterval) {
    int btnRead = digitalRead(buttonPin);
    handleButtonState(btnRead);

    lastBtnRead = currentTime;
  }
}

void handleButtonState(int btnRead) {
  if (btnRead == LOW && last_state == HIGH) {
    handleButtonDown();
  } else if (btnRead == LOW && last_state == LOW) {
    if ((!long_press) && (millis() - last_button_state_change > LONG_PRESS_THRESHOLD)) {
      long_press = 1;
      handleLongPress();
    }
  } else if (btnRead == HIGH && last_state == LOW) {
    if (!long_press) {
      handleButtonUp();
    }
  }
  last_state = btnRead;
}

void handleButtonDown() {
  long_press = 0;
  last_button_state_change = millis();
}

void handleButtonUp() {
  last_button_state_change = millis();
  if (long_press == 0) {
    handleShortPress();
  } else {
    // long press is handled as soon as it is detected, so nothing to do here
  }
}

void handleShortPress() {
  if (mode == 1) {
    set_m += 1;
    set_m %= 60;
  }
  if (mode == 2) {
    set_h += 1;
    set_h %= 12;
  }
}


void handleLongPress() {
  if (mode == 0) {
    set_h = my_clock.getHour(h12_flag, pm_flag)%12;
    set_m = my_clock.getMinute();
  } else if (mode == 2) {
    setTime(set_h, set_m, 0);
  }
  
  mode += 1;
  mode %= 3;
}

void setTime(int h, int m, int s) {
  my_clock.setHour(h);
  my_clock.setMinute(m);
  my_clock.setSecond(s);
}

void updateDisplay() {
  long currentTime = millis();
  if (currentTime - lastDisplayUpdate >= displayUpdateInterval) {
//          switch(mode) {
//            case 0: show(EEN); break;
//            case 1: show(TWEE); break;
//            case 2: show(DRIE); break;
//          }
    if (mode == 0) showCurrentTime();
    else if (mode == 1) showSetMinute();
    else if (mode == 2) showSetHour();
    lastDisplayUpdate = currentTime;
  }
}


void showSetMinute() {
  if (millis() > last_blink_toggle + 500) {
    last_blink_toggle = millis();
    set_blink = 1 - set_blink;
  }
  show((set_blink ? TIJD : 0) | minute_indication(set_m));
}

void showSetHour() {
  if (millis() > last_blink_toggle + 500) {
    last_blink_toggle = millis();
    set_blink = 1 - set_blink;
  }
  show((set_blink ? minute_indication(set_m) : 0) | hour_indication(set_h, set_m));
}

void showCurrentTime() {
  unsigned long result = HET | IS;

  int h = my_clock.getHour(h12_flag, pm_flag) % 12;
  int m = my_clock.getMinute() % 60;
  int s = my_clock.getSecond();

  // when reading the clock, always round to the nearest 5 minute increment
  // 5:57:30 to 6:02:30 should be presented as '6 o'clock'
  // add 2.5 minutes
  m += 2;
  s += 30;
  if (s >= 30) {
    m += 1;
  }
  if (m >= 60) {
    m %= 60;
    h += 1;
  }

  result |= get_time_bits(h, m);

  show(result);
}

void displayNumber(int n) {
  while (n > 0) {
    show(HET|IS | hour[n%10]);
    delay(1000);
    n /= 10;
  }
}

unsigned long get_time_bits(int h, int m) {
  // set the resulting time (add 1 hour if it's more than 15 after the hour - this is locale dependent, and works for dutch).
  return minute_indication(m) | hour_indication(h, m);
}

unsigned long hour_indication(int h, int m) {
  m -= m % 5;
  if (m > 15) {
    return hour[(h + 1) % 12];
  }
  return hour[h % 12];
}

unsigned long minute_indication(int m) {
  m -= m % 5;
  switch (m) {
    case 0: return UUR;
    case 5: return M_VIJF | NA;
    case 10: return M_TIEN | NA;
    case 15: return KWART | NA;
    case 20: return M_TIEN | VOOR | HALF;
    case 25: return M_VIJF | VOOR | HALF;
    case 30: return HALF;
    case 35: return M_VIJF | NA | HALF;
    case 40: return M_TIEN | NA | HALF;
    case 45: return KWART | VOOR;
    case 50: return M_TIEN | VOOR;
    case 55: return M_VIJF | VOOR;
  }
  return 0; // should not happen
}

void clearLeds() {
  show(0L);
}

void show(unsigned long pattern) {
  if (hit) {
    pattern = hour[hit];
  }
  digitalWrite(strobe_pin, LOW);
  // note we're using inverse logic on the leds, so high = off
  shiftOut(data_pin, clock_pin, LSBFIRST, ~(pattern));
  shiftOut(data_pin, clock_pin, LSBFIRST, ~(pattern >> 8));
  shiftOut(data_pin, clock_pin, LSBFIRST, ~(pattern >> 16));
  digitalWrite(strobe_pin, HIGH);
}
