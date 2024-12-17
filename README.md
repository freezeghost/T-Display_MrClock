# T-Display_MrClock

MrClock game watches running on LILYGO T-Display and T-Display-S3.
![Comparision between T-DISPLAY and T-DISPLAY-S3 in enclosure](images/TD_vs_TDS3.jpeg)

Whole project is provided for free and personal use only. Not for commercial use!

On main display are shown 4 statuses
MrClock itself
bargraph is showing seconds. It is calculated according to speed. This is due to quite long delay between
receiving of packets, usually it is aprox 2 sec, but time to time packet is lost (due to UDP) than delay
can be up to 15 sec.
Last row in display shows few statuses:
strength of WiFi signal with showing disconection
mode C - client, S - server, A - stand alone (shown only when WiFi is off and clock is in server mode).
In long gap can be diplayed red "! UPD !" - signaling availability of firmware update here at github.
Small number in right bottom coner showing speed of MrClock / timekeeper in fact fastspeed clock.

clock are showing statuses of MRClock as follow:

## MrClock is available

RED  clock is STOPPED

![clock is STOPPED](images/clk_stopped.JPEG)

GREEN clock is RUNNING

![clock is RUNNING](images/clk_running.JPEG)

## MrClock is not available

GREY previously was stopped

![previously was stopped](images/clk_stopped_no_server.JPEG)

ORANGE previously was running, shown time is calculated from previous speed

![previously was running](images/clk_running_no_server.JPEG)

Delay between MrClock is available and lost signal is setted at 20 sec due to time to time lost packets.

Since version 0.3.0 is available menu for access to menu press top button
Move up and down use both button :-)
Enter to setting long press top button (>700ms)
For acknowledge also long press top button

Structure of menu:

- Clock client/server
- WiFi Client/Hotspot/off
- Settings
  - Start setting AP (for setting of SSID through WiFiManager)
  - Time (available only at clock server mode)
  - Speed (available only at clock server mode)
  - Back
- About
- UPGRADE (available only when is ready new Upgrade on github, long press top button)
- Exit

For connection to SSID (WiFi) use menu access.
How to connect is shown on display. After start of WiFiManager you have 60sec for connection.
Then WiFiManager is automaticaly shutdown.
With WiFimanager you can also delete stored credentials of set WiFi.

When on display is showen red ! UPD ! is ready new firmware version. On firmware lower then 0.3.0 for update press top button for install.
On newer firmware go to menu and start there:-).
During update do not disconnect power supply! Whole process can take up to 5 minutes.

3D file is made with DesignSpark Mechanical version 6.0.3. Software is provided for free of charge at RS-Componnets.
Screws for closing I used woodscrew lenght 11mm dia 2.5mm
