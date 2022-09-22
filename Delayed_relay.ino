#include <Arduino.h>
#include <time.h> 
#include <ESP8266WiFi.h>
#include "CTBot.h"
#include "HTTPSRedirect.h"
// #include <string>
// #include <iostream>

#define WIFI_SSID "Androidi"
#define WIFI_PASSWORD "androidi"
#define NTP_SERVER "id.pool.ntp.org"
#define BOT_TOKEN "5550249778:AAFpzax3l8uGVdp9-6gh2Nl9gFERVN4AEEE"
#define GScriptId "AKfycbzRAJ_gDSeubQa9l_xqv25kB9xFUOI9_ygU332yrckBgHQSd6LnRrEJmhr47buj6uM"

// Google Sheets setup (do not edit)
#define host "script.google.com"
const int httpsPort = 443;
#define fingerprint ""
String url = String("/macros/s/") + GScriptId + "/exec";
HTTPSRedirect* client = nullptr;

CTBot robot;
String payload_base =  "{\"command\": \"insert_row\", \"sheet_name\": \"Sheet1\", \"values\": ";
String payload = "";
String from_name = "Guest";
int ledPin = 4;
int ledStatus = 0;
int endTime= 0;
int addedTime = 0;
int startTime = 0;
int statusSend = 0;

static const time_t EPOCH_2000_01_01 = 946684800;
static const unsigned long REBOOT_TIMEOUT_MILLIS = 15000;

String printToHour(time_t now) {
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);

  int hour = timeinfo.tm_hour + 7;
  int mins = timeinfo.tm_min;
  
  String hourString = String(hour);
  String minsString = String(mins);

  // Serial.printf(" %04d-%02d-%02dT%02d:%02d:%02d %s",
  //     year, month, day, hour, mins, sec, dow_string);

  return hourString + ":" + minsString;
}

void setupSntp() {
  Serial.print(F("Configuring SNTP"));
  configTime(0 /*timezone*/, 0 /*dst_sec*/, NTP_SERVER);

  unsigned long startMillis = millis();
  while (true) {
    Serial.print('.'); // Each '.' represents one attempt.
    time_t now = time(nullptr);
    if (now >= EPOCH_2000_01_01) {
      Serial.println(F(" Done."));
      break;
    }

    // Detect timeout and reboot.
    unsigned long nowMillis = millis();
    if ((unsigned long) (nowMillis - startMillis) >= REBOOT_TIMEOUT_MILLIS) {
    
    Serial.println(F(" FAILED! Rebooting..."));
    delay(1000);
    ESP.restart();
  
    Serial.print(F(" FAILED! But cannot reboot. Continuing"));
    startMillis = nowMillis;
    
    }

    delay(500);
  }
}

void setupHttps() {
  // Use HTTPSRedirect class to create a new TLS connection
  client = new HTTPSRedirect(httpsPort);
  client->setInsecure();
  client->setPrintResponseBody(true);
  client->setContentTypeHeader("application/json");
  
  Serial.print("Connecting to ");
  Serial.println(host);

  // Try to connect for a maximum of 5 times
  bool flag = false;
  for (int i=0; i<5; i++){ 
    int retval = client->connect(host, httpsPort);
    if (retval == 1){
       flag = true;
       Serial.println("Connected to Https");
       break;
    }
    else
      Serial.println("Connection failed. Retrying...");
  }
  if (!flag){
    Serial.print("Could not connect to server: ");
    Serial.println(host);
    return;
  }
  delete client;    // delete HTTPSRedirect object
  client = nullptr; // delete HTTPSRedirect object
}

void publish() {
  static bool flag = false;
  if (!flag){
    client = new HTTPSRedirect(httpsPort);
    client->setInsecure();
    flag = true;
    client->setPrintResponseBody(true);
    client->setContentTypeHeader("application/json");
  }
  if (client != nullptr){
    if (!client->connected()){
      client->connect(host, httpsPort);
      Serial.println("Succes creating client object!");
    }
  }
  else{
    Serial.println("Error creating client object!");
  }
  
  // Create json object string to send to Google Sheets
  payload = payload_base + "\"" + from_name + "," + printToHour(startTime) + "," + printToHour(endTime) + "\"}";
  
  // Publish data to Google Sheets
  Serial.println("Publishing data...");
  Serial.println(payload);
  if(client->POST(url, host, payload)){ 
    Serial.println("Sucess publishing");
  }
  else{
    Serial.println("Error while connecting");
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(ledPin,OUTPUT);
  digitalWrite(ledPin,HIGH);
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  robot.wifiConnect(WIFI_SSID,WIFI_PASSWORD);
  robot.setTelegramToken(BOT_TOKEN);
  if(robot.testConnection()){
    Serial.println("connect to CTbot");}
   else {
    Serial.println("not connected");
  }
  
  setupSntp();
  setupHttps();
  Serial.print("ready");
}

void loop() {
  TBMessage pesan;
  
  time_t now = time(nullptr);
  Serial.println(printToHour(now));

  if (now > endTime)
  {
    digitalWrite(ledPin, HIGH);
    ledStatus = 0;
  }
  
  
  if(robot.getNewMessage(pesan)){
    Serial.print("pesan ");
    Serial.println(pesan.text);
    from_name = pesan.sender.firstName;
    if(pesan.text.equalsIgnoreCase("/On")){
      String respon = "Berikut waktu untuk menyalakan relay, " + from_name + ".\n";
      respon += "/1_jam\n";
      respon += "/2_jam\n";
      respon += "/3_jam\n";
      respon += "/4_jam\n";
      respon += "/5_jam\n";
      robot.sendMessage(pesan.sender.id, respon );
      delay(3000);
      if (robot.getNewMessage(pesan))
      Serial.print("pesan ");
      Serial.println(pesan.text);
      {
        if (pesan.text.equalsIgnoreCase("/1_jam"))
        {
          ledStatus = 1;
          addedTime = 1 * 3600;
          startTime = now;
          endTime = startTime + addedTime;
          digitalWrite(ledPin, LOW); 
          robot.sendMessage(pesan.sender.id,"Relay akan menyala 1 jam");
          publish();
        }
        else if (pesan.text.equalsIgnoreCase("/2_jam"))
        {
          ledStatus = 1;
          addedTime = 2 * 3600;
          startTime = now;
          endTime = startTime + addedTime;
          digitalWrite(ledPin, LOW); 
          robot.sendMessage(pesan.sender.id,"Relay akan menyala 2 jam");
          publish();
        }
        else if (pesan.text.equalsIgnoreCase("/3_jam"))
        {
          ledStatus = 1;
          addedTime = 3 * 3600;
          startTime = now;
          endTime = startTime + addedTime;
          digitalWrite(ledPin, LOW); 
          robot.sendMessage(pesan.sender.id,"Relay akan menyala 3 jam");
          publish();
        }
        else if (pesan.text.equalsIgnoreCase("/4_jam"))
        {
          ledStatus = 1;
          addedTime = 4 * 3600;
          startTime = now;
          endTime = startTime + addedTime;
          digitalWrite(ledPin, LOW); 
          robot.sendMessage(pesan.sender.id,"Relay akan menyala 4 jam");
          publish();
        }
        else if (pesan.text.equalsIgnoreCase("/5_jam"))
        {
          ledStatus = 1;
          addedTime = 5 * 3600;
          startTime = now;
          endTime = startTime + addedTime;
          digitalWrite(ledPin, LOW); 
          robot.sendMessage(pesan.sender.id,"Relay akan menyala 5 jam");
          publish();
        }
        else
        {
          robot.sendMessage(pesan.sender.id,"Timeot silahkan beri perintah /On kembali");
        }
      }
    } else if(pesan.text.equalsIgnoreCase("/Stop")){
      ledStatus = 0;
      digitalWrite(ledPin, HIGH); 
      robot.sendMessage(pesan.sender.id,"Stop");;
    } else if(pesan.text.equalsIgnoreCase("/Status")){
      if (ledStatus)
      {
        robot.sendMessage(pesan.sender.id,"Listrik Menyala");;
      }
      else
      {
        robot.sendMessage(pesan.sender.id,"Listrik Mati");;
      }
    }
    else{
      String welcome = "Selamat Datang di Delayed Relay, " + from_name + ".\n";
      welcome += "Ini adalah perintah prototype Saklar cerdas .\n\n";
      welcome += "/On : to switch the Led ON\n";
      welcome += "/Stop : to switch the Led OFF\n";
      welcome += "/status : Returns current status of LED\n";
      robot.sendMessage(pesan.sender.id, welcome );
    }
  }
  delay(1000);
}
