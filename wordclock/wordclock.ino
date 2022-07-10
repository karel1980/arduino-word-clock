/* Components used: trinket 5V, 3 shift registers (CD4094BE), clock module (TBD)
 *  
 *  Arduino IDE settings: 
 *  Register Adafruit boards manager url: https://adafruit.github.io/arduino-board-index/package_adafruit_index.json
 *  - board: adafruit pro trinket 5v/16MHz (USB)https://adafruit.github.io/arduino-board-index/package_adafruit_index.json
 *  - programmer = USBtinyISP
 */

// shift registers
int strobePin = 8;
int dataPin = 6;
int clockPin= 5;
// output enable pin is connected to 5V (always on)

// button
int buttonPin = 13;

// clockModule
// clock uses i2c, can be connected to a battery to remember time on power outage, but is optional

// global variables for storing led states. We're using inverse logic (1 = off, 0 is on)

void setup() {
  pinMode(strobePin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
}

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


void loop() {
  for (byte j = 0; j < 24; j++) {
    show(HET | IS | M_VIJF | NA| TIEN);
    delay(3000);
  }
}

void show(unsigned long pattern) {
  writeLeds(pattern);
}

void writeLeds(unsigned long pattern) {
  digitalWrite(strobePin, LOW);
  shiftOut(dataPin, clockPin, LSBFIRST, (pattern));
  shiftOut(dataPin, clockPin, LSBFIRST, (pattern>>8));
  shiftOut(dataPin, clockPin, LSBFIRST, (pattern>>16));
  digitalWrite(strobePin, HIGH);
}
