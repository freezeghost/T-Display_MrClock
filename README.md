# T-Display_MrClock

MrClock game watches running on LILYGO T-Display and T-Display-S3.
![](image/TD_vs_TDS3.jpeg)

Whole project is provided for free and personal use only. Not for commercial use!

On main display are shown 4 statuses
MrClock itself
bargraph is showing seconds. It is calculated according to speed. This is due to quite long delay between
receiving of packets usually is aprox 2 sec between, but time to time packet is lost (due to UDP) and delay
can be up to 15 sec.
streght of WiFi signal with showing disconection
Small number in right bottom coner showing speed of MrClock

clock are showing statuses of MRClock as follow:

|  MrClock is available                                                     |
|---------------------------------------------------------------------------|
| RED  | clock are STOPPED                                                  |
|![](image/clk_stopped.JPEG)												|
|GREEN | clock is RUNNING                                                   |
|![](image/clk_running.JPEG)												|
|---------------------------------------------------------------------------|
|  MrClock is not available                                                 |
|---------------------------------------------------------------------------|
| GREY |previously was stopped                                              |
|![](image/clk_stopped_no_server.JPEG)												|
|ORANGE|previously was running, shown time is calculated from previous speed|
|![](image/clk_running_no_server.JPEG)												|

Delay between MrClock is available and lost signal is setted at 20 sec due to time to time lost packets.

For connection to SSID (WiFi) long press bottom button (>1000ms). Bottom mean, that buttons are on right side from display.
How to connect is shown on display. After start of WiFiManager you have 60sec for connection.
Then WiFiManager is automaticaly shutdown.
With WiFimanager you can also delete stored credentials of set WiFi.

When on display is showen red ! UPD ! is ready new firmware version. For update press top button for install.

3D file is made with DesignSpark Mechanical version 6.0.3. Software is provided for free of charge at RS-Componnets.
Screws for closing I used woodscrew lenght 11mm dia 2.5mm

I changed folder for library TFT_eSPI due to future posible switch between T-Display and T-Display-S3