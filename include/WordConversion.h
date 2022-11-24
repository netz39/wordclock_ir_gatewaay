#ifndef TIME2WORD_CONVERSION_H
#define TIME2WORD_CONVERSION_H

#define MAX_LED_PRO_WORD 2

typedef struct {
  uint8_t led_position:5; //max. 23 positions
  uint8_t numberOfLeds:2; //max 2 Leds for one Word  
} clockWord;

clockWord hours[] ={
  {8,1},  // 12
  {0,1},  // 01
  {11,1}, // 02
  {1,1},  // 03
  {10,1}, // 04
  {12,1}, // 05
  {2,1},  // 06
  {3,2},  // 07
  {9,1},  // 08
  {13,1}, // 09
  {5,1},  // 10
  {6,1}   // 11
};

clockWord rest[] ={
  {20,1}, // FÜNF
  {18,1}, // ZEHN
  {19,1}, // NACH
  {16,2}, // VIERTEL
  {14,1}, // VOR
  {15,1}, // HALB
  {7,1},  // UHR
  {22,1}, // ES
  {21,1}, // IST

};
/*
00 -> EINS
01 -> DREI
02 -> SECHS
03 -> SIEBEN_1
04 -> SIEBEN_2
05 -> ZEHN
06 -> ELF
07 -> UHR
08 -> ZWÖLF
09 -> ACHT
10 -> VIER
11 -> ZWEI
12 -> FÜNF
13 -> NEUN
14 -> VOR
15 -> HALB
16 -> VIERTEL_1
17 -> VIERTEL_2
18 -> ZEHN
19 -> NACH
20 -> FÜNF
21 -> IST
22 -> ES
*/
/*
 * Bit 1 -> FÜNF
 * Bit 2 -> ZEHN
 * Bit 3 -> NACH
 * Bit 4 -> VIERTEL
 * Bit 5 -> VOR
 * Bit 6 -> HALB
 * Bit 7 -> UHR
 * Bit 8 -> 1+ offset for houre
 * 
 */
uint8_t minutes[] ={ 
  0b01000000, //00 - 0
  0b00000101, //05 - 5 nach
  0b00000110, //10 - 10 nach
  0b10001000, //15 - viertel
  0b10110010, //20 - 10 vor halb
  0b10110001, //25 - 5 vor halb
  0b10100000, //30 - halb
  0b10100101, //35 - 5 nach halb
  0b10100110, //40 - 10 nach halb
  0b10011000, //45 - viertel vor
  0b10010010, //50 - 10 vor
  0b10010001, //55 - 5 vor
};

#endif
