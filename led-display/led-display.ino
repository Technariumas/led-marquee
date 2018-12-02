#include "Adafruit_GFX.h"
#include <Fonts/FreeSans9pt7b.h>

#define OE 10
#define DATA1 6
#define DATA2 7
#define ST 8
#define CK 9
#define A 2
#define B 3
#define C 4
#define D 5

uint8_t screen[24][20];
uint8_t currLine = 0;

class Lenta : public Adafruit_GFX {
   public:

  Lenta(int16_t w, int16_t h) : Adafruit_GFX(w, h){ // Constructor
  }
  
  void drawPixel(int16_t x, int16_t y, uint16_t color) {
    if(x < 0 || x > 159 || y < 0 || y > 23) {
      return;
    }
    
    uint8_t i = x >> 3;
    uint8_t b = 128 >> (x & 0b00000111);
    
    if(color) {
      screen[y][i] &= ~b;
    } else {
      screen[y][i] |= b;
    }
  }
};

Lenta lenta(160 + 16, 24);

uint8_t t = 0;

void clearRegisters() {
  digitalWrite(ST, LOW);
  digitalWrite(DATA1, HIGH);    
  digitalWrite(DATA2, HIGH);    
  for(int i = 0; i < 160; i++){
    digitalWrite(CK, HIGH);    
    digitalWrite(CK, LOW);    
  }
  digitalWrite(ST, HIGH);
}

void lineSelect(uint8_t l) {
  digitalWrite(A, (l & 0b0001) != 0);
  digitalWrite(B, (l & 0b0010) != 0);
  digitalWrite(C, (l & 0b0100) != 0);
  digitalWrite(D, (l & 0b1000) != 0);
}

void setup() {
  Serial.begin(9600);

  pinMode(A, OUTPUT);
  pinMode(B, OUTPUT);
  pinMode(C, OUTPUT);
  pinMode(D, OUTPUT);
  pinMode(OE, OUTPUT);
  pinMode(ST, OUTPUT);
  pinMode(CK, OUTPUT);
  pinMode(DATA1, OUTPUT);
  pinMode(DATA2, OUTPUT);

  uint8_t t = 0;
  for(int i = 0; i < 24; i++){
    for(int j = 0; j < 20; j++) {
      screen[i][j] = 255;
    }
  }

  clearRegisters();
//  lenta.fillCircle(10, 10, 5, 1);
//  lenta.setFont(&FreeSans9pt7b);
  lenta.setTextWrap(false);

  lenta.setTextSize(2);
//  lenta.setCursor(0, 16);
//  lenta.print("Internet of Shit");
}

void testOut() {
  digitalWrite(ST, LOW);
  shiftOut(DATA2, CK, MSBFIRST, 0x55);
  shiftOut(DATA1, CK, MSBFIRST, 0xAA);
  digitalWrite(ST, HIGH);
}

void shiftOutLine(uint8_t line1[], uint8_t line2[]) {
  for(int8_t i = 19; i >= 0; i--){
    uint8_t l1, l2;
    l1 = line1[i];
    l2 = line2[i];
    for(uint8_t b = 0; b < 8; b++) {
      if(l1 & 0x01) {
        PORTD |= _BV(PD7);        
      } else {
        PORTD &= ~_BV(PD7);
      }
      if((l2 & 0x01)) {
        PORTD |= _BV(PD6);
      } else {
        PORTD &= ~_BV(PD6);        
      }

      PORTB|= _BV(PB1);
      PORTB &= ~_BV(PB1);
      l1 = l1 >> 1;
      l2 = l2 >> 1;
    }
  }
}

bool isScreenDone = false;

void scanScreen() {
  digitalWrite(ST, LOW);
  uint8_t dataPin;

  if(currLine < 8) {
    lineSelect(7 - currLine);  
    shiftOutLine(screen[currLine], screen[currLine + 16]);
  } else if(currLine > 7 && currLine < 16){
    lineSelect(23 - currLine);  
    shiftOutLine(screen[currLine], screen[currLine]);
  }

  if(++currLine > 15) {
    currLine = 0;
    isScreenDone = true;
  } else {
    isScreenDone = false;
  }
  
  digitalWrite(ST, HIGH);
}

char textBuf[128];
uint8_t tindex = 0;
char buf[128];

int i;
uint32_t ts = millis();
int tx = 0, ty = 4;

int scrollBoundary = -355;



void loop() {
  scanScreen();
  digitalWrite(OE, LOW);
  delayMicroseconds(200);    
  digitalWrite(OE, HIGH);

  if(Serial.available()) {
    if(tindex < 128) {
      char c = Serial.read();
      if(c == '\n') {
        buf[tindex] = 0;
        tindex = 0;

        strcpy(textBuf, buf);
        tx = 159;
        int x1, y1, w, h;
        lenta.getTextBounds(textBuf, tx, ty, &x1, &y1, &w, &h);
        scrollBoundary = -w;
      } else {
        buf[tindex] = c;
        tindex++;
      }
    }
  }

  if(isScreenDone && (millis() - ts > 50)) {
    ts = millis();

    memset(screen, 255, 24*20);
    tx = tx - 1;
    if(tx < scrollBoundary) {
      tx= 160;
    }
    lenta.setCursor(tx, ty);
    lenta.print(textBuf);
  }

}
