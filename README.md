![FOSSA](https://user-images.githubusercontent.com/33088785/169420554-1b5a132a-3235-44ac-9ede-35d7c592e6e7.png)

## OFFLINE, FOSS, CHEAP, BILLS/COINS, EASY CONFIG WEB PORTAL 


> <i>Join our <a href="https://t.me/makerbits">telegram support/chat</a>.</i>

## Video tutorial

(coming soon)

## Parts (PROJECT COST UNDER £200!)
* ESP32 WT32-SC01 **£20**
* Generic USB cable, with data (used to make the Bill Acceptor config cable) **£2**
* DG600F(S) Multi Coin Acceptor **£30**
* NV10USB+ bill acceptor (Seems to be plenty 2nd hand ones on ebay) **£70**
* Storage box (the readily available "aluminum medicine box" boxes on amazon are perfect) **£30**
* Screw Terminal block **£1**
* 12v power supply (12v battery also works well, for unplugged version) **£8**
* 12v to 5v step down converter **£5**
* Male/female GPIO jumpers **£5**

![169537001-983c7e17-1163-4f13-9ac7-c695e0e7277f](https://user-images.githubusercontent.com/33088785/188747774-be2a7ef2-e894-4de0-8333-c38bff823510.jpeg)

## Coin acceptor wiring

![169517488-65bfba37-0c9c-4dc4-9533-c6c4517cc1ff](https://user-images.githubusercontent.com/33088785/188748943-960a15fd-f0c8-48e9-870a-af6cde1a3b31.png)

## Bill acceptor wiring

![169518370-2bdf7acd-e5f9-4d34-bd34-26854b805704](https://user-images.githubusercontent.com/33088785/188748970-7f463a3b-0594-4902-b8c9-0e084029618d.png)

## Installing arduino + libraries

Install the Arduino IDE,<br>
https://www.arduino.cc/en/Main/Software

Install the ESP32 hardware,<br>
https://github.com/espressif/arduino-esp32#installation-instructions

Copy the libraries from this projects <a href="/libraries">libraries</a> folder, to your `"/Arduino/libraries"` folder (usually in OS `"Home"` directory)

![BITCOIN](https://i.imgur.com/mCfnhZN.png)

## Compiling/uploading

* Connect your ESP32 and hit upload. 
* Tap the reset button on the ESP32 when Arduino has finished compiling and you get the little dots `...---...---` in monitor.

## Configuring

* Go to your LNbits install (you can use our demo server for testing).
* Enable LNURLDevices extension.
* Create an ATM attached to wallet, select a currency and set percentage profit
* Copy the LNURLATM string

![image](https://user-images.githubusercontent.com/33088785/169524860-203a6c07-eb61-4b68-b493-098ca6333c01.png)

Hit button on ATM boot (when you see the logo screen to trigger access point).

* Connect to the portal over you phone (default password: `ToTheMoon`).
* Enter your credentials
* Reboot ATM
* Profit
