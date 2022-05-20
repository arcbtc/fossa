![FOSSA](https://user-images.githubusercontent.com/33088785/169420554-1b5a132a-3235-44ac-9ede-35d7c592e6e7.png)

## OFFLINE, FOSS, CHEAP, BILLS/COINS, EASY CONFIG WEB PORTAL 


> <i>Join our <a href="https://t.me/makerbits">telegram support/chat</a>.</i>

## Video tutorial

(coming soon)

## Parts (Project cost £80)
* RPI_ILI9486 3.5 inch TFT (the type the Raspiblitz uses)
* ESP32 NodeMCU 32s
* Generic USB cable, with data (Used to make the Bill Acceptor config cable)
* Breadboard (some bread boards are two small for esp32 and two are required)
* DG600F(S) Multi Coin Acceptor
* Storage box ("aluminum medicine box" you can buy from amazon works very well)
* Screw Terminal block
* 12v power supply (12v battery also works well, for unplugged version)
* 12v to 5v step down converter
* Male/female, female/female, male/male GPIO jumpers

![fossaparts](https://user-images.githubusercontent.com/33088785/169497083-730cb959-dd4c-450c-8549-457fab2fb57c.jpeg)


## Wiring the screen

![image](https://user-images.githubusercontent.com/33088785/169515768-183ccd70-8f3b-4334-a4f0-9ccccdbcbf93.png)

## Wiring the coin acceptor

| COIN ACCEPTOR PINS  | ESP32 | POWER SUPPLY |
| ------------- | ------------- | ------------- | 
| 5  | NA  | NA  |
| 4  | NA  | NA  |
| 3 (GND)  | NEGATIVE | NA  |
| 2  (Serial out) | 9  | NA  |
| 1 (12v)  | NA  | POSITIVE  |

## Installing arduino + libraries

Install the Arduino IDE,<br>
https://www.arduino.cc/en/Main/Software

Install the ESP32 hardware,<br>
https://github.com/espressif/arduino-esp32#installation-instructions

From "Manage Libraries" install,<br>
* TFT_eSPI
* ArduinoJson
* qrcode (By Richard Moore)

![BITCOIN](https://i.imgur.com/mCfnhZN.png)

## Wiring

![ESP32 GPIO Map](https://i.imgur.com/sRI8A6M.png)
