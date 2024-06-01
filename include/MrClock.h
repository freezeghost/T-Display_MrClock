/*
UPD packet of FastClock
each row is terminated with CRLF (0x0D, 0x0A)

fastclock
version=2
name=MRclock
ip-address=
ip-port=
text=
clocktype=fastclock
active=yes
speed=4
clock=13:14:15
weekday=none
interval=2

send and recieve is done at multicast IP and port
IP - 239.50.50.20
port usualy 2000
*/

#include <Arduino.h>
#include <WiFi.h>

//#define MrDEBUG_SER //debug over serial

//Start debug via serial port
#ifdef MrDEBUG_SER
  #define MrDBG(x) x
#else
  #define MrDBG(x)
#endif

//external variables
//those variables should be copied to main program!
extern long MrClock_TimeOut; //speed of MrClock for offline calculation - display
extern long mrClk_TimeOut;  //delay of incoming packet from MrClock server
extern int mHH;
extern int mMM;
extern int mSS;
extern int MrSpeed; //speed of MrClock
extern byte MrClock_status;

extern byte clockMode;
extern byte mrSetSpeed;

String rec, Parameter, Value, message;
bool multiStart = false;
int ini, last;
long TOdelay=20000; //when MrClock is stopped can be delay up to 15sec! When running usually is around 2sec
long sndTick=1000; //timer for sendig data at server mode

char packetBuffer[650]; //buffer for receive
IPAddress multicastAddress(239,50,50,20); //multicast IP address of MRclock
unsigned int multicastPort = 2000; //usual port for MRclock
WiFiUDP udpClock; //receive datagram from Mrclock

void offLine(){
  //Offline calculation of MrClock
  if (MrClock_status >= 2 && (MrClock_TimeOut <= millis() || MrSpeed > millis())){
    MrClock_TimeOut = MrSpeed  + millis(); //timeout for calculation of MrClock
    mSS++; //seconds
    if (mSS>=60){
      mSS=0;
      mMM++; //minutes
    }
    if (mMM>=60){
      mMM=0;
      mHH++; //hours
    }
    if (mHH>=24){
      mHH=0; //overlap day
    }
  }
}

void mrPacket_client(){
//*************************************************************************************************
  //start receiving multicast packet
  if (multiStart==false && WiFi.status() == WL_CONNECTED){
    udpClock.beginMulticast(multicastAddress, multicastPort); //multicast for MrClock
    multiStart = true; //indication that multicast is running
    mrClk_TimeOut = millis() + TOdelay; //connection set timeout
    MrDBG(Serial.println("Multicast running");)
  }
//*************************************************************************************************
  //receive Packet only when multicast rec is activated
  int packetSize=0;
  if (multiStart==true){
    packetSize = udpClock.parsePacket();
  }
  if (packetSize) {
    //read packet into packetBufffer
    int len = udpClock.read(packetBuffer, 650);
    if (len > 0) {
      packetBuffer[len] = 0;
    }
    rec = packetBuffer; //copy of data to string variable due to functionality
    MrDBG(Serial.println("Packet MRclock is available");)
    MrDBG(Serial.printf("MRclock calculated %02d:%02d:%02d\r\n", mHH, mMM, mSS);)
    //deploy of MrClock packet
    ini = rec.indexOf ("clock=");
    if(ini != -1){
      if (( rec.indexOf ("\n", ini)) != -1) {
        Parameter = rec.substring (ini + 6, rec.indexOf ("\n", ini)-1);
        ini = Parameter.indexOf (":"); //first colon
        Value = Parameter.substring (0,ini+1);
        mHH = Value.toInt(); //hours
        last = Parameter.indexOf (":",ini+1); //second colon
        Value = Parameter.substring (ini+1,last);
        mMM = Value.toInt(); //minutes
        Value = Parameter.substring (last +1,Parameter.length());
        mSS = Value.toInt(); //seconds
        MrDBG(Serial.printf("MRclock -> %02d:%02d:%02d\r\n", mHH, mMM, mSS);)
      }
    }
    //looking for speed of MrClock
    ini = rec.indexOf ("speed=");
    if(ini != -1){
      if (( rec.indexOf ("\n", ini)) != -1) {
        Value = rec.substring (ini + 6, rec.indexOf ("\n", ini)-1);
        MrSpeed = 1000/Value.toInt(); //setting for offline counter
        MrDBG(Serial.printf("MrClock speed = %s (%i ms)\r\n", Value, MrSpeed);)
        MrClock_TimeOut = MrSpeed  + millis(); //offline calculation timeout
      }
    }

    //check if clock is running active=yes / active=no
    if (rec.indexOf ("active=yes")!=-1){
      MrClock_status=2; //clock running
      MrDBG(Serial.println("Clock is running");)
    }
    if (rec.indexOf ("active=no")!=-1){
      MrClock_status=1; //clock stopped
      MrDBG(Serial.println("Clock is stopped");)
    }
    MrDBG(Serial.println("");)
    mrClk_TimeOut = millis() + TOdelay; //connection set timeout
  }

  offLine(); //offline calculation of time

  //timeout incoming MRclock
  if (mrClk_TimeOut <=millis()){
    if(MrClock_status>=2){
      MrClock_status=3; //running  and no connection
    } else{
      MrClock_status=0; // stopped and no connection
    }
    MrDBG(Serial.printf("MrClock is not available delay (%ld ms)\r\n", millis() - mrClk_TimeOut);)
    mrClk_TimeOut = millis() + TOdelay; //connection set timeout
  }
}

//send MrClock packet
void mrPacket_server(){
  offLine(); //offline calculation of time
  if (sndTick <= millis()){
    sndTick = 1000 + millis(); //send every 1000ms packet

    String active="";
    if (MrClock_status>=2){
      active="yes";
    }else{
      active="no";
    }
    if(WiFi.getMode() != WIFI_MODE_NULL){ // ensure that WiFi is connected!
          message = "fastclock\r\n\
version=2\r\n\
name=MRclock_ESP32\r\n\
ip-address=\r\n\
ip-port=\r\n\
text=\r\n\
clocktype=fastclock\r\n\
active=" + active + "\r\n\
speed=" + String((1000/MrSpeed), DEC) + "\r\n\
clock=" + String(mHH, DEC) + ":" + String(mMM, DEC) + ":" + String(mSS, DEC) + "\r\n\
weekday=none\r\n\
 \r\n";
      uint8_t buffer[200];
      message.getBytes(buffer, message.length());
      udpClock.beginPacket(multicastAddress, multicastPort);
      udpClock.write(buffer, message.length());
      udpClock.endPacket();
      MrDBG(Serial.println(message);)
    }
  }
}