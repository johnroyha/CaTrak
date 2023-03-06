/*

*/
#include <Wire.h> //Needed for I2C to GNSS
#include <RadioLib.h> //Need for LoRa 
#include <u-blox_GNSS_Library.h> //Needed for u-blox GPS
SFE_UBLOX_GNSS myGNSS;

//Pin Set Up
#define LED_PIN PB15
#define USER_BTN PB2
#define ISM_BAND 915.0
//Radio Setup
// no need to configure pins, signals are routed to the radio internally
STM32WLx radio = new STM32WLx_Module();

// set RF switch configuration for Nucleo WL55JC1
// NOTE: other boards may be different!
static const RADIOLIB_PIN_TYPE rfswitch_pins[] =
                         {PC3,  PC4,  PC5};
static const Module::RfSwitchMode_t rfswitch_table[] = {
  {STM32WLx::MODE_IDLE,  {LOW,  LOW,  LOW}},
  {STM32WLx::MODE_RX,    {HIGH, HIGH, LOW}},
  {STM32WLx::MODE_TX_LP, {HIGH, HIGH, HIGH}},
  {STM32WLx::MODE_TX_HP, {HIGH, LOW,  HIGH}},
  STM32WLx::END_OF_MODE_TABLE,
};
/////////////////
int buttonState = 0;
bool hasBeenPressed = false; // flag to keep track of whether the button has been pressed

float* get_position() {
  static float position[2]; // array to hold latitude and longitude values
  position[0] = myGNSS.getLatitude() / 10000000.0; // convert latitude to degrees
  position[1] = myGNSS.getLongitude() / 10000000.0; // convert longitude to degrees

  return position; // return the position array
}

void transmitPacket(String catData){
  Serial.print(F("[STM32WL] Transmitting packet ... "));
  // you can transmit C-string or Arduino string up to 256 characters long
  // NOTE: transmit() is a blocking method!
  //       See example STM32WLx_Transmit_Interrupt for details
  //       on non-blocking transmission method.
  //int state = radio.transmit(coordinates);
  int state = radio.transmit(catData);
  // you can also transmit byte array up to 256 bytes long
  /*
    byte byteArr[] = {0x01, 0x23, 0x45, 0x56, 0x78, 0xAB, 0xCD, 0xEF};
    int state = radio.transmit(byteArr, 8);
  */
  if (state == RADIOLIB_ERR_NONE) {
    // the packet was successfully transmitted
    Serial.println(F("success!"));
    // print measured data rate
    Serial.print(F("[STM32WL] Datarate:\t"));
    Serial.print(radio.getDataRate());
    Serial.println(F(" bps"));

  } else if (state == RADIOLIB_ERR_PACKET_TOO_LONG) {
    // the supplied packet was longer than 256 bytes
    Serial.println(F("too long!"));

  } else if (state == RADIOLIB_ERR_TX_TIMEOUT) {
    // timeout occured while transmitting packet
    Serial.println(F("timeout!"));

  } else {
    // some other error occurred
    Serial.print(F("failed, code "));
    Serial.println(state);
  }
}

String getCoordString(float* position) {
  String coordinates = String(position[0], 7);
  coordinates += ", ";
  coordinates += String(position[1], 7);
  return  "c" + coordinates;
}


// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_PIN, OUTPUT);
  pinMode(USER_BTN, INPUT);
  //Initilization of GPS
  Wire.setSDA(PB9);
  Wire.setSCL(PB8);
  Wire.begin();
  myGNSS.begin();
  myGNSS.setI2COutput(COM_TYPE_UBX); //Set the I2C port to output UBX only (turn off NMEA noise)
  myGNSS.saveConfigSelective(VAL_CFG_SUBSEC_IOPORT); //Save (only) the communications port settings to flash and BBR
  //
  //LoRa Init
  // set RF switch control configuration
  // this has to be done prior to calling begin()
  radio.setRfSwitchTable(rfswitch_pins, rfswitch_table);
  radio.setOutputPower(22);

  // initialize STM32WL with default settings, except frequency
  Serial.print(F("[STM32WL] Initializing ... "));
  int state = radio.begin(ISM_BAND);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }

  // set appropriate TCXO voltage for Cicerone
  state = radio.setTCXO(1.7);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("setTCXO: success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }
//
  Serial.begin(9600);
}

// the loop function runs over and over again forever
void loop() {
  buttonState = digitalRead(USER_BTN); // read the state of the button
  if (buttonState == HIGH && !hasBeenPressed) { // if button is pressed and not already pressed
    digitalWrite(LED_PIN, HIGH); // turn on LED
    //Serial.println("Hello, World!"); // print message to serial monitor
    float* position = get_position();
    //Serial.print("Latitude: ");
    //Serial.println(position[0],7);
    //Serial.print("Longitude: ");
    //Serial.println(position[1], 7);
    String coordinates = getCoordString(position);
    transmitPacket(coordinates);
    Serial.println(coordinates);
 
      
    hasBeenPressed = true; // set flag to true
  } else if (buttonState == LOW) { // if button is not pressed
    digitalWrite(LED_PIN, LOW); // turn off LED
    hasBeenPressed = false; // reset flag
  }
}
