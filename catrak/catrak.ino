#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <Wire.h>
#include <SPI.h>
#include <LoRa.h> 

// Provide the token generation process info
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"
//Insert your network credentials
#define WIFI_SSID "CIK1000M_2.4G_2996"
#define WIFI_PASSWORD "9813334a4b14"
//Insert Firebase project API Key
#define API_KEY "AIzaSyBHpZX1SyZLH1qE40hjGgjp8bewiwGAvfU" //Sue
//#define API_KEY "AIzaSyBBiJFOwm34DcXaJQfpEZyw_jI1NQMwBgU" //Zayn

//Insert Authroized Email and Corresponding Password
#define USER_EMAIL "suek42659@gmail.com" //Sue
//#define USER_EMAIL "zayn@accesscomm.ca"
#define USER_PASSWORD "Suek42659!@#" //Sue
//#define USER_PASSWORD "CaTrak"
//Insert RTDB URLefine the RTDB URL
#define DATABASE_URL "https://catrak-b3603-default-rtdb.firebaseio.com/" //Sue
//#define DATABASE_URL "https://catrak-42f82-default-rtdb.firebaseio.com/" //Zayn

// Define Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
//save user id
String uid;
//database main path
String databasePath;
//child node
String DATAPath = "/data";
String RSSIPath = "/rssi";
String SNRPath = "/snr";
String FreqErrorPath = "/freqerror";
String timePath = "/timestamp";
//parent node
String ParentPath;
//Firebase Json file
FirebaseJson json;
//timestamp variable
int timestamp;
const char* ntpServer = "pool.ntp.org";

// Initialize WiFi
void initWiFi(){
  WiFi.begin(WIFI_SSID,WIFI_PASSWORD);
  Serial.print("Connecting to Wifi --");
  while(WiFi.status() != WL_CONNECTED){
    Serial.print(WiFi.status());
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  Serial.println();
}

// Function that gets current epoch time
unsigned long getTime(){
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
  //Serial.println("Failed to obtain time");
  return(0);
  }
  time(&now);
  return now;  
}
//Initialize LoRa packet variables
String receivedData = "";
String RSSI = "";
String SNR = "";
String PFE = "";
bool receivedPacket = false;

void setup() {
  Serial.begin(9600);
  delay(200); //Wait for ESP32 to be able to print
  
  //Initialize LoRa Receiver
  Serial.println("LoRa Receiver");
  LoRa.setPins(15, 32, 26);
  if (!LoRa.begin(915E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  //WIFI&Firebase
  initWiFi();
  configTime(0, 0, ntpServer);

  //Assign the api key
  config.api_key = API_KEY;
  //Assign the user sign in credntials
  auth.user.email = USER_EMAIL;        //Sue
  auth.user.password = USER_PASSWORD;  //Sue
  //Assign the RTDB URL (required)
  config.database_url = DATABASE_URL;
  
  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);
  
  //Assign the callback function for the long running token generation task
  config.token_status_callback = tokenStatusCallback;

  //Asign the maximum retry of token generation
  config.max_token_generation_retry = 5;

  //Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &auth);

  //getting the user UID
  Serial.println("Getting User UID");
  while ((auth.token.uid) == ""){
    Serial.print('.');
    delay(1000);  
  }

  //print UID
  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.println(uid);

  //update database path
  databasePath = "/UsersData/" + uid + "/readings";

 
}

void loop() {

  // try to parse packet
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // received a packet
    // read packet
    while (LoRa.available()) {
      receivedData += (char)LoRa.read();
    }
    //Print Packet and values
    Serial.println("[RFM97] Received: ");
    Serial.println(receivedData);
    Serial.print("RSSI: ");
    Serial.print(LoRa.packetRssi());
    Serial.print(",");
    Serial.print(" SNR: ");
    Serial.println(LoRa.packetSnr());
    //Store Packet Parameters
    RSSI = String(LoRa.packetRssi());
    SNR = String(LoRa.packetSnr());
    PFE = String(LoRa.packetFrequencyError());

    receivedPacket = true;
    
  }
 
    
  //WIFI&Firebase
  //Send new readings to database
  //if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0)){
  if (Firebase.ready() && receivedPacket && receivedData.charAt(0) == 'c'){
    
    //Get current timestamp
    timestamp = getTime();
    Serial.print("Time: ");
    Serial.println(timestamp);

    ParentPath = databasePath + "/" + String(timestamp);
    json.set(DATAPath.c_str(), receivedData);
    json.set(RSSIPath.c_str(), RSSI);
    json.set(SNRPath.c_str(), SNR);
    json.set(FreqErrorPath.c_str(), PFE);
    json.set(timePath, String(timestamp));
    Serial.printf("Set json.. %s\n", Firebase.RTDB.setJSON(&fbdo, ParentPath.c_str(), &json) ? "ready" : fbdo.errorReason().c_str());

    //Reset Data
    receivedData = "";
    RSSI = "";
    SNR = "";
    PFE = "";
    receivedPacket = false;

    Serial.println("Sent Packet To Webserver");
    Serial.println("----------------------------------------");
    
  

  }  

}