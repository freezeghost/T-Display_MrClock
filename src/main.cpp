/*
For change between T-DISPLAY and T-DISPLAY-S3
Use platformio.ini

for open of menu press top button
*/
#include "config.h"
#include <Arduino.h>
#include <EEPROM.h>

#include "TFT_eSPI.h"

#include <WiFi.h>

#include <FS.h>
#include <SPIFFS.h>
#include <esp32fota.h>

#include <Button2.h>

#include "MrClock.h"

#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager

TFT_eSPI tft = TFT_eSPI();//Invoke custom library

Button2 btnUp, btnDwn;  //change control of buttons for solving "Long press button react after release #3"

WiFiManager wm;

char hostname[23];
byte modeWiFi = 0; //WiFi mode 0-client; 1-server; 2-OFF stored in EEPROM 0
long retry=0; //retry time for testing reconnect to WiFi
bool update = false; //marker for available update
bool WMActive = false;
long wmTimeout = 0;
//TODO
byte lang = 0;  //Language 0-English; 1-Czech; 2-German stored in EEPROM 1

//MrClock variables
long MrClock_TimeOut; //speed of MrClock for offline calculation - display
long mrClk_TimeOut;  //delay of incoming packet from MrClock server
int mHH;    //Stored in EEPROM 4
int mMM;    //Stored in EEPROM 5
int mSS;
int MrSpeed=1000; //speed of MrClock in ms
int MrSpeedPrev = 1000; //previous setting of speed
byte MrClock_status=1; //start with stopped clock
byte clockMode = 0;     //clock mode 0-client; 1-server stored in EEPROM 2
byte mrSetSpeed = 1;    //clock speed stored in server mode in EEPROM 3

//automated upload software
#ifdef TD
    esp32FOTA esp32FOTA("MrClock_v1", version, false, true);
    const char* manifest_url = "https://raw.githubusercontent.com/freezeghost/T-Display_MrClock/main/FW/mrclockv1.json"; //correction of path to github repo for direct access
#else
    esp32FOTA esp32FOTA("MrClock3_v1", version, false, true);
    const char* manifest_url = "https://raw.githubusercontent.com/freezeghost/T-Display_MrClock/main/FW/mrclock3v1.json"; //correction of path to github repo for direct access
#endif
//! Long time delay, it is recommended to use shallow sleep, which can effectively reduce the current consumption
void espDelay(int ms)
{
    esp_sleep_enable_timer_wakeup(ms * 1000);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH,ESP_PD_OPTION_ON);
    esp_light_sleep_start();
}

#include <uiMenu.h>

//Control buttons
//short click
void btnClick(Button2& btn){
    if(btn == btnUp){nav.doNav(downCmd);}   // It's called downCmd because it decreases the index of an array. Visually that would mean the selector goes upwards.
    if(btn == btnDwn){
        if (nav.sleepTask){
            if(clockMode == 1){ //clock server mode
                if(MrClock_status<=1){
                    MrClock_status=2; //start clock
                }else{
                    MrClock_status=1; //stop clock
                }
            }
        }else{ //when menu is up
            nav.doNav(upCmd); // It's called upCmd because it increases the index of an array. Visually that would mean the selector goes downwards.
        }
    }
    if(wm.getConfigPortalActive()==true){ //Solving issue "Can't exit when is AP setting running #4"
        DBG(Serial.println("Manually exit ConfigAP");)
        wm.stopConfigPortal();
    }
}
//long hold
void btnLongClick(Button2& btn){
    if(btn == btnUp){
        nav.doNav(enterCmd);
        if(nav.sleepTask){mrSetSpeed=1000/MrSpeed;}
    }
    if(btn == btnDwn){nav.doNav(escCmd);}
}

void button_loop()
{
    btnUp.loop();
    btnDwn.loop();
}

//WiFi control
void WiFimode(byte mode){
    switch (mode){
        case 0: //client mode
            if(WiFi.getMode() != WIFI_MODE_STA){
                WiFi.mode(WIFI_MODE_STA);
                WiFi.setHostname(hostname); //define hostname
                WiFi.begin();
            }else{
                //reconnect WiFi
                if (wm.getWiFiIsSaved()==true && WiFi.status()!=WL_CONNECTED && retry<=millis()){
                    retry = millis() + 2484; //try again within 2.5 sec ;-P
                    WiFi.reconnect(); //try to connect
                    DBG(Serial.println("WiFi try to reconnect");)
                }
            }
            break;
        case 1: //hotspot
            if(WiFi.getMode() != WIFI_MODE_AP){
                WiFi.disconnect();
                WiFi.softAP("MrClock_v1","",5,0,8,false);
                //IPAddress IP = WiFi.softAPIP();
            }
            break;
        case 2: //OFF
            if(WiFi.getMode() != WIFI_MODE_NULL){
                WiFi.disconnect();
                WiFi.mode(WIFI_OFF);
            }
            break;
        default:
            WiFi.disconnect();
            WiFi.mode(WIFI_OFF);
    }
}
//display strength of WiFi - base height 10px, width 14px, actual size is multplicator
void rssiWiFi(uint16_t x0, uint16_t y0, uint16_t Color, uint16_t Size){
    if(Size<=0){
        Size=1;
    }
    int rssi = WiFi.RSSI(); //get strength RSSI
    uint32_t wCross = TFT_BLACK;
    uint32_t wColor5 = 0x10A1;
    uint32_t wColor25 = 0x10A1;
    uint32_t wColor50 = 0x10A1;
    uint32_t wColor75 = 0x10A1;
    uint32_t wColor100 = 0x10A1;
    if (WiFi.status() == WL_CONNECTED){
        //lowest strength
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
void MRclock(int xpos, int ypos, int hh, int mm, int stat, int font){
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
    String sClock="";
    //Correction for hours include leading zero
    if (hh < 10) {
        sClock = "0";
    }
    sClock = sClock + hh + ":";
    //correction for minutes
    tft.setTextColor(color, TFT_BLACK);
    if (mm < 10) {
        sClock = sClock + "0";
    }
    sClock = sClock + mm;
    tft.drawString(sClock, xpos, ypos, font);

}
//***************************************************************************************************************************************************

//! ***********************************************************************************************
//!   SETUP
//! ***********************************************************************************************
void setup()
{
    Serial.begin(115200);
    delay(500);
    Serial.printf("Start MrClock v1 display firmware %s", version);

    //menu settings
    nav.idleTask=idle;//point a function to be used when menu is suspended
    nav.timeOut = 30; //set time out of menu
    nav.idleOn(); //menu will start on idle state, press select to enter menu
    options->invertFieldKeys=true; //Swap buttons for editting values
    mainMenu.dirty = true; //let update menu live
    subSettings.dirty = true; //let update menu live

    nav.showTitle=false; //hide menu title
    //disable of menu
    mainMenu[4].disable();  //upgrade firmware, enabled only when is available new firmware
    if(clockMode==0){ //setting for time and speed is available only in server mode of clock
        subSettings[1].disable();   //time setting
	    subSettings[2].disable();   //speed setting
    }else{
        subSettings[1].enable();   //time setting
	    subSettings[2].enable();   //speed setting
    }

    //setup buttons
    btnUp.begin(BUTTON_2);
    btnUp.setLongClickTime(700);
    btnUp.setDebounceTime(10);
    btnUp.setClickHandler(btnClick);
    btnUp.setLongClickDetectedHandler(btnLongClick);    //change from setLongClickHandler for solving "Long press button react after release #3"

    btnDwn.begin(BUTTON_1);
    btnDwn.setLongClickTime(700);
    btnDwn.setDebounceTime(10);
    btnDwn.setClickHandler(btnClick);
    btnDwn.setLongClickDetectedHandler(btnLongClick);   //change from setLongClickHandler for solving "Long press button react after release #3"

    //Setting of LCD
    tft.init();
    tft.setRotation(1);
    tft.setSwapBytes(true);
    tft.fillScreen(TFT_BLACK);

    if (TFT_BL > 0) { // TFT_BL has been set in the TFT_eSPI library in the User Setup file TTGO_T_Display.h
         pinMode(TFT_BL, OUTPUT); // Set backlight pin to output mode
         digitalWrite(TFT_BL, TFT_BACKLIGHT_ON); // Turn backlight on. TFT_BACKLIGHT_ON has been set in the TFT_eSPI library in the User Setup file TTGO_T_Display.h
    }

    //ini screen
    tft.setTextFont(2);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_BLUE, TFT_BLACK);
    tft.drawString(version, 5, 5);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextFont(4);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("START", tft.width()/2, (tft.height()/2)-16);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString("MrClock display", tft.width()/2, (tft.height()/2)+16);

    //Chip ID of current device
    uint64_t chipid = ESP.getEfuseMac(); // The chip ID is essentially its MAC address(length: 6 bytes).
    uint16_t chip = (uint16_t)(chipid >> 32);
    snprintf(hostname, 23, "MrClock-%04X%08X", chip, (uint32_t)chipid);

    EEPROM.begin(4096); //ini EEPROM
    if(EEPROM.readByte(0)>3){ //very simple test if device has fresh installation.
        //new ini of EEPROM
        EEPROM.writeByte(0,0);  //WiFi mode 0-client
        EEPROM.writeByte(1,0);  //Language 0-English
        EEPROM.writeByte(2,0);  //clock mode 0-client
        EEPROM.writeByte(3,1);  //clock speed
        EEPROM.writeByte(4,0);  //MrClock hours
        EEPROM.writeByte(5,0);  //MrClock minutes
        EEPROM.commit();
        delay(500);
    }else{  //get stored data from EEPROM
        modeWiFi = EEPROM.readByte(0);
        lang = EEPROM.readByte(1);
        clockMode = EEPROM.readByte(2);
        mrSetSpeed = EEPROM.readByte(3);
        mHH = EEPROM.readByte(4);
        mMM = EEPROM.readByte(5);
    }

    MrSpeed=1000/mrSetSpeed;

   // WiFi
    if(modeWiFi==0){ //Client mode
        DBG(Serial.println(hostname);)
        WiFi.mode(WIFI_STA);
        WiFi.setHostname(hostname); //define hostname

        DBG(Serial.printf("Check if is saved WiFi: %i\n", wm.getWiFiIsSaved());)
        if(wm.getWiFiIsSaved()==true){
            wm.setConnectTimeout(20); // how long to try to connect for before continuing
            bool res = wm.autoConnect(); //connect to WiFi via WiFiManager

            DBG(
            if(!res) {
                Serial.println("Failed to connect");
            }
            else {
                Serial.println("connected...");
            }
            )
        }

        //remote upload firmware
        esp32FOTA.setManifestURL( manifest_url );
        esp32FOTA.printConfig();
        DBG(Serial.println("Check if is available new firmware");)
        //Show there is new firmware
        update = esp32FOTA.execHTTPcheck();
    }else{
        delay(1000);
    }
    if(modeWiFi==1){    //server mode
        WiFi.disconnect();
        WiFi.softAP("MrClock_v1","",5,0,8,false);
    }
    tft.fillScreen(TFT_BLACK); //blank display content
}

//! ***********************************************************************************************
//! MAIN LOOP
//! ***********************************************************************************************
void loop()
{
    button_loop();

    //choose clock control
    switch (clockMode){
        case 0: //client mode
            mrPacket_client();
            break;
        case 1: //server mode
            mrPacket_server();
            break;
        default:
            mrPacket_client();
            break;
    }

    nav.poll();//this device only draws when needed
    //base screen show
    if (nav.sleepTask && wm.getConfigPortalActive()==false){
        // Show game time
        #ifdef TD
            tft.setTextSize(1);
            tft.setTextDatum(TL_DATUM);
            MRclock(-4,0,mHH,mMM,MrClock_status,8);
        #else //TDS3
            tft.setTextSize(1);
            tft.setTextDatum(MC_DATUM);
            MRclock(tft.width()/2,(tft.height()-55)/2,mHH,mMM,MrClock_status,8);
        #endif

        //draw bargraph with running seconds
        int yBar = tft.height()-49;
        tft.drawSmoothRoundRect(0,yBar,3,3,tft.width()-1,15,TFT_BLUE,TFT_BLACK);
        int bar = mSS * ((tft.width()-2)/60);
        tft.fillSmoothRoundRect(1,yBar+1,bar,13,3,TFT_YELLOW,TFT_BLACK);
        tft.fillSmoothRoundRect(bar+1,yBar+1,tft.width()-2-bar,13,3,TFT_BLACK,TFT_BLACK);

        tft.setTextFont(4); //4 26x14 pixel should be 32??
        tft.setTextSize(1);
        tft.setTextDatum(TL_DATUM);

        // WiFi status
        switch (modeWiFi){
            case 0: //Client
                //Show RSSI
                rssiWiFi(5, tft.height()-32, TFT_BLUE,3);
            break;
            case 1: //Access point
                tft.setTextColor(TFT_GREEN1,TFT_BLACK);
                tft.drawString("AP",10,tft.height()-24,4);
            break;
            case 2: //OFF
                tft.setTextColor(TFT_RED,TFT_BLACK);
                tft.drawString("OFF",5,tft.height()-24,4);
                clockMode = 1; //when WiFi is off clock can't be in client mode
            break;
            default:
                tft.setTextColor(TFT_RED,TFT_BLACK);
                tft.drawString("N/A",5,tft.height()-24,4);
            break;
        }

        // Show mode C - client, S - server, A - stand alone server + WiFi off
        tft.setTextColor(TFT_GREY1,TFT_BLACK);
        if(clockMode == 0){ // clock in client mode
            tft.drawString("C",60,tft.height()-24);
        }else{
            if(modeWiFi == 2){ //clock in server mode
                tft.drawString("A",60,tft.height()-24); //when WiFi is off is clock are running stand alone
            }else{
                tft.drawString("S",60,tft.height()-24); //server mode
            }
        }

        // show update info
        if (update)  {
            tft.setTextFont(4);
            tft.setTextSize(1);
            tft.setTextDatum(TC_DATUM);
            tft.setTextColor(TFT_RED, TFT_BLACK);
            tft.drawString("! UPD !", tft.width()/2, tft.height()-24);
            mainMenu[4].enable();   //Enable upgrade menu
        }

        //showing current game speed
        if(MrSpeedPrev!=MrSpeed){
            MrSpeedPrev=MrSpeed; //store current speed
            tft.fillRect(tft.width()-40,tft.height()-30,39,30,TFT_BLACK); //delete previous value
        }
        tft.setTextColor(TFT_BLUE,TFT_BLACK);
        if (MrSpeed!=0){
            tft.drawNumber(1000/MrSpeed,tft.width()-40,tft.height()-24); //MrSpeed is in miliseconds!
        }

        nav.doNav(navCmd(idxCmd,1)); //reset menu to top

        WiFimode(modeWiFi); //WiFi control
    }
    //Control for WiFiManager
    //Solving issue "Can't exit when is AP setting running #4"
    if(wm.getConfigPortalActive()==true){
        wm.process();
        if(wmTimeout<=millis() && WiFi.softAPgetStationNum()==0){ 
            DBG(Serial.println("WiFiManager timeout exit ConfigAP");)
            wm.stopConfigPortal();
        }
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextFont(4);
        tft.setTextDatum(MC_DATUM);
        tft.drawString("Connect to config AP", tft.width() / 2, (tft.height()/2)-51);
        tft.setTextColor(TFT_GREEN1, TFT_BLACK);
        tft.drawString("\"MrClock_v1\"", tft.width() / 2, tft.height()/2-16);
        tft.drawString("pwd:\"mrclockv1\"", tft.width() / 2, tft.height()/2+16);
        tft.setTextColor(TFT_YELLOW);
        tft.drawString("http://192.168.4.1", tft.width()/2, (tft.height()/2)+51);
        WMActive=true;

    }else if(WMActive==true){
        tft.fillScreen(TFT_BLACK);
        WMActive=false;
    }
    //delay(50); //safe for ESP32
}