/*
  RadioLib SX1276 Receive Example using the RFM97C 915MHz

  Gotchas:
    * The RadioLib defaults the SX1276 to 434MHz so the reception is pretty
      poor. This is fixed with a radio.begin(915.0);
    * The RadioLib really requires DIO0 and DIO1 be connected. Reset is optional.
    * Avoid ESP pins 34/35/36/39 as they are inputs only
*/

#include <RadioLib.h> //Click here to get the library: http://librarymanager/All#RadioLib
#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <Wire.h>

// Provide the token generation process info
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"
//Insert your network credentials
#define WIFI_SSID "SMKIM"
#define WIFI_PASSWORD "vqvt5559"
//Insert Firebase project API Key
#define API_KEY "AIzaSyBHpZX1SyZLH1qE40hjGgjp8bewiwGAvfU"
//Insert Authroized Email and Corresponding Password
#define USER_EMAIL "suek42659@gmail.com"
#define USER_PASSWORD "Suek42659!@#"
//Insert RTDB URLefine the RTDB URL
#define DATABASE_URL "https://catrak-b3603-default-rtdb.firebaseio.com/"

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

//Timer variables (send new readings every second)
unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 1000;

// SX1276 has the following connections on the SparkFun ESP32 Thing Plus C:
//children node
int pin_cs = 15; 
int pin_dio0 = 26; //aka A0
int pin_nrst = 32;
int pin_dio1 = 25; //aka A1
SX1276 radio = new Module(pin_cs, pin_dio0, pin_nrst, pin_dio1);

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

void setup() {
  Serial.begin(115200);
  delay(200); //Wait for ESP32 to be able to print
  //WIFI&Firebase
  initWiFi();
  configTime(0, 0, ntpServer);

  //Assign the api key
  config.api_key = API_KEY;
  //Assign the user sign in credntials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
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

  //Catrak Lora function  
  Serial.print(F("[SX1276] Initializing ... "));
  //int state = radio.begin(); //-121dBm
  //int state = radio.begin(868.0); //-20dBm
  int state = radio.begin(915.0); //-23dBm
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("init success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }

}

void loop() {
  
  Serial.print(F("[SX1276] Waiting for incoming transmission ... "));
  String str;
  String id;
  id = "char";
  int state = radio.receive(str);
  Serial.println(str);
  Serial.println(id);
 

  if (state == RADIOLIB_ERR_NONE && str[0] == id[0]) {
    // packet was successfully received
    Serial.println(F("success!"));

    // print the data of the packet
    Serial.print(F("[SX1276] Data:\t\t\t"));
    Serial.println(str);

    // print the RSSI (Received Signal Strength Indicator)
    // of the last received packet
    Serial.print(F("[SX1276] RSSI:\t\t\t"));
    Serial.print(radio.getRSSI());
    Serial.println(F(" dBm"));

    // print the SNR (Signal-to-Noise Ratio)
    // of the last received packet
    Serial.print(F("[SX1276] SNR:\t\t\t"));
    Serial.print(radio.getSNR());
    Serial.println(F(" dB"));

    // print frequency error
    // of the last received packet
    Serial.print(F("[SX1276] Frequency error:\t"));
    Serial.print(radio.getFrequencyError());
    Serial.println(F(" Hz"));

  } else if (state == RADIOLIB_ERR_RX_TIMEOUT) {
    // timeout occurred while waiting for a packet
    Serial.println(F("timeout!"));

  } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
    // packet was received, but is malformed
    Serial.println(F("CRC error!"));

  } else {
    // some other error occurred
    Serial.print(F("failed, code "));
    Serial.println(state);
  }

  //WIFI&Firebase
  //send new readings to database
  if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0)){
    sendDataPrevMillis == millis();
    //Get current timestamp
    timestamp = getTime();
    Serial.print("time: ");
    Serial.println(timestamp);

    ParentPath = databasePath + "/" + String(timestamp);

    json.set(DATAPath.c_str(), String(str));
    json.set(RSSIPath.c_str(), String(radio.getRSSI()));
    json.set(SNRPath.c_str(), String(radio.getSNR()));
    json.set(FreqErrorPath.c_str(), String(radio.getFrequencyError()));
    json.set(timePath, String(timestamp));
    Serial.printf("Set json.. %s\n", Firebase.RTDB.setJSON(&fbdo, ParentPath.c_str(), &json) ? "ready" : fbdo.errorReason().c_str());

  }  

}
