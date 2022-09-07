![FOSSA](https://user-images.githubusercontent.com/33088785/169420554-1b5a132a-3235-44ac-9ede-35d7c592e6e7.png)

## OFFLINE, FOSS, CHEAP, BILLS/COINS, EASY CONFIG WEB PORTAL 


> <i>Join our <a href="https://t.me/makerbits">telegram support/chat</a>.</i>

## Video tutorial

(coming soon)

## Parts (PROJECT COST UNDER £200!)

Can run with a coin mech and bill acceptor, or either on their own.
* ESP32 WT32-SC01 **£20**
* Generic USB cable, with data (used to make the Bill Acceptor config cable) **£2**
* DG600F(S) Multi Coin Acceptor **£30**
* NV10USB+ bill acceptor (Seems to be plenty 2nd hand ones on ebay) **£70**
* Box ("aluminum medicine box" on amazon **£30**), ("Amazon Basic Home Safe", for more secure solution **£70**)
* Screw Terminal block **£1**
* 12v power supply (12v battery also works well, for unplugged version) **£8**
* 12v to 5v step down converter **£5**
* Male/female GPIO jumpers **£5**

![image](https://user-images.githubusercontent.com/33088785/188751337-f1a946d6-0424-42f6-8bc2-2f06f87d78a1.png)

## Construction

### Coin acceptor wiring

![169517488-65bfba37-0c9c-4dc4-9533-c6c4517cc1ff](https://user-images.githubusercontent.com/33088785/188748943-960a15fd-f0c8-48e9-870a-af6cde1a3b31.png)

### Bill acceptor wiring

![169518370-2bdf7acd-e5f9-4d34-bd34-26854b805704](https://user-images.githubusercontent.com/33088785/188748970-7f463a3b-0594-4902-b8c9-0e084029618d.png)

### Mounting in box

Use the templates provided <a href="cuttingTemplate.pdf"here</a>. print out standard UK A4. Its useful if the pins on the bill acceptor and coin mech are accessible.

* For the `Aluminim Storage Box` solution, holes can be cut with a sharp knife (clearly not secure, but fine for somewhere you can keep an eye on the ATM or for demoes).

* For the `Home Safe` solution, holes can be cut with angle grinder and a very thin cutter. (If you have not used an angle grinder before, don't be scared, they're cheap, easy enought to use, and very useful. Just take your time and wear safetly equipment.)

We use CT1 sealent/adhesive (or similar) for mounting screen, although the screen has screw points, which should prob be used for added security.

## Installing arduino + libraries

Install the Arduino IDE,<br>
https://www.arduino.cc/en/Main/Software

Install the ESP32 hardware,<br>
https://github.com/espressif/arduino-esp32#installation-instructions

Copy the libraries from this projects <a href="/libraries">libraries</a> folder, to your `"/Arduino/libraries"` folder (usually in OS `"Home"` directory)

![BITCOIN](https://i.imgur.com/mCfnhZN.png)
