#include "config.h"
#include <Arduino.h>

#include "TFT_eSPI.h"

#include <SPI.h>

#include <WiFi.h>

#include <FS.h>
#include <SPIFFS.h>
#include <esp32fota.h>

#include <Button2.h>

#include "MrClock.h"

#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager

#define TFT_GREEN1  0x07E6
#define TFT_ORANGE1 0xFBE0
#define TFT_GREY1   0x39C4

TFT_eSPI tft = TFT_eSPI();//Invoke custom library

Button2 btn1(BUTTON_1);
Button2 btn2(BUTTON_2);

Button2 btn;
WiFiManager wm;

char buff[512];
long retry=0;

//MrClock variables
long MrClock_TimeOut; //speed of MrClock for offline calculation - display
long mrClk_TimeOut;  //delay of incoming packet from MrClock server
int mHH;
int mMM;
int mSS;
int MrSpeed=1000; //speed of MrClock in ms
int MrClock_status=1; //start with stopped clock

int MrSpeedPrev = 1000; //previous setting of speed

int version = 240401;

//automated upload software
esp32FOTA esp32FOTA("MrClock_v1", version, false, true);
const char* manifest_url = "https://raw.githubusercontent.com/freezeghost/T-Display_MrClock/main/FW/mrclockv1.json"; //correction of path to github repo for direct access


//! Long time delay, it is recommended to use shallow sleep, which can effectively reduce the current consumption
void espDelay(int ms)
{
    esp_sleep_enable_timer_wakeup(ms * 1000);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH,ESP_PD_OPTION_ON);
    esp_light_sleep_start();
}

//Controll buttons
void btn_handler(Button2& btn) {
    switch (btn.getType()) {
        case single_click:
            /*
            Future use
            if(btn==btn1){}
            if(btn==btn2){}
            */
            break;
        case double_click:
            /*
            Future use
            if(btn==btn1){}
            if(btn==btn2){}
            */
            break;
        case triple_click:
            /*
            Future use
            if(btn==btn1){}
            if(btn==btn2){}
            */
            break;
        case long_click:
            if(btn==btn2){ //start WiFiManager config portal with timeout 60sec, when no one is connected, it will automatically exit
                tft.fillScreen(TFT_BLACK);
                tft.setTextColor(TFT_WHITE, TFT_BLACK);
                tft.setTextFont(4);
                tft.setTextDatum(MC_DATUM);
                tft.drawString("Connect to config AP", tft.width() / 2, (tft.height()/2)-51);
                tft.setTextColor(TFT_GREEN1, TFT_BLACK);
                tft.drawString("\"MrClock_v1\"", tft.width() / 2, tft.height()/2-16);
                tft.drawString("pwd:\"mrclockv1\"", tft.width() / 2, tft.height()/2+16);
                tft.setTextColor(TFT_YELLOW);
                tft.drawString("http://192.168.4.1", tft.width()/2, (tft.height()/2)+51);
                wm.setConfigPortalTimeout(60); // auto close configportal after n seconds
                wm.setAPClientCheck(true); // avoid timeout if client connected to softap
                wm.startConfigPortal("MrClock_v1","mrclockv1");
                delay(1000);
                tft.fillScreen(TFT_BLACK);
            }
            break;
        default:
            DBG(Serial.print("Default case ");)
            break;
    }
}
void button_loop()
{
    btn1.loop();
    btn2.loop();
}

//display strenght of WiFi
void rssiWiFi(uint16_t x0, uint16_t y0, uint16_t Color, uint16_t Size){
    if(Size<=0){
        Size=1;
    }
    int rssi = WiFi.RSSI(); //get strenght RSSI
    uint32_t wCross = TFT_BLACK;
    uint32_t wColor5 = 0x10A1;
    uint32_t wColor25 = 0x10A1;
    uint32_t wColor50 = 0x10A1;
    uint32_t wColor75 = 0x10A1;
    uint32_t wColor100 = 0x10A1;
    if (WiFi.status() == WL_CONNECTED){
        //lowest strenght
        wColor5 = Color;
        //25%
        if (rssi >=-74){wColor25 = Color;}
        //50%
        if (rssi >=-66) {wColor50 = Color;}
        //75%
        if (rssi >=-58){wColor75 = Color;}
        //100%
        if (rssi >=-50){wColor100 = Color;}
    } else {
        //no connection
        wCross = TFT_RED;
    }

    //draw picture
    tft.drawLine(x0, y0, x0+(5*Size), y0+(5*Size), wCross);
    tft.drawLine(x0, y0+(5*Size), x0+(5*Size), y0, wCross);
    tft.fillRect(x0, y0+(8*Size), 2*Size, 2*Size, wColor5); //very poor signal
    tft.fillRect(x0+(3*Size), y0+(6*Size), 2*Size, 4*Size, wColor25); //25%
    tft.fillRect(x0+(6*Size), y0+(4*Size), 2*Size, 6*Size, wColor50); //50%
    tft.fillRect(x0+(9*Size), y0+(2*Size), 2*Size, 8*Size, wColor75); //75%
    tft.fillRect(x0+(12*Size), y0, 2*Size, 10*Size, wColor100); //100%
}

//***************************************************************************************************************************************************
//MRclock display time
//xpos, ypos, hours, minutes, status 1 - STOPPED, 2 - RUNNING, 3 - RUNNING no connection with MrClock, 0 - STOPPED no connection with MrClock
void MRclock(int xpos, int ypos, int hh, int mm, int stat){
    tft.setTextDatum(TL_DATUM);
    //display clock status
    uint16_t color = TFT_GREY1; //default is grey
    switch (stat) {
        case 0: //base
        color = TFT_GREY1; //light grey
        break;
        case 1: //clock is stopped
        color = TFT_RED;
        break;
        case 2: //clock is running
        color = TFT_GREEN1;
        break;
        case 3: //running no connection with MRclock
        color = TFT_ORANGE1;
        break;
        default:
        color = TFT_GREY1; //default color grey
        break;
    }
     tft.setTextColor(color, TFT_BLACK);
    //Correction for hours include leading zero
    if (hh < 10) xpos += tft.drawChar('0', xpos, ypos, 8);
    xpos += tft.drawNumber(hh, xpos, ypos, 8);
    xpos += tft.drawChar(':', xpos , ypos, 8);
    //correction for minutes
    tft.setTextColor(color, TFT_BLACK);
    if (mm < 10) xpos += tft.drawChar('0', xpos, ypos, 8);
    tft.drawNumber(mm, xpos, ypos, 8);
}
//***************************************************************************************************************************************************

//! ***********************************************************************************************
//!   SETUP
//! ***********************************************************************************************
void setup()
{
    Serial.begin(115200);
    Serial.println("Start MrClock v1 display");
    delay(500);
    tft.init();
    tft.setRotation(1);
    tft.setSwapBytes(true);
    tft.fillScreen(TFT_BLACK);

    if (TFT_BL > 0) { // TFT_BL has been set in the TFT_eSPI library in the User Setup file TTGO_T_Display.h
         pinMode(TFT_BL, OUTPUT); // Set backlight pin to output mode
         digitalWrite(TFT_BL, TFT_BACKLIGHT_ON); // Turn backlight on. TFT_BACKLIGHT_ON has been set in the TFT_eSPI library in the User Setup file TTGO_T_Display.h
    }

    //ini screen
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextFont(4);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("START", tft.width()/2, (tft.height()/2)-16);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString("MrClock display", tft.width()/2, (tft.height()/2)+16);

    //interrupt for buttn 1
    btn1.setClickHandler(btn_handler);
    btn1.setLongClickTime(1000);
    btn1.setDoubleClickHandler(btn_handler);
    btn1.setLongClickHandler(btn_handler);

    //interrupt for buttn 2
    btn2.setClickHandler(btn_handler);
    btn2.setLongClickTime(1000);
    btn2.setDoubleClickHandler(btn_handler);
    btn2.setLongClickHandler(btn_handler);

    wm.setConnectTimeout(20); // how long to try to connect for before continuing
    wm.setConfigPortalTimeout(5); // unfortunately I had to add this row. I can't get saved credentials from WiFiManager
    //if(wm.getWiFiIsSaved()==true){} not working:-(
    bool res = wm.autoConnect(); //connect to WiFi via WiFiManager

    DBG(
        if(!res) {
            Serial.println("Failed to connect");
        }
        else {
            Serial.println("connected...");
        }
    )

    //remote upload firmware
    esp32FOTA.setManifestURL( manifest_url );
    esp32FOTA.printConfig();
    tft.fillScreen(TFT_BLACK); //blank display content
}

//! ***********************************************************************************************
//! MAIN LOOP
//! ***********************************************************************************************
void loop()
{
    tft.setTextSize(1);
    mPacket();
    MRclock(-4,0,mHH,mMM,MrClock_status);
    //draw bargraph with running seconds
    tft.drawSmoothRoundRect(0,85,3,3,239,15,TFT_BLUE,TFT_BLACK);
    int bar = mSS * 3.97;
    tft.fillSmoothRoundRect(1,86,bar,13,3,TFT_YELLOW,TFT_BLACK);
    if(mSS==0){tft.fillSmoothRoundRect(1,86,238,13,3,TFT_BLACK,TFT_BLACK);}
    tft.setTextFont(4);
    tft.setTextSize(1);
    if(MrSpeedPrev!=MrSpeed){
        MrSpeedPrev=MrSpeed; //store current speed
        tft.fillRect(200,100,40,35,TFT_BLACK); //delete previous value
    }
    tft.setTextColor(TFT_BLUE);
    if (MrSpeed!=0){
        tft.drawNumber(1000/MrSpeed,200,108); //MrSpeed is in miliseconds!
    }
    //Show RSSI
    rssiWiFi(5, 103, TFT_BLUE,3);

    button_loop();

    //reconnect WiFi
    if (wm.getWiFiIsSaved()==true && WiFi.status() !=3 && retry <= millis()){
        retry = millis() + 2500; //try again within 2.5 sec
        WiFi.reconnect(); //try to connect
        DBG(Serial.println("WiFi try to reconnect");)
    }

    //Automated upload firmware
    bool updatedNeeded = esp32FOTA.execHTTPcheck();
    if (updatedNeeded)  {
        DBG(Serial.println("New version of firmware!!!");)
        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.setTextFont(4);
        tft.setTextDatum(MC_DATUM);
        tft.drawString("! UPGRADE !", tft.width()/2, (tft.height()/2)-16);
        tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        tft.drawString("! FIRMWARE !", tft.width()/2, (tft.height()/2)+16);
        esp32FOTA.execOTA();
        tft.fillScreen(TFT_BLACK);
    }
}
