
// FOSSA NO ACCESS POINT, SIMPLE

//============================//
//============EDIT============//
//============================//

#define BTN1 39 //Screen tap button

#define RX1 32 //Bill acceptor
#define TX1 33 //Bill acceptor

#define TX2 4 //Coinmech
#define INHIBITMECH 2 //Coinmech


// LNURLDevices ATM details
String baseURLATM = "https://legend.lnbits.com/lnurldevice/api/v1/lnurl/M8wiiij8oLEgK6RNXLvoFs";
String secretATM = "3czaYALeuAp4H36tH3nasH";
String currencyATM = "USD";

// Coin and Bill Acceptor amounts
int billAmountInt[3] = {5,10,20};
float coinAmountFloat[6] = {0.02, 0.05, 0.1, 0.2, 0.5, 1};
int charge = 10; // % you will charge people for service, set in LNbits extension
int maxamount = 30; // max amount per withdraw

//============================//
//============================//
//============================//

#include <Hash.h>
#include "qrcoded.h"
#include "Bitcoin.h"

String qrData;

int bills;
float coins;
float total;

int billAmountSize = sizeof(billAmountInt) / sizeof(int);
float coinAmountSize = sizeof(coinAmountFloat) / sizeof(float);

int moneyTimer = 0;
    
#include <Wire.h>
#include <TFT_eSPI.h>
#include <HardwareSerial.h>
#include <JC_Button.h>

HardwareSerial SerialPort1(1);
HardwareSerial SerialPort2(2);

TFT_eSPI tft = TFT_eSPI();
Button BTNA(BTN1);

void setup()  
{  
  BTNA.begin();
  
  tft.init();
  tft.setRotation(1);
  tft.invertDisplay(false);
  
  SerialPort1.begin(300, SERIAL_8N2, TX1, RX1);
  SerialPort2.begin(4800, SERIAL_8N1, TX2);

  pinMode(INHIBITMECH, OUTPUT); 
}

void loop()
{
  // Turn on machines
  SerialPort1.write(184);
  digitalWrite(INHIBITMECH, HIGH);
  
  tft.fillScreen(TFT_BLACK);
  printMessage("Feed me FIAT", String(charge) + "% charge", "", TFT_WHITE, TFT_BLACK);
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
  printMessage("Feed me fiat", "", "", TFT_WHITE, TFT_BLACK);
  while( waitForTap || total == 0){
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
