//////////////LOAD LIBRARIES////////////////

#include "FS.h"
#include <WiFiManager.h> 
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include "SPIFFS.h"
#include <Arduino.h>
#include "qrcode.h"
#include <SPI.h>
#include <TFT_eSPI.h> 
#include "xbm.h" 
#define INTERUPT 21


TFT_eSPI tft = TFT_eSPI();   // Invoke library

/////////////////SOME VARIABLES///////////////////

char lnbits_server[40] = "lnbits.com";
char lnbits_port[6]  = "443";
char admin_key[200] = "acd514d03727467f9349efb2a2b9ac8e";
char currency[5] = "GBP";
char coin_values[40] = "2,5,10,20,50,100";
char charge_percent[4] = "2";
char max_amount[10] = "3";
char wifi_pass[20] = "pikachu1";
char static_ip[16] = "10.0.1.56";
char static_gw[16] = "10.0.1.1";
char static_sn[16] = "255.255.255.0";

float feefloat = atoi(charge_percent) / 100;
bool shouldSaveConfig = false;
const char* lnurl;
const char* lnurl_id;
String lnurlid;
int lnurl_used = 0;
const char* spiffcontent = "";
String spiffing; 
int coins[6];

float conversion;
/////////////////////SETUP////////////////////////

void setup() {
  Serial.begin(115200);
  Serial2.begin(4800, SERIAL_8E1, 36, 27); 
  Serial.println("Coin Acceptor Ready!");
  pinMode(INTERUPT, OUTPUT);
  
  tft.begin();               // Initialise the display
  tft.fillScreen(TFT_BLACK); // Black screen fill
  tft.setRotation(1);
 
  tft.setTextColor(TFT_WHITE);  // Set text colour to black, no background (so transparent)
  tft.setTextSize(2);

// START PORTAL 
  fossa_portal();
}

///////////////////MAIN LOOP//////////////////////
void loop() {
  // GET RATES
  on_rates();
  digitalWrite(INTERUPT, HIGH);
  tft.drawXBitmap(0, 0, insert, 320, 240, TFT_WHITE, TFT_BLACK);
  tft.setTextColor(TFT_GREEN);
  tft.setCursor(10, 115, 1);    
  tft.println(coin_values);
  tft.setCursor(10, 180, 1);
  tft.println("1BTC (-"+ String(charge_percent) + "%) = " + String(conversion * (1 - feefloat)) + " " + currency);
  tft.setCursor(10, 200, 1);
  tft.println("10000SAT (-" + String(charge_percent) + "%) = " + String(roundf(conversion * (1 - feefloat) * 100) /1000000)  + " " + currency);
  tft.setTextColor(TFT_WHITE);
  mech();
}

//////////////////COIN MECH LOOP///////////////////
void mech() {  
  int mechvalue = 0;
  int amount = 0;
  int satoshis = 0  ;
  int tempnum[6];
  unsigned long lastMillis = 0;
  for (int i = 0; i < 6; i++) {
    tempnum[i] = getValue(coin_values,',',i).toInt();
  }
  while (true) {
    if (Serial2.available()) {
      mechvalue = Serial2.read();
       if (mechvalue < 6) {
          processing();
          lastMillis = millis();
          amount = amount + tempnum[mechvalue -1];
        } 
    }
    if ((millis() - lastMillis) > 5000 && lastMillis != 0) {
      digitalWrite(INTERUPT, LOW);
      satoshis = ((amount*1000000)/conversion) * (1-feefloat);
      currentamount(amount, satoshis);
      lastMillis = 0;
      get_lnurl(satoshis);
      break;
    }
  }
}

//////////////////NODE CALLS///////////////////

  
void get_lnurl(int amount) {
  WiFiClientSecure client;
  const char* lnbitsserver = lnbits_server;
  const char* adminkey = admin_key;
  int lnbitsport = atoi( lnbits_port );
  if (!client.connect(lnbitsserver, lnbitsport)){
    error("failed to connect");
    return;   
  }
  String topost = "{  \"title\" : \"fossa\" , \"min_withdrawable\" : " + String(amount) +" , \"max_withdrawable\" : " + String(amount) +" , \"uses\" : 1, \"wait_time\" : 1, \"is_unique\" : false}";
  client.print(String("POST ")+ "https://" + lnbitsserver +":"+ lnbitsport + "/withdraw/api/v1/links HTTP/1.1\r\n" +
                 "Host: "  + lnbitsserver +":"+ lnbitsport +"\r\n" +
                 "User-Agent: ESP322\r\n" +
                 "X-Api-Key:" + adminkey + "\r\n" +
                 "Content-Type: application/json\r\n" +
                 "Connection: close\r\n"  +
                 "Content-Length: " + topost.length() + "\r\n" +
                 "\r\n" + 
                 topost + "\n");
    String line = client.readStringUntil('\n');
    while (client.connected()) {
     String line = client.readStringUntil('\n');
     if (line == "\r") {    
       break;
     }
    }
    String content = client.readStringUntil('\n');
    client.stop();
    const size_t capacity = JSON_OBJECT_SIZE(3) + 620;
    DynamicJsonDocument doc(capacity);
    deserializeJson(doc, content); 
    lnurl = doc["lnurl"];
    lnurl_id = doc["id"];
    lnurlid = lnurl_id;
    if (!lnurl){
      error(doc["message"]);
      delay(5000);
      return;   
    }
    if (!doc["message"]){
      showAddress();
      delay(1000);
      check_lnurl();
      return;   
    }
    error(doc["message"]);
    delay(5000);
    return;  
}

void check_lnurl() {
  int timeleft = 120;
  while(lnurl_used == 0){
    WiFiClientSecure client;
    const char* lnbitsserver = lnbits_server;
    const char* adminkey = admin_key;
    int lnbitsport = atoi( lnbits_port );
    if (!client.connect(lnbitsserver, lnbitsport)){
      error("failed to connect");
      return;   
    }
    client.print(String("GET ")+ "https://" + lnbitsserver +":"+ lnbitsport + "/withdraw/api/v1/links/" + lnurlid + " HTTP/1.1\r\n" +
                 "Host: "  + lnbitsserver +":"+ lnbitsport +"\r\n" +
                 "User-Agent: ESP322\r\n" +
                 "X-Api-Key:" + adminkey + "\r\n" +
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
    const size_t capacity = JSON_OBJECT_SIZE(3) + 620;
    DynamicJsonDocument doc(capacity);
    deserializeJson(doc, content); 

    tft.fillRect(0, 220, 320, 100, TFT_WHITE);  
    tft.setTextColor(TFT_BLACK);
    tft.setCursor(2, 220, 1);
    tft.print("invoice expires in: " + String(timeleft) + "secs");
    delay(5000);
    if(doc["used"]){
      lnurl_used = doc["used"];
    }
    if(lnurl_used == 1){
      tft.fillScreen(TFT_BLACK);
      tft.setTextColor(TFT_GREEN);
      tft.setCursor(80, 60, 2);
      tft.println("COMPLETE");  
      delay(5000);
    }
     if(timeleft <= 0){
      tft.fillScreen(TFT_BLACK);
      tft.setTextColor(TFT_RED);
      tft.setCursor(60, 60, 2);
      tft.println("OUT OF TIME");
      lnurl_used = 1;
      delay(5000);
    }
    timeleft = timeleft - 5;
  }
  lnurl_used == 0;
  lnurl = "";
  lnurl_id = "";
  lnurlid = "";
}

void on_rates(){
  WiFiClientSecure client;
  if (!client.connect("api.opennode.co", 443)) {
    return;
  }
  
  String url = "https://api.opennode.co/v1/rates";
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
    String full_currency = "BTC" + String(currency);
    conversion = doc["data"][full_currency][full_currency.substring(3)]; 
    delay(2000);
}

void showAddress(){
  tft.fillScreen(TFT_WHITE); // Black screen fill
  String addr = lnurl;
  int qrSize = 10;
  int sizes[17] = { 14, 26, 42, 62, 84, 106, 122, 152, 180, 213, 251, 287, 331, 362, 412, 480, 504 };
  int len = addr.length();
  for(int i=0; i<17; i++){
    if(sizes[i] > len){
      qrSize = i+3;
      break;
    }
  }
  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(qrSize)];
  qrcode_initText(&qrcode, qrcodeData, qrSize, 1, addr.c_str());
  float scale = 3;
   
  for (uint8_t y = 0; y < qrcode.size; y++) {
    for (uint8_t x = 0; x < qrcode.size; x++) {
      if(qrcode_getModule(&qrcode, x, y)){   
        tft.fillRect(70+scale*x, 2 + 17+scale*y, scale, scale, TFT_BLACK);    
      }
    }
  }
 tft.setTextColor(TFT_BLACK);
 tft.setCursor(2, 220, 1);
 tft.print("invoice expires in: 120secs");
}

////////////////////////////

void portallaunched(){
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN);
  tft.setCursor(60, 60, 2);
  tft.println("Portal Launched");   
}

void processing(){
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN);
  tft.setCursor(60, 60, 2);
  tft.println("Processing...");   
}

void error(String message){
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_RED);
  tft.setCursor(60, 60, 2);
  tft.println("Error:" + message);   
}

void currentamount(int amount, int satoshis){
  tft.drawXBitmap(0, 0, paid, 320, 240, TFT_WHITE, TFT_BLACK);
  tft.setTextColor(TFT_RED);
  tft.setCursor(110, 53, 2);
  tft.print(String(String(currency) + " " + String(float(amount) / 100)));
  tft.setTextColor(TFT_GREEN);
  tft.setCursor(110, 105, 2);
  tft.print(String(satoshis));
  tft.setCursor(180, 117, 1);
  tft.print(" (-" + String(charge_percent) + "% fee)");
  delay(5000);
}

void fossa_portal(){

  WiFiManager wm;
  Serial.println("mounting FS...");
  while(!SPIFFS.begin(true)){
    Serial.println("failed to mount FS");
    delay(200);
   }

//CHECK IF RESET IS TRIGGERED/WIPE DATA
  for (int i = 0; i <= 5; i++) {
    Serial.println(touchRead(15));
    if(touchRead(15) < 20){
    File file = SPIFFS.open("/config.txt", FILE_WRITE);
    file.print("placeholder");
    wm.resetSettings();
    }
    delay(1000);
  }

//MOUNT FS AND READ CONFIG.JSON
  File file = SPIFFS.open("/config.txt");
  
  spiffing = file.readStringUntil('\n');
  spiffcontent = spiffing.c_str();
  DynamicJsonDocument json(700);
  deserializeJson(json, spiffcontent);
  if(String(spiffcontent) != "placeholder"){
    strcpy(lnbits_server, json["lnbits_server"]);
    strcpy(lnbits_port, json["lnbits_port"]);
    strcpy(admin_key, json["admin_key"]);
    strcpy(wifi_pass, json["wifi_pass"]);
    strcpy(currency, json["currency"]);
    strcpy(coin_values, json["coin_values"]);
    strcpy(charge_percent, json["charge_percent"]);
    strcpy(max_amount, json["max_amount"]);
  }

//ADD PARAMS TO WIFIMANAGER
  wm.setSaveConfigCallback(saveConfigCallback);

  WiFiManagerParameter custom_lnbits_server("server", "LNbits server", lnbits_server, 40);
  WiFiManagerParameter custom_lnbits_port("port", "LNbits port", lnbits_port, 6);
  WiFiManagerParameter custom_admin_key("key", "LNbits admin key", admin_key, 200);
  WiFiManagerParameter custom_wifi_pass("pass", "Portal Wifi Pass", wifi_pass, 20);
  WiFiManagerParameter custom_currency("currency", "Currency (ie USD)", currency, 6);
  WiFiManagerParameter custom_coin_values("values", "Coin values", coin_values, 40);
  WiFiManagerParameter custom_charge_percent("charge", "Charge percent", charge_percent, 4);
  WiFiManagerParameter custom_max_amount("max", "Max amount", max_amount, 10);
  wm.addParameter(&custom_lnbits_server);
  wm.addParameter(&custom_lnbits_port);
  wm.addParameter(&custom_admin_key);
  wm.addParameter(&custom_wifi_pass);
  wm.addParameter(&custom_currency);
  wm.addParameter(&custom_coin_values);
  wm.addParameter(&custom_charge_percent);
  wm.addParameter(&custom_max_amount);
  
//IF RESET WAS TRIGGERED, RUN PORTAL AND WRITE FILES
  if (!wm.autoConnect("⚡ FOSSA ⚡", wifi_pass)) {
    portallaunched();
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    ESP.restart();
    delay(5000);
  }
  Serial.println("connected :)");
  strcpy(lnbits_server, custom_lnbits_server.getValue());
  strcpy(lnbits_port, custom_lnbits_port.getValue());
  strcpy(admin_key, custom_admin_key.getValue());
  strcpy(wifi_pass, custom_wifi_pass.getValue());
  strcpy(currency, custom_currency.getValue());
  strcpy(coin_values, custom_coin_values.getValue());
  strcpy(charge_percent, custom_charge_percent.getValue());
  strcpy(max_amount, custom_max_amount.getValue());
  if (shouldSaveConfig) {
    Serial.println("saving config");
    DynamicJsonDocument json(1024);
    json["lnbits_server"] = lnbits_server;
    json["lnbits_port"] = lnbits_port;
    json["admin_key"] = admin_key;
    json["wifi_pass"] = wifi_pass;
    json["currency"] = currency;
    json["coin_values"] = coin_values;
    json["charge_percent"] = charge_percent;
    json["max_amount"] = max_amount;

    File configFile = SPIFFS.open("/config.txt", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
      }
      serializeJsonPretty(json, Serial);
      serializeJson(json, configFile);
      configFile.close();
      shouldSaveConfig = false;
  }

  Serial.println("local ip");
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.gatewayIP());
  Serial.println(WiFi.subnetMask());
}

void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

String getValue(String data, char separator, int index){
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;
  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
      found++;
      strIndex[0] = strIndex[1]+1;
      strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }
  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}
