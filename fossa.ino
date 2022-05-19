
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
#include <JC_Button.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>

#include <Hash.h>
#include "qrcoded.h"
#include "Bitcoin.h"

#define PARAM_FILE "/elements.json"

HardwareSerial coinAcceptor(1);
HardwareSerial billAcceptor(2);

/////////////////////////////////
///////////CHANGE////////////////
/////////////////////////////////
         
bool format = false; // true for formatting SPIFFS, use once, then make false and reflash
int portalPin = 27; // Pin that triggers portal

/////////////////////////////////
/////////////////////////////////

byte noteInEscrow = 0;

String password;
String qrData;
String currency = "GBP";
String apPassword = "ToTheMoon1"; //default WiFi AP password
String baseURLATM;
String secretATM;
String currencyATM = "";
String dataIn = "0";
String preparedURL;

int randomPin;
int maxamount;
int credit = 0;

int billAcceptorValues[6] = {500, 1000, 2000};
int coinAcceptorValues[6] = {10, 20, 50, 100};

const byte BUTTON_PIN_A = 39;
Button BTNA(BUTTON_PIN_A);

/////////////////////////////////////
////////////////PORTAL///////////////
/////////////////////////////////////

bool triggerAp = false; 

String content = "<h1>ATM Access-point</br>For easy variable and wifi connection setting</h1>";

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
      "label": "Coin values comma seperated",
      "value": ""
    },
    {
      "name": "billmech",
      "type": "ACInput",
      "label": "Note values comma seperated",
      "value": ""
    },
    {
      "name": "maxamount",
      "type": "ACInput",
      "label": "Max withdrawable in fiat",
      "value": ""
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

void setup() {
  Serial.begin(115200);
  BTNA.begin();
  
  tft.init();
  tft.setRotation(1);
 
  pinMode(11, OUTPUT);  
  digitalWrite(11, HIGH);
  
  logo();
  int timer = 0;
  
  while (timer < 2000)
  {
    digitalWrite(2, LOW);
    Serial.println(touchRead(portalPin));
    if(touchRead(portalPin) < 60){
      Serial.println("Launch portal");
      triggerAp = true;
      timer = 5000;
    }
    digitalWrite(2, HIGH);
    timer = timer + 100;
    delay(300);
  }
  
 // h.begin();
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
    billAcceptorValues[0] = getValue(billmech, ',', 0).toInt();
    billAcceptorValues[1] = getValue(billmech, ',', 1).toInt();
    billAcceptorValues[2] = getValue(billmech, ',', 2).toInt();
    billAcceptorValues[3] = getValue(billmech, ',', 3).toInt();
    billAcceptorValues[4] = getValue(billmech, ',', 4).toInt();
    billAcceptorValues[5] = getValue(billmech, ',', 5).toInt();
    
    const JsonObject maRoot3 = doc[3];
    const char *maRoot3Char = maRoot3["value"];
    const String coinmech = maRoot2Char;
    coinAcceptorValues[0] = getValue(coinmech, ',', 0).toInt();
    coinAcceptorValues[1] = getValue(coinmech, ',', 1).toInt();
    coinAcceptorValues[2] = getValue(coinmech, ',', 2).toInt();
    coinAcceptorValues[3] = getValue(coinmech, ',', 3).toInt();
    coinAcceptorValues[4] = getValue(coinmech, ',', 4).toInt();
    coinAcceptorValues[5] = getValue(coinmech, ',', 5).toInt();

    const JsonObject maRoot4 = doc[4];
    const char *maRoot4Char = maRoot4["value"];
    const String maxamountstr = maRoot4Char;
    maxamount = maxamountstr.toInt();
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
      aux.loadElement(param, {"password", "lnurl", "coinmech", "billmech", "maxamount"});
      param.close();
    }

    if (portal.where() == "/config")
    {
      File param = FlashFS.open(PARAM_FILE, "r");
      if (param)
      {
        aux.loadElement(param, {"password", "lnurl", "coinmech", "billmech", "maxamount"});
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
      elementsAux.saveElement(param, {"password", "lnurl", "coinmech", "billmech", "maxamount"});
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
  config.reconnectInterval = 1;

  if (triggerAp == true)
  {
    digitalWrite(11, LOW);
    message("  Portal launched...", true);
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
    message("Restart/launch portal!", true);
    delay(99999999);
  }

  message("Starting bill acceptor", true);
  billAcceptor.begin(300, SERIAL_8N2, 3, 1);
  coinAcceptor.begin(4800, SERIAL_8N1, 9);
  
  billAcceptor.write(184);
  delay(1000);
  while(!billAcceptor.available())
  {
    billAcceptor.write(182);
    billAcceptor.read();
  }
}

/////////////////////////////////////
/////////////MAIN LOOP///////////////
/////////////////////////////////////

void loop(){
  unsigned long stored_time = 0;
  unsigned long current_time = 0;
  bool active = true;
  bool feedme = true;
  bool paydisplay = false;
  int coin = 0;
  int bill = 0;
  int tally = 0;
  tft.fillScreen(TFT_BLACK);
  while(active == true){
    if(feedme == true){
      feedmefiat();
    }
    if(stored_time > 1 && current_time - stored_time > 10000){
      active = false;
      paydisplay = true;
      digitalWrite(11, LOW);
    }
    while(paydisplay){
      float amount = tally / 100;
      qrShowCodeLNURL("PRESS BUTTON TO EXIT");
      while(true){}
    }
    byte byteIn = 0;
    int byteInInt = 0;
    int byteIntCoin = 0;
    byteIn = billAcceptor.read();
    byteInInt = billAcceptor.read();
    byteIntCoin = coinAcceptor.read();
    message(BTNA.read(), true);
    printbyte(byteInInt);
    current_time = millis();
    if ( byteInInt >= 1 && byteInInt <= 3){
      bill = 0;
      stored_time = millis();
      delay(1000);
      feedme = false;
      billAcceptor.write(172);
      delay(500);
      billAcceptor.write(172);
      tft.fillScreen(TFT_BLACK);
      bill = int(billAcceptorValues[byteInInt - 1]);
      tally = (tally + bill + coin);
      tft.setCursor(120, 40);
      tft.setTextColor(TFT_ORANGE);
      tft.setTextSize(10);
      tft.setCursor(140, 80);
      tft.println(String(tally) + currency);
      tft.setCursor(180, 140);
      tft.setTextColor(TFT_WHITE);
      tft.setTextSize(2);
      tft.println(String(bill) + " entered");
      current_time = millis();
    }
    if ( byteIntCoin >1){
      bill = 0;
      stored_time = millis();
      tft.fillScreen(TFT_BLACK);
      bill = float(billAcceptorValues[byteIntCoin - 1]);
      tally = (tally + bill + coin);
      tft.setCursor(120, 40);
      tft.setTextColor(TFT_ORANGE);
      tft.setTextSize(10);
      tft.setCursor(140, 80);
      tft.println(String(tally) + currency);
      tft.setCursor(180, 140);
      tft.setTextColor(TFT_WHITE);
      tft.setTextSize(2);
      tft.println(String(bill) + " entered");
      current_time = millis();
    }
  }
}

/////////////////////////////////////
///////////DISPLAY STUFF/////////////
/////////////////////////////////////

void message(String note, bool blackscreen)
{
  if(blackscreen == true){
    tft.fillScreen(TFT_BLACK);
  }
  tft.setTextSize(3);
  tft.setCursor(40, 80);
  tft.setTextColor(TFT_WHITE);
  tft.println(note);
}

void printbyte(byte byteIn){
  tft.fillRect(430, 300, 200, 200, TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(430, 300);
  tft.setTextSize(2);
  tft.println(String(byteIn));
}

void waking()
{
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(3);
  tft.setCursor(40, 80);
  tft.setTextColor(TFT_WHITE);
  tft.print("Waking up");
  delay(1000);
  tft.print("..5");
  delay(1000);
  tft.print("..4");
  delay(1000);
  tft.print("..3");
  delay(1000);
  tft.print("..2");
  delay(1000);
  tft.print("..1");
  delay(1000);
  tft.print(".");
}

void logo()
{
  tft.fillScreen(TFT_WHITE);
  tft.setCursor(130, 100);
  tft.setTextSize(10);
  tft.setTextColor(TFT_PURPLE);
  tft.println("FOSSA");
  tft.setTextColor(TFT_BLACK);
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
  tft.setCursor(120, 280);
  tft.println("(feed me fiat)");
  tft.setTextSize(10);
  tft.setCursor(160, 80);
  tft.println("SATS");
  tft.setCursor(180, 140);
  tft.println("FOR");
  tft.setCursor(160, 200);
  tft.println("FIAT!");
  delay(100);
  tft.setTextColor(TFT_GREEN);
  tft.setCursor(160, 80);
  tft.println("SATS");
  tft.setCursor(180, 140);
  tft.println("FOR");
  tft.setCursor(160, 200);
  tft.println("FIAT!");
  delay(100);
  tft.setTextColor(TFT_BLUE);
  tft.setCursor(160, 80);
  tft.println("SATS");
  tft.setCursor(180, 140);
  tft.println("FOR");
  tft.setCursor(160, 200);
  tft.println("FIAT!");
  delay(100);
  tft.setTextColor(TFT_ORANGE);
  tft.setCursor(160, 80);
  tft.println("SATS");
  tft.setCursor(180, 140);
  tft.println("FOR");
  tft.setCursor(160, 200);
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
        tft.fillRect(65 + 2 * x, 5 + 2 * y, 2, 2, TFT_BLACK);
      }
      else
      {
        tft.fillRect(65 + 2 * x, 5 + 2 * y, 2, 2, TFT_WHITE);
      }
    }
  }

  tft.setCursor(0, 220);
  tft.setTextSize(2);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.println(message);
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
  randomPin = random(1000, 9999);
  byte nonce[8];
  for (int i = 0; i < 8; i++)
  {
    nonce[i] = random(256);
  }

  byte payload[51]; // 51 bytes is max one can get with xor-encryption

    size_t payload_len = xor_encrypt(payload, sizeof(payload), (uint8_t *)secretATM.c_str(), secretATM.length(), nonce, sizeof(nonce), randomPin, dataIn.toInt());
    preparedURL = baseURLATM + "?atm=1&p=";
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
  Serial.println(qrData);
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
