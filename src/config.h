const char* version = "0.3.4"; //actual version of firmware

#ifndef TFT_DISPOFF
  #define TFT_DISPOFF 0x28
#endif

#ifndef TFT_SLPIN
  #define TFT_SLPIN   0x10
#endif

#define ADC_EN          14
#define ADC_PIN         34

#ifdef TD
  #define BUTTON_2      35
  #define BUTTON_1      0
#endif

#define DEBUG_SER //debug over serial connection

//start debug over serial port
#ifdef DEBUG_SER
  #define DBG(x) x
#else
  #define DBG(x)
#endif