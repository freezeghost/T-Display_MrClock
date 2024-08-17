#include <menu.h>
#include <menuIO/serialIO.h>
#include <menuIO/TFT_eSPIOut.h>
// #include <menuIO/chainStream.h>
#include <menuIO/esp8266Out.h>//must include this even if not doing web output...

using namespace Menu;

//hide menu solving issue "a graphics problem by about #8"
//core voids must be at the end of uiMenu.h :-)
void showAbout();
void doUpgrade();
void startWiFiManager();
void setSpeed();
void EnableMnu();
void DisableMnu();

//when menu is suspended
result idle(menuOut& o,idleEvent e) {
  o.clear();
  if (e==idleStart) {
    //clear screen
    tft.fillScreen(TFT_BLACK);
    DBG(Serial.println("Clear TFT result IDLE");)
    //Store settings
    if(EEPROM.readByte(0)!=modeWiFi){EEPROM.writeByte(0,modeWiFi);} //WiFi mode
    if(EEPROM.readByte(1)!=lang){EEPROM.writeByte(1,lang);} //Language
    if(EEPROM.readByte(2)!=clockMode){EEPROM.writeByte(2,clockMode);} //Clock mode

    if(clockMode==1){ //When is clock in server mode store data
      if(EEPROM.readByte(3)!=mrSetSpeed){EEPROM.writeByte(3,mrSetSpeed);}  //clock speed
      EEPROM.writeByte(4,mHH);  //MrClock hours
      EEPROM.writeByte(5,mMM);  //MrClock minutes
    }
    EEPROM.commit();
  }
  return proceed;
}

//define a pad style menu (single line menu)
PADMENU(padTime,"Time ",doNothing,noEvent,wrapStyle
  ,FIELD(mHH,"",":",0,23,1,0,doNothing,noEvent,wrapStyle)
  ,FIELD(mMM,"","",0,59,1,0,doNothing,noEvent,wrapStyle)
);

SELECT(clockMode, selClock, "Clock ", doNothing, noEvent, wrapStyle
  ,VALUE("Client",0,DisableMnu,enterEvent)
  ,VALUE("Server",1,EnableMnu,enterEvent)
);

SELECT(modeWiFi,selWiFi,"WiFi ",doNothing,noEvent,wrapStyle //can be changed to CHOOSE with open sub menu and display all posibilities
  ,VALUE("Client mode",0,doNothing,noEvent)
  ,VALUE("Hotspot",1,doNothing,noEvent)
  ,VALUE("OFF",2,doNothing,noEvent)
);

MENU(subSettings,"Settings",doNothing,noEvent,wrapStyle
  ,OP("Start setting AP",startWiFiManager,enterEvent)
  ,SUBMENU(padTime)
  ,FIELD(mrSetSpeed,"Speed ","",1,10,1,0,setSpeed,enterEvent,wrapStyle)
  ,EXIT("<< Back")
);

MENU(mainMenu,"Main menu",doNothing,noEvent,wrapStyle
  ,SUBMENU(selClock)
  ,SUBMENU(selWiFi)
  ,SUBMENU(subSettings)
  ,OP("About",showAbout,enterEvent)
  ,OP("UPGRADE",doUpgrade,enterEvent)
  ,EXIT("EXIT")
);

// define menu colors --------------------------------------------------------
//  {{disabled normal,disabled selected},{enabled normal,enabled selected, enabled editing}}

#define Black RGB565(0,0,0)
#define Red	RGB565(255,0,0)
#define Green RGB565(0,255,0)
#define Blue RGB565(0,0,255)
#define Gray RGB565(50,50,50) //128,128,128
#define LighterRed RGB565(255,150,150)
#define LighterGreen RGB565(150,255,150)
#define LighterBlue RGB565(150,150,255)
#define DarkerRed RGB565(128,0,0)
#define DarkerGreen RGB565(0,150,0)
#define DarkerBlue RGB565(0,0,150)
#define DarkerYellow RGB565(50,50,0)
#define Cyan RGB565(0,255,255)
#define Magenta RGB565(255,0,255)
#define Yellow RGB565(255,255,0)
#define White RGB565(255,255,255)

const colorDef<uint16_t> colors[6] MEMMODE={
  { //background color
    {
      (uint16_t)Black, //disabled item
      (uint16_t)Black //disabled item cursor on
    },
    {
      (uint16_t)Black, //not selected (cursor out)
      (uint16_t)DarkerBlue, //cursor on
      (uint16_t)DarkerBlue //editing item select, field, edit
    }
  },
  { //foreground color (text)
    {
      (uint16_t)Gray, //disabled menu
      (uint16_t)Gray  //disabled menu cursor on
    },
    { //menu text color
      (uint16_t)DarkerBlue, //menu item
      (uint16_t)White, //cursor on
      (uint16_t)Gray //Edit part where is no cursor on
    }
  },
  { //foreground color of value
    { //disabled item
      (uint16_t)DarkerYellow, //not selected (cursor out)
      (uint16_t)DarkerYellow //cursor on
    },
    {
      (uint16_t)Yellow, //not selected (cursor out)
      (uint16_t)Yellow, //cursor on
      (uint16_t)Red    //editing
    }
  },
  { // color of unit (Field)
    { //disabled item
      (uint16_t)DarkerYellow,  //not selected (cursor out)
      (uint16_t)DarkerYellow   //cursor on
    },
    {
      (uint16_t)Yellow, //not selected (cursor out)
      (uint16_t)Yellow, //cursor on
      (uint16_t)Red     //editing
    }
  },
  { // color of cursor
    {
      (uint16_t)White, //! ????
      (uint16_t)Gray   //! ????
    },
    {
      (uint16_t)Black,      //Frame not selected item
      (uint16_t)DarkerBlue, //Frame of cursor
      (uint16_t)White //! ???
    }
  },//cursorColor
  {
    {
      (uint16_t)White,
      (uint16_t)Yellow
    },
    {// title Color
      (uint16_t)DarkerRed, //Background
      (uint16_t)White, //foreground color of text
      (uint16_t)White
    }
  },
};

#define MAX_DEPTH 5

serialIn serial(Serial);

//define serial output device
idx_t serialTops[MAX_DEPTH]={0};
serialOut outSerial(Serial,serialTops);

#ifdef TD
  #define GFX_WIDTH 240
  #define GFX_HEIGHT 135
#else //TDS3
  #define GFX_WIDTH 320
  #define GFX_HEIGHT 170
#endif

#define fontW 12
#define fontH 27 //25

const panel panels[] MEMMODE = {{0, 0, GFX_WIDTH / fontW, GFX_HEIGHT / fontH}};
navNode* nodes[sizeof(panels) / sizeof(panel)]; //navNodes to store navigation status
panelsList pList(panels, nodes, 1); //a list of panels and nodes
idx_t eSpiTops[MAX_DEPTH]={0};
TFT_eSPIOut eSpiOut(tft,colors,eSpiTops,pList,fontW,fontH+1);
menuOut* constMEM outputs[] MEMMODE={&outSerial,&eSpiOut};//list of output devices
outputsList out(outputs,sizeof(outputs)/sizeof(menuOut*));//outputs list controller

NAVROOT(nav,mainMenu,MAX_DEPTH,serial,out);

unsigned long idleTimestamp = millis();

void showAbout(){
  nav.exit(); //hide menu solving issue "a graphics problem by about #8"
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextFont(4);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("MrClock display", tft.width() / 2, (tft.height()/2)-51);
  tft.setTextColor(TFT_GREEN1, TFT_BLACK);
  #ifdef TD
    tft.drawString("github.com/", tft.width() / 2, tft.height()/2-16);
    tft.drawString("freezeghost", tft.width() / 2, tft.height()/2+16);
  #else
    tft.drawString("github.com/freezeghost", tft.width() / 2, tft.height()/2-16);
  #endif
  tft.setTextColor(TFT_BLUE);
  tft.drawString(version, tft.width()/2, (tft.height()/2)+51);
  delay(5000);
}

void doUpgrade(){
  nav.exit(); //hide menu solving issue "a graphics problem by about #8"
  DBG(Serial.println("New version of firmware!!!");)
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.setTextFont(4);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("! UPGRADING !", tft.width()/2, (tft.height()/2)-16);
  tft.drawString("! FIRMWARE !", tft.width()/2, (tft.height()/2)+16);
  esp32FOTA.execOTA();
  tft.fillScreen(TFT_BLACK);
}

void startWiFiManager(){
  //need to exit twice due to submenu
  nav.exit(); //hide menu solving issue "a graphics problem by about #8"
  nav.exit(); //hide menu solving issue "a graphics problem by about #8"
  WMConfigShow = true;
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
  wm.setConfigPortalTimeout(60); // auto close configportal after n seconds 60
  wm.setAPClientCheck(true); // avoid timeout if client connected to softap
  wm.startConfigPortal("MrClock_v1","mrclockv1");
}

void setSpeed(){
  MrSpeed=1000/mrSetSpeed;
}

void EnableMnu(){
  MrClock_status=1;
  //Solving issue "No change of menu when change mode client / server #5"
  subSettings[1].enable();   //time setting
  subSettings[2].enable();   //speed setting
}

void DisableMnu(){
  MrClock_status=1;
  //Solving issue "No change of menu when change mode client / server #5"
  subSettings[1].disable();   //time setting
	subSettings[2].disable();   //speed setting
}