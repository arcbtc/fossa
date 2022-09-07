
// FOSSA WITH ACCESS-POINT

//========================================================//
//============EDIT IF USING DIFFERENT HARDWARE============//
//========================================================//

bool format = false; // true for formatting FOSSA memory, use once, then make false and reflash

#define BTN1 39 //Screen tap button

#define RX1 32 //Bill acceptor
#define TX1 33 //Bill acceptor

#define TX2 4 //Coinmech
#define INHIBITMECH 2 //Coinmech

//========================================================//
//========================================================//
//========================================================//

#include <WiFi.h>
#include <WebServer.h>
#include <FS.h>
#include <SPIFFS.h>
using WebServerClass = WebServer;
fs::SPIFFSFS &FlashFS = SPIFFS;
#define FORMAT_ON_FAIL true

#include <AutoConnect.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <Wire.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include <JC_Button.h>

#include <Hash.h>
#include "qrcoded.h"
#include "Bitcoin.h"

#define PARAM_FILE "/elements.json"

String qrData;
String password;
String apPassword = "ToTheMoon1"; //default WiFi AP password
String baseURLATM;
String secretATM;
String currencyATM = "";

int bills;
float coins;
float total;
int maxamount;
int charge;

bool billBool = true;
bool coinBool = true;

int moneyTimer = 0;

// Coin and Bill Acceptor amounts
int billAmountInt[5] = {5, 10, 20, 50, 100};
float coinAmountFloat[6] = {0.02, 0.05, 0.1, 0.2, 0.5, 1};
int billAmountSize = sizeof(billAmountInt) / sizeof(int);
float coinAmountSize = sizeof(coinAmountFloat) / sizeof(float);

HardwareSerial SerialPort1(1);
HardwareSerial SerialPort2(2);

Button BTNA(BTN1);

/////////////////////////////////////
////////////////PORTAL///////////////
/////////////////////////////////////

bool triggerAp = false; 

String content = "<h1>ATM Access-point</br>For easy variable setting</h1>";

// custom access point pages
static const char PAGE_ELEMENTS[] PROGMEM = R"(
{
  "uri": "/config",
  "title": "Access Point Config",
  "menu": true,
  "element": [
    {
      "name": "text",
      "type": "ACText",
      "value": "AP options",
      "style": "font-family:Arial;font-size:16px;font-weight:400;color:#191970;margin-botom:15px;"
    },
    {
      "name": "password",
      "type": "ACInput",
      "label": "Password",
      "value": "ToTheMoon"
    },
    {
      "name": "text",
      "type": "ACText",
      "value": "Project options",
      "style": "font-family:Arial;font-size:16px;font-weight:400;color:#191970;margin-botom:15px;"
    },
    {
      "name": "lnurl",
      "type": "ACInput",
      "label": "ATM string LNbits LNURLDevices",
      "value": ""
    },
    {
      "name": "coinmech",
      "type": "ACInput",
      "label": "Coin values comma seperated, ie 0.01,0.02,0.05,0.10,0.20,0.50,1",
      "value": ""
    },
    {
      "name": "billmech",
      "type": "ACInput",
      "label": "Note values comma seperated, ie 5,10,20",
      "value": ""
    },
    {
      "name": "maxamount",
      "type": "ACInput",
      "label": "Max withdrawable in fiat",
      "value": "20"
    },
    {
      "name": "charge",
      "type": "ACInput",
      "label": "Percentage charge for service",
      "value": "10"
    },
    {
      "name": "load",
      "type": "ACSubmit",
      "value": "Load",
      "uri": "/config"
    },
    {
      "name": "save",
      "type": "ACSubmit",
      "value": "Save",
      "uri": "/save"
    },
    {
      "name": "adjust_width",
      "type": "ACElement",
      "value": "<script type='text/javascript'>window.onload=function(){var t=document.querySelectorAll('input[]');for(i=0;i<t.length;i++){var e=t[i].getAttribute('placeholder');e&&t[i].setAttribute('size',e.length*.8)}};</script>"
    }
  ]
 }
)";


static const char PAGE_SAVE[] PROGMEM = R"(
{
  "uri": "/save",
  "title": "Elements",
  "menu": false,
  "element": [
    {
      "name": "caption",
      "type": "ACText",
      "format": "Elements have been saved to %s",
      "style": "font-family:Arial;font-size:18px;font-weight:400;color:#191970"
    },
    {
      "name": "validated",
      "type": "ACText",
      "style": "color:red"
    },
    {
      "name": "echo",
      "type": "ACText",
      "style": "font-family:monospace;font-size:small;white-space:pre;"
    },
    {
      "name": "ok",
      "type": "ACSubmit",
      "value": "OK",
      "uri": "/config"
    }
  ]
}
)";

TFT_eSPI tft = TFT_eSPI();

WebServerClass server;
AutoConnect portal(server);
AutoConnectConfig config;
AutoConnectAux elementsAux;
AutoConnectAux saveAux;

/////////////////////////////////////
////////////////SETUP////////////////
/////////////////////////////////////

void setup()  
{  
  BTNA.begin();
  
  tft.init();
  tft.setRotation(1);
  tft.invertDisplay(false);
  tft.fillScreen(TFT_BLACK);
  logo();

  int timer = 0;
  while(timer < 2000){
    BTNA.read();
    if (BTNA.wasReleased()) {
      timer = 5000;
      triggerAp = true;
    }
    timer = timer + 100;
    delay(100);
  }
  SerialPort1.begin(300, SERIAL_8N2, TX1, RX1);
  SerialPort2.begin(4800, SERIAL_8N1, TX2);

  pinMode(INHIBITMECH, OUTPUT); 

  FlashFS.begin(FORMAT_ON_FAIL);
  SPIFFS.begin(true);
  if(format == true){
    SPIFFS.format(); 
  }
  // get the saved details and store in global variables
  File paramFile = FlashFS.open(PARAM_FILE, "r");
  if (paramFile)
  {
    StaticJsonDocument<2500> doc;
    DeserializationError error = deserializeJson(doc, paramFile.readString());

    const JsonObject maRoot0 = doc[0];
    const char *maRoot0Char = maRoot0["value"];
    password = maRoot0Char;
    
    const JsonObject maRoot1 = doc[1];
    const char *maRoot1Char = maRoot1["value"];
    const String lnurlATM = maRoot1Char;
    baseURLATM = getValue(lnurlATM, ',', 0);
    secretATM = getValue(lnurlATM, ',', 1);
    currencyATM = getValue(lnurlATM, ',', 2);

    const JsonObject maRoot2 = doc[2];
    const char *maRoot2Char = maRoot2["value"];
    const String billmech = maRoot2Char;
    if(billmech == ""){
      billAmountInt[0] = getValue(billmech, ',', 0).toInt();
      billAmountInt[2] = getValue(billmech, ',', 2).toInt();
      billAmountInt[3] = getValue(billmech, ',', 3).toInt();
      billAmountInt[4] = getValue(billmech, ',', 4).toInt();
      billAmountInt[5] = getValue(billmech, ',', 5).toInt();
    }
    const JsonObject maRoot3 = doc[3];
    const char *maRoot3Char = maRoot3["value"];
    const String coinmech = maRoot2Char;
    if(coinmech == ""){
      coinAmountFloat[0] = getValue(coinmech, ',', 0).toFloat();
      coinAmountFloat[1] = getValue(coinmech, ',', 1).toFloat();
      coinAmountFloat[2] = getValue(coinmech, ',', 2).toFloat();
      coinAmountFloat[3] = getValue(coinmech, ',', 3).toFloat();
      coinAmountFloat[4] = getValue(coinmech, ',', 4).toFloat();
      coinAmountFloat[5] = getValue(coinmech, ',', 5).toFloat();
    }

    const JsonObject maRoot4 = doc[4];
    const char *maRoot4Char = maRoot4["value"];
    const String maxamountstr = maRoot4Char;
    maxamount = maxamountstr.toInt();

    const JsonObject maRoot5 = doc[5];
    const char *maRoot5Char = maRoot5["value"];
    const String chargestr = maRoot5Char;
    charge = chargestr.toInt();
  }
  else{
    triggerAp = true;
  }
  paramFile.close();
  server.on("/", []() {
    content += AUTOCONNECT_LINK(COG_24);
    server.send(200, "text/html", content);
  });
    
  elementsAux.load(FPSTR(PAGE_ELEMENTS));
  elementsAux.on([](AutoConnectAux &aux, PageArgument &arg) {
    File param = FlashFS.open(PARAM_FILE, "r");
    if (param)
    {
      aux.loadElement(param, {"password", "lnurl", "coinmech", "billmech", "maxamount", "charge"});
      param.close();
    }

    if (portal.where() == "/config")
    {
      File param = FlashFS.open(PARAM_FILE, "r");
      if (param)
      {
        aux.loadElement(param, {"password", "lnurl", "coinmech", "billmech", "maxamount", "charge"});
        param.close();
      }
    }
    return String();
  });
  saveAux.load(FPSTR(PAGE_SAVE));
  saveAux.on([](AutoConnectAux &aux, PageArgument &arg) {
    aux["caption"].value = PARAM_FILE;
    File param = FlashFS.open(PARAM_FILE, "w");
    if (param)
    {
      // save as a loadable set for parameters.
      elementsAux.saveElement(param, {"password", "lnurl", "coinmech", "billmech", "maxamount", "charge"});
      param.close();
      // read the saved elements again to display.
      param = FlashFS.open(PARAM_FILE, "r");
      aux["echo"].value = param.readString();
      param.close();
    }
    else
    {
      aux["echo"].value = "Filesystem failed to open.";
    }
    return String();
  });
  config.auth = AC_AUTH_BASIC;
  config.authScope = AC_AUTHSCOPE_AUX;
  config.ticker = true;
  config.autoReconnect = true;
  config.apid = "Device-" + String((uint32_t)ESP.getEfuseMac(), HEX);
  config.psk = password;
  config.menuItems = AC_MENUITEM_CONFIGNEW | AC_MENUITEM_OPENSSIDS | AC_MENUITEM_RESET;
  config.title = "FOSSA";
  config.reconnectInterval = 1;

  if (triggerAp == true)
  {
    digitalWrite(11, LOW);
    printMessage("Portal launched", "", "", TFT_WHITE, TFT_BLACK);
    config.immediateStart = true;
    portal.join({elementsAux, saveAux});
    portal.config(config);
    portal.begin();
    while (true)
    {
      portal.handleClient();
    }
    timer = 2000;
  }
  if(currencyATM == ""){
    digitalWrite(11, LOW);
    printMessage("Restart", "launch portal!", "", TFT_WHITE, TFT_BLACK);
    delay(99999999);
  }
}

void loop()
{
  // Turn on machines
  SerialPort1.write(184);
  digitalWrite(INHIBITMECH, HIGH);
  tft.fillScreen(TFT_BLACK);
  moneyTimerFun();
  makeLNURL();
  qrShowCodeLNURL("SCAN ME. TAP SCREEN WHEN FINISHED");
}

void printMessage(String text1, String text2, String text3, int ftcolor, int bgcolor)
{
  tft.fillScreen(bgcolor);
  tft.setTextColor(ftcolor, bgcolor);
  tft.setTextSize(5);
  tft.setCursor(30, 40);
  tft.println(text1);
  tft.setCursor(30, 120);
  tft.println(text2);
  tft.setCursor(30, 200);
  tft.setTextSize(3);
  tft.println(text3);
}

void logo()
{
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(130, 100);
  tft.setTextSize(10);
  tft.setTextColor(TFT_PURPLE);
  tft.println("FOSSA");
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(40, 170);
  tft.setTextSize(3);
  tft.println("Bitcoin Lightning ATM");
  tft.setCursor(300, 290);
  tft.setTextSize(2);
  tft.println("(GRIFF Edition)");
}

void feedmefiat()
{
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(60, 40);
  tft.setTextSize(3);
  tft.println("Bitcoin Lightning ATM");
  tft.setCursor(10, 280);
  tft.println("(feed me fiat. " + String(charge) + "% charge)");
  tft.setTextSize(10);
  tft.setCursor(160, 80);
  tft.println("SATS");
  tft.setCursor(180, 140);
  tft.println("FOR");
  tft.setCursor(160, 190);
  tft.println("FIAT!");
  delay(100);
  tft.setTextColor(TFT_GREEN);
  tft.setCursor(160, 80);
  tft.println("SATS");
  tft.setCursor(180, 140);
  tft.println("FOR");
  tft.setCursor(160, 190);
  tft.println("FIAT!");
  delay(100);
  tft.setTextColor(TFT_BLUE);
  tft.setCursor(160, 80);
  tft.println("SATS");
  tft.setCursor(180, 140);
  tft.println("FOR");
  tft.setCursor(160, 190);
  tft.println("FIAT!");
  delay(100);
  tft.setTextColor(TFT_ORANGE);
  tft.setCursor(160, 80);
  tft.println("SATS");
  tft.setCursor(180, 140);
  tft.println("FOR");
  tft.setCursor(160, 190);
  tft.println("FIAT!");
  delay(100);
}

void qrShowCodeLNURL(String message)
{
  tft.fillScreen(TFT_WHITE);
  qrData.toUpperCase();
  const char *qrDataChar = qrData.c_str();
  QRCode qrcoded;
  uint8_t qrcodeData[qrcode_getBufferSize(20)];
  qrcode_initText(&qrcoded, qrcodeData, 11, 0, qrDataChar);

  for (uint8_t y = 0; y < qrcoded.size; y++)
  {
    for (uint8_t x = 0; x < qrcoded.size; x++)
    {
      if (qrcode_getModule(&qrcoded, x, y))
      {
        tft.fillRect(120 + 4 * x, 20 + 4 * y, 4, 4, TFT_BLACK);
      }
      else
      {
        tft.fillRect(120 + 4 * x, 20 + 4 * y, 4, 4, TFT_WHITE);
      }
    }
  }

  tft.setCursor(40, 290);
  tft.setTextSize(2);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.println(message);
  
  bool waitForTap = true;
  while(waitForTap){
    BTNA.read();
    if (BTNA.wasReleased()) {
      waitForTap = false;
    }
  }
}

void moneyTimerFun()
{
  bool waitForTap = true;
  coins = 0;
  bills = 0;
  total = 0;
  while( waitForTap || total == 0){
    if(total == 0){
      feedmefiat();
    }
    if (SerialPort1.available()) {
      int x = SerialPort1.read();
       for (int i = 0; i < billAmountSize; i++){
         if((i+1) == x){
           bills = bills + billAmountInt[i];
           total = (coins + bills);
           printMessage(billAmountInt[i] + currencyATM, "Total: " + String(total) + currencyATM, "TAP SCREEN WHEN FINISHED", TFT_WHITE, TFT_BLACK);
         }
       }
    }
    if (SerialPort2.available()) {
      int x = SerialPort2.read();
      for (int i = 0; i < coinAmountSize; i++){
         if((i+1) == x){
           coins = coins + coinAmountFloat[i];
           total = (coins + bills);
           printMessage(coinAmountFloat[i] + currencyATM, "Total: " + String(total) + currencyATM, "TAP SCREEN WHEN FINISHED", TFT_WHITE, TFT_BLACK);
         }
       }
    }
    BTNA.read();
    if (BTNA.wasReleased() || total > maxamount) {
      waitForTap = false;
    }
  }
  total = (coins + bills) * 100;

  // Turn off machines
  SerialPort1.write(185);
  digitalWrite(INHIBITMECH, LOW);
}

/////////////////////////////////////
/////////////UTIL STUFF//////////////
/////////////////////////////////////

String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  const int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++)
  {
    if (data.charAt(i) == separator || i == maxIndex)
    {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }

  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void to_upper(char *arr)
{
  for (size_t i = 0; i < strlen(arr); i++)
  {
    if (arr[i] >= 'a' && arr[i] <= 'z')
    {
      arr[i] = arr[i] - 'a' + 'A';
    }
  }
}

////////////////////////////////////////////
///////////////LNURL STUFF//////////////////
////USING STEPAN SNIGREVS GREAT CRYTPO//////
////////////THANK YOU STEPAN////////////////
////////////////////////////////////////////

void makeLNURL()
{
  int randomPin = random(1000, 9999);
  byte nonce[8];
  for (int i = 0; i < 8; i++)
  {
    nonce[i] = random(256);
  }

  byte payload[51]; // 51 bytes is max one can get with xor-encryption

    size_t payload_len = xor_encrypt(payload, sizeof(payload), (uint8_t *)secretATM.c_str(), secretATM.length(), nonce, sizeof(nonce), randomPin, float(total));
    String preparedURL = baseURLATM + "?atm=1&p=";
    preparedURL += toBase64(payload, payload_len, BASE64_URLSAFE | BASE64_NOPADDING);
    
  Serial.println(preparedURL);
  char Buf[200];
  preparedURL.toCharArray(Buf, 200);
  char *url = Buf;
  byte *data = (byte *)calloc(strlen(url) * 2, sizeof(byte));
  size_t len = 0;
  int res = convert_bits(data, &len, 5, (byte *) url, strlen(url), 8, 1);
  char *charLnurl = (char *) calloc(strlen(url) * 2, sizeof(byte));
  bech32_encode(charLnurl, "lnurl", data, len);
  to_upper(charLnurl);
  qrData = charLnurl;
}

int xor_encrypt(uint8_t *output, size_t outlen, uint8_t *key, size_t keylen, uint8_t *nonce, size_t nonce_len, uint64_t pin, uint64_t amount_in_cents)
{
  // check we have space for all the data:
  // <variant_byte><len|nonce><len|payload:{pin}{amount}><hmac>
  if (outlen < 2 + nonce_len + 1 + lenVarInt(pin) + 1 + lenVarInt(amount_in_cents) + 8)
  {
    return 0;
  }

  int cur = 0;
  output[cur] = 1; // variant: XOR encryption
  cur++;

  // nonce_len | nonce
  output[cur] = nonce_len;
  cur++;
  memcpy(output + cur, nonce, nonce_len);
  cur += nonce_len;

  // payload, unxored first - <pin><currency byte><amount>
  int payload_len = lenVarInt(pin) + 1 + lenVarInt(amount_in_cents);
  output[cur] = (uint8_t)payload_len;
  cur++;
  uint8_t *payload = output + cur;                                 // pointer to the start of the payload
  cur += writeVarInt(pin, output + cur, outlen - cur);             // pin code
  cur += writeVarInt(amount_in_cents, output + cur, outlen - cur); // amount
  cur++;

  // xor it with round key
  uint8_t hmacresult[32];
  SHA256 h;
  h.beginHMAC(key, keylen);
  h.write((uint8_t *) "Round secret:", 13);
  h.write(nonce, nonce_len);
  h.endHMAC(hmacresult);
  for (int i = 0; i < payload_len; i++)
  {
    payload[i] = payload[i] ^ hmacresult[i];
  }

  // add hmac to authenticate
  h.beginHMAC(key, keylen);
  h.write((uint8_t *) "Data:", 5);
  h.write(output, cur);
  h.endHMAC(hmacresult);
  memcpy(output + cur, hmacresult, 8);
  cur += 8;

  // return number of bytes written to the output
  return cur;
}
