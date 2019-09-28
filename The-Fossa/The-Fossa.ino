/**
 *  The FOSSA uses Zap desktop wallet tunnelled through serveo.net "ssh -R SOME-NAME.serveo.net:3010:localhost:8180 serveo.net"
 *  Due to using an admin macaroon, an LND instance with limited funds is preferable, Zap desktop wallet is ideal
 *  Wiring - ESP32 NodeMCU 32s + ST7735 TFT 1.8
 *
 *  3.5inch TFT PIN MAP: [VCC - 5V, GND - GND, CS - GPIO5, Reset - GPIO16, AO (DC) - GPI17, SDA (MOSI) - GPIO23, SCK - GPIO18, LED - 3.3v]
 */

#include "xbm.h"             // Sketch tab header for xbm images

#include <TFT_eSPI.h>        // Hardware-specific library

#include <WiFiClientSecure.h>

#include <ArduinoJson.h>

#include "qrcode.h"

#include <SPI.h>

#include <math.h>


TFT_eSPI tft = TFT_eSPI();   // Invoke library

String giftinvoice;
String giftid;
String giftlnurl;
bool spent = false;
String giftstatus = "unpaid";
unsigned long tickertime;
unsigned long tickertimeupdate;
unsigned long giftbreak = 600000;  
unsigned long currenttime = 600000;
String amount;
int satoshis;
String timeleft;
int countdown;
int fee = 2; //percentage fee your going to take
float feefloat = fee/100;

float conversion;
String on_currency = "BTCEUR";
int httpsPort = 443;
const char* gifthost = "api.lightning.gifts";

const char* lndhost = "SOME-RANDOM-WORD.serveo.net"; //if using serveo.net replace the *SOME-RANDOM-WORD* bit
String adminmacaroon = "YOUR-ADMIN-MACAROON"; //obv, this is dodge, so use an instance of LND with limited funds, I use desktop Zap wallet and make it available through serveo.net (https://docs.zaphq.io/docs-desktop-neutrino-connect is a useful doc) 
const int lndport = 3010;

String accepts = "10 20 50 1 2";

//Wifi details
char wifiSSID[] = "YOUR-WIFI";
char wifiPASS[] = "WIFI-PASS";

// Constants
const int coinpin = 4;
float totes;
int counta;
unsigned long prevmils = 0;
unsigned long tempmils = 0;


void setup()
{
  pinMode(coinpin, INPUT_PULLUP);
  Serial.begin(115200);
  
  tft.begin();               // Initialise the display
  tft.fillScreen(TFT_BLACK); // Black screen fill
  tft.setRotation(3);
 
  tft.setTextColor(TFT_WHITE);  // Set text colour to black, no background (so transparent)
  tft.setTextSize(2); 
  //connect to local wifi            
  WiFi.begin(wifiSSID, wifiPASS);   
  while (WiFi.status() != WL_CONNECTED) {
    wificheck();
    delay(2000);
  }

  attachInterrupt(digitalPinToInterrupt(coinpin), coinInterrupt, RISING);
  pinMode(21, OUTPUT); 
  digitalWrite(21, LOW); 
  on_rates();

}


void loop() {
  nodecheck();
  splash();

  if (millis() - prevmils > 3000 && counta == 1){
    detachInterrupt(digitalPinToInterrupt(coinpin));
    digitalWrite(21, HIGH); 
     tft.fillScreen(TFT_BLACK);
   Serial.println(totes);
   
   counta = 2;
  }
  if (counta == 2){
   
   
   satoshis = ((totes*100000000)/conversion) * (1-feefloat);
    
   currentamount();

   Serial.println(satoshis);
   amount = satoshis;
   create_gift();
   Serial.println(giftinvoice); 
   makepayment();
   prevmils = millis();
   checkgift();
   
   while(giftstatus == "unpaid"){
    
    checkgiftstatus();
    delay(2000);
    
  }
   showAddress(giftlnurl);
   Serial.println(giftlnurl);
   
   tft.setTextColor(TFT_WHITE);
   tft.setCursor(2, 220, 1);
   tft.print("invoice expires in:120secs");
   
   while(spent == false){
    
    
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(230, 220, 1);
    tft.print(timeleft + "secs");
    
    tft.setTextColor(TFT_BLACK);
    tft.setCursor(2, 220, 1);
    countdown = int(120 - ((millis() - prevmils)/1000));
    
    timeleft = countdown;
    tft.print("invoice expires in:");
    tft.setCursor(230, 220, 1);
    tft.print(timeleft + "secs");
    
    checkgift();
    delay(2000);
    if (countdown <= 3){
      spent = true;
      Serial.println(spent);
    }
  }
   digitalWrite(21, LOW); 
   counta = 0;
   spent = false;
   totes = 0;
   delay(1000);
   attachInterrupt(digitalPinToInterrupt(coinpin), coinInterrupt, RISING);
  }
}

// Interrupt
void coinInterrupt(){
  prevmils = millis();
  totes = totes + 0.1;
  counta = 1;
  
}

void currentamount(){
  tft.drawXBitmap(0, 0, paid, 320, 240, TFT_WHITE, TFT_BLACK);
    tft.setTextColor(TFT_RED);
    tft.setCursor(110, 53, 2);
    tft.print(String(totes));
    
    tft.setTextColor(TFT_GREEN);
    tft.setCursor(110, 105, 2);
    tft.print(String(satoshis));
    tft.setCursor(180, 117, 1);
    tft.print(" (-" + String(fee) + "% fee)");
}

void create_gift(){

  WiFiClientSecure client;
  
  if (!client.connect(gifthost, httpsPort)) {
    return;
  }
  
  String topost = "{  \"amount\" : " + amount +"}";
  String url = "/create";
  client.print(String("POST ") + url + " HTTP/1.1\r\n" +
                "Host: " + gifthost + "\r\n" +
                "User-Agent: ESP32\r\n" +
                "Content-Type: application/json\r\n" +
                "Connection: close\r\n" +
                "Content-Length: " + topost.length() + "\r\n" +
                "\r\n" + 
                topost + "\n");

  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      break;
    }
  }
  
  String line = client.readStringUntil('\n');
  const size_t capacity = JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(6) + 620;
  DynamicJsonDocument doc(capacity);
  deserializeJson(doc, line);
  const char* order_id = doc["orderId"]; 
  const char* statuss = doc["status"]; 
  const char* lightning_invoice_payreq = doc["lightningInvoice"]["payreq"]; 
  const char* lnurl = doc["lnurl"];
  giftinvoice = lightning_invoice_payreq;
  giftstatus = statuss;
  giftid = order_id;
  giftlnurl = lnurl;
}

void nodecheck(){
  bool checker = false;
  while(!checker){
  WiFiClientSecure client;
  
  if (!client.connect(lndhost, lndport)){
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_RED);
    tft.setCursor(60, 60, 2);
    tft.println("no node detected"); 
    delay(1000);
  }
  else{
    checker = true;
  }
  }
}


void wificheck(){
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_RED);
    tft.setCursor(60, 60, 2);
    tft.println("Connecting to wifi");   
}

void makepayment(){
  String memo = "Memo-";
  WiFiClientSecure client;
  if (!client.connect(lndhost, lndport)){
    return;
  }
  String topost = "{\"payment_request\": \""+ giftinvoice +"\"}";
  client.print(String("POST ")+ "https://" + lndhost +":"+ String(lndport) +"/v1/channels/transactions HTTP/1.1\r\n" +
               "Host: "  + lndhost +":"+ String(lndport) +"\r\n" +
               "User-Agent: ESP322\r\n" +
               "Grpc-Metadata-macaroon:" + adminmacaroon + "\r\n" +
               "Content-Type: application/json\r\n" +
               "Connection: close\r\n" +
               "Content-Length: " + topost.length() + "\r\n" +
               "\r\n" + 
               topost + "\n");

  String line = client.readStringUntil('\n');
  Serial.println(line);
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {  
      break;
    }
  }  
  String content = client.readStringUntil('\n');
  client.stop(); 
}


void checkgiftstatus(){
  WiFiClientSecure client;
  if (!client.connect(gifthost, httpsPort)) {
    return;
  }
  String url = "/status/" + giftid;
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + gifthost + "\r\n" +
               "User-Agent: ESP32\r\n" +
               "Content-Type: application/json\r\n" +
               "Connection: close\r\n\r\n");

  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      break;
    }
  }
  String line = client.readStringUntil('\n');
  const size_t capacity = JSON_OBJECT_SIZE(1) + 40;
  DynamicJsonDocument doc(capacity);
  deserializeJson(doc, line);
  const char* giftstatuss = doc["status"]; 
  giftstatus = giftstatuss;
  
}

void checkgift(){
  WiFiClientSecure client;
  if (!client.connect(gifthost, httpsPort)) {
    return;
  }
  String url = "/gift/" + giftid;
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + gifthost + "\r\n" +
               "User-Agent: ESP32\r\n" +
               "Content-Type: application/json\r\n" +
               "Connection: close\r\n\r\n");

  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      break;
    }
  }
  String line = client.readStringUntil('\n');
  Serial.println(line);
  const size_t capacity = 3*JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(9) + 1260;
  DynamicJsonDocument doc(capacity);  
  deserializeJson(doc, line);
  spent = doc["spent"]; 
}

void on_rates(){
  WiFiClientSecure client;
  if (!client.connect("api.opennode.co", 443)) {
    return;
  }
  String url = "/v1/rates";
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: api.opennode.co\r\n" +
               "User-Agent: ESP32\r\n" +
               "Connection: close\r\n\r\n");
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {

      break;
    }
  }
  String line = client.readStringUntil('\n');
    const size_t capacity = 169*JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(168) + 3800;
    DynamicJsonDocument doc(capacity);
    deserializeJson(doc, line);
    conversion = doc["data"][on_currency][on_currency.substring(3)]; 

}

void showAddress(String XXX){
  tft.fillScreen(TFT_WHITE); // Black screen fill
  String addr = XXX;
  int qrSize = 10;
  int sizes[17] = { 14, 26, 42, 62, 84, 106, 122, 152, 180, 213, 251, 287, 331, 362, 412, 480, 504 };
  int len = addr.length();
  for(int i=0; i<17; i++){
    if(sizes[i] > len){
      qrSize = i+1;
      break;
    }
  }
  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(qrSize)];
  qrcode_initText(&qrcode, qrcodeData, qrSize, 1, addr.c_str());
//  ucg.setPrintPos(10,10);
  float scale = 4;
   
  for (uint8_t y = 0; y < qrcode.size; y++) {
    for (uint8_t x = 0; x < qrcode.size; x++) {
      if(qrcode_getModule(&qrcode, x, y)){   
        tft.fillRect(70+scale*x, 2 + 17+scale*y, scale, scale, TFT_BLACK);    
      }
    }
  }
 tft.setTextColor(TFT_BLACK);
 tft.setCursor(2, 220, 1);
 tft.print("invoice expires in:120secs");
}

void splash(){
  
  if(counta == 1){
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_GREEN);
    tft.setCursor(80, 60, 2);
    tft.println("PROCESSING"); 
  }
   else{
  tft.drawXBitmap(0, 0, sats, 320, 90, TFT_WHITE, TFT_BLACK);
  for (int i=0;i<=2;i++){
   
  tft.drawXBitmap(0, 90, bitcoin, 320, 150, TFT_YELLOW, TFT_BLACK);
  tft.drawXBitmap(0, 90, bitcoin, 320, 150, TFT_ORANGE, TFT_BLACK);
  }
  }
  if(counta == 1){
     tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_GREEN);
    tft.setCursor(80, 60, 2);
    tft.println("PROCESSING");
  }
   else{
    for (int i=0;i<=2;i++){
   
  tft.drawXBitmap(0, 90, lightning, 320, 150, TFT_YELLOW, TFT_BLACK);
  tft.drawXBitmap(0, 90, lightning, 320, 150, TFT_ORANGE, TFT_BLACK);
    }
  }
  if(counta == 1){
     tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_GREEN);
    tft.setCursor(80, 60, 2);
    tft.println("PROCESSING");
  }
  else{
  tft.drawXBitmap(0, 0, insert, 320, 240, TFT_WHITE, TFT_BLACK);
  tft.setTextColor(TFT_GREEN);
  tft.setCursor(130, 90, 1);    
  tft.println(accepts);
  tft.setCursor(10, 180, 1);
  tft.println("1 BTC (-"+ String(fee) + "%) = " + String(conversion * (1-feefloat)) + " " + on_currency.substring(3));
  tft.setCursor(10, 200, 1);
  tft.println("1000 SAT (-" + String(fee) + "%) = " + String((conversion / 100000) * (1-feefloat)) + " " + on_currency.substring(3));
  delay(2000);
  tft.setTextColor(TFT_WHITE);
  }
}
