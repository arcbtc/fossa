/**
 *  The Fossa uses Zap desktop wallet tunnelled through serveo.net "ssh -R SOME-RANDOM-WORD.serveo.net:3010:localhost:8180 serveo.net"
 *  
 *  Wiring - ESP32 NodeMCU 32s + ST7735 TFT 1.8
 *
 *  Epaper PIN MAP: [VCC - 3.3V, GND - GND, CS - GPIO5, Reset - GPIO16, AO (DC) - GPI17, SDA (MOSI) - GPIO23, SCK - GPIO18, LED - GPIO4]
 *       
 *
 */

#include <WiFiClientSecure.h>
#include <ArduinoJson.h> 
#include "qrcode.h"
#include <SPI.h>
#include "Ucglib.h"

Ucglib_ST7735_18x128x160_HWSPI ucg(/*cd=*/ 17, /*cs=*/ 5, /*reset=*/ 16);

String giftinvoice;
String giftid;
String giftlnurl;
String amount;

bool spent = false;
String giftstatus = "unpaid";
unsigned long tickertime;
unsigned long tickertimeupdate;
unsigned long giftbreak = 600000;  
unsigned long currenttime = 600000;

//enter your wifi details
char wifiSSID[] = "YOUR-WIFI";
char wifiPASS[] = "YOUR-WIFI-PASSWORD";

const char* gifthost = "api.lightning.gifts";
const int httpsPort = 443;

const char* lndhost = "SOME-RANDOM-WORD.serveo.net"; //if using serveo.net replace the *SOME-RANDOM-WORD* bit
String adminmacaroon = "YOUR-LND-ADMIN-MACAROON"; //obv, this is dodge, so use an instance of LND with limited funds, I use desktop Zap wallet and make it available through serveo.net 
const int lndport = 3010;

String on_currency = "BTCEUR";  //currency can be changed here ie BTCEUR BTCGBP etc
String on_sub_currency = on_currency.substring(3);


// Constants
const int coinpin = 15;
int totes;
int counta;
unsigned long prevmils = 0;
unsigned long tempmils = 0;
float conversion;

bool checker = false;


void setup() {
 
  delay(1000);
  ucg.begin(UCG_FONT_MODE_TRANSPARENT);
  ucg.setFont(ucg_font_ncenR14_hr);
  ucg.clearScreen();
  
  Serial.begin(115200);
         
  WiFi.begin(wifiSSID, wifiPASS);   
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("connecting");
    delay(2000);
  }

  nodecheck();
  ucg.setFont(ucg_font_helvB10_hr);
  ucg.setPrintPos(0,20);

  attachInterrupt(digitalPinToInterrupt(coinpin), coinInterrupt, RISING);

  on_rates();
  
}

void loop() {
  if (millis() - prevmils > 3000 && counta == 1){
   Serial.println(totes);
   
   counta = 2;
  }
  if (counta == 2){
  float satoshis = (totes/100)/conversion;
   int intsats = (int) round(satoshis*100000000.0);
   amount = intsats;
   Serial.println(intsats);
   create_gift();
   Serial.println(giftinvoice); 
   makepayment();
   checkgift();
   while(giftstatus == "unpaid"){
    checkgiftstatus();
    delay(500);
  }
   showAddress(giftlnurl);
   while(!spent){
    checkgift();
    delay(500);
  }
   
   counta = 0;
   spent = false;
  }
}

// Interrupt
void coinInterrupt(){
  prevmils = millis();
  totes = totes + 10;
  counta = 1;

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
  WiFiClientSecure client;
  if (!client.connect(lndhost, lndport)){
    return;
  }
  
  client.print(String("GET ")+ "https://" + lndhost +":"+ String(lndport) +"/v1/getinfo HTTP/1.1\r\n" +
               "Host: "  + lndhost +":"+ String(lndport) +"\r\n" +
               "User-Agent: ESP32\r\n" +
               "Content-Type: application/json\r\n" +
               "Connection: close\r\n\r\n");
                 
  String line = client.readStringUntil('\n');
    
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      break;
    }
  } 
  String content = client.readStringUntil('\n');
  client.stop();
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
  ucg.setPrintPos(10,10);
  float scale = 2;
  int padding = 20;
  ucg.setColor(255, 255, 255);
  ucg.drawBox(0, 0, 200, 200);
  ucg.setColor(0, 0, 0);
  for (uint8_t y = 0; y < qrcode.size; y++) {
    for (uint8_t x = 0; x < qrcode.size; x++) {
      if(qrcode_getModule(&qrcode, x, y)){       
        ucg.drawBox(padding+scale*x, 10 + padding+scale*y, scale, scale);
      }
    }
  }
}
