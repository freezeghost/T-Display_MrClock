const char* version = "0.2.0"; //actual version of firmware


#define T_DISPLAY //switch between T-Display and T-Display-S3

#ifdef T_DISPLAY
  #define TD
#else
  #define TDS
#endif

#ifndef TFT_DISPOFF
  #define TFT_DISPOFF 0x28
#endif

#ifndef TFT_SLPIN
  #define TFT_SLPIN   0x10
#endif

#define ADC_EN          14
#define ADC_PIN         34

#define BUTTON_1        35
#define BUTTON_2        0

//#define DEBUG_SER //debug over serial connection

//start debug over serial port
#ifdef DEBUG_SER
  #define DBG(x) x
#else
  #define DBG(x)
#endif