/** 
 *MESHTASTIC DECODER  
 * 
 * decide at compile time if it's a SENSOR or DECODER
 * 
 * 
 * to decide it's a SENSOR or a DECODER define it at line 20
 * 
 * 
 */

//only sensor
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <esp_sleep.h>
//-------------

//every compilation:
#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include "functions.h"
//-------------

// see https://medium.com/@androidcrypto/esp32-how-to-generate-true-random-numbers-true-rng-951fcda58e9b
#include "bootloader_random.h" // for true random RNG

//define use_case, SENSOR or DECODER:
#define SENSOR

//define the pins used by the LORA antenna + board led
#define LORA_SCK   5
#define LORA_MISO  19
#define LORA_MOSI  27
#define LORA_SS    18
#define LORA_RST   23
#define LORA_DIO0  26

#define BOARD_LED 25



//make code, not war





/*
setup for the SENSOR compilation, it requires a dht22 sensor
it should be an eso32 and support esp light sleep
*/

#ifdef SENSOR

#define DHTPIN 21      // DAT pin connects to D2 on ESP32
#define DHTTYPE DHT22  // Change to DHT11 if using that sensor

DHT_Unified dht(DHTPIN, DHTTYPE);

#endif 






int counter = 0;


void sprint(const std::string value) {
  Serial.print(value.c_str());
}

void sprintln(const std::string value) {
  Serial.println(value.c_str());
}


/**
 *send a packet
 *
 *@param std::string packet 
 */
void radioTx(std::string packet) { 
  std::vector<uint8_t> pk;
  sprintln("Sending packet: ");
  sprintln(packet.c_str());
  hexStringToByte(packet,pk);

  // send packet
  LoRa.beginPacket();
  LoRa.write(pk.data(), pk.size());
  LoRa.endPacket();
}

/**
 * If the packet has bit "want acknowledge", remove it and retransmit packet
 */
void acknowledgePacket(std::vector<uint8_t> v){

  if(retrieveBitVector(v,12,3)==1){
    removeWantAcknowledge(v);
    radioTx(vectorToHexString(v));
  }
}


/**
 * hear for radio packets avaiable
 * 
 * @param Packet packet class reference
 * if avaiable: put packet received into vector
 * else: stop
 */
void radioRx(Packet &pk){
  int packetSize = LoRa.parsePacket();
  pk.size=packetSize;
  if (packetSize) {
    //time received packet:
    while (LoRa.available()) {
      addValue((int)LoRa.read(), pk.data);
    }//check if packet had losses on his way
    pk.rssi=LoRa.packetRssi();
  } 
}

/**
 * receive packets, print it on the serial
 */
void serialRadioRx() {
  Packet pk;//instantiate the packet class

  uint32_t t1;
  uint32_t t2;
  uint32_t time_milliseconds;
 

  time_milliseconds = millis();
  t1 = micros();
  radioRx(pk);
  t2 = micros();
  if (pk.size>0) {
    //time received packet:
    // read packet and calculate the time difference    
    sprintln("------------------------------------------------");
    sprintln(pk.info().c_str());
    //try printing payload:
    sprintln(tryPrintPayload(pk.data).c_str());
    Serial.println("time spent waiting for the packet in microseconds:" + String(timeDifference(t1, t2)) + " received at: " + String(time_milliseconds) + " (milliseconds)");
    sprintln("---");

  }
}




/**
 * Build for the lora sensor 
 * uses a dht22 sensor to monitor temperature and humidity
 * 
 * can perform tasks remotely if a command message is sent through the private_channel
 * command messages start always with /
 */
#ifdef SENSOR

Parser ps; //parser class logic

/**
* use dht sensor to measure temperature,
* create a valid command to send throught /cantine 
* 
* @return "/cantine temperature:" + tmp + " humidity:" + hum   
*/
std::string dhtCall(){
  std::string command ="/cantine ";
  sensors_event_t event;

  //Look for temperature and humidity
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    throw std::invalid_argument("error reading temperature");
  } else {
    char buffer[50];
    sprintf(buffer, "temp: %.1f C°", event.temperature);
    command.append(buffer);
  }

  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    throw std::invalid_argument("Error reading humidity!");
  } else {
    char buffer[50];
    sprintf(buffer, "humidity: %.1f %%", event.relative_humidity);
    command.append(buffer);
  }

  //battery level:
  char battery_lvl[50];
  sprintf(battery_lvl," battery level: %i ",getBatteryLevel());
  command.append(battery_lvl);

  sprintln(command);
  return(command);
}

/**
 * Sleep using esp32 light sleep till it see an interrupt from DIO0 pin (got a message)
 * esp32 sleep set up on the setup
 */
void sleepTillMessage(){
  LoRa.receive();

  Serial.println("Sleeping...");
  
  digitalWrite(25,LOW);
  
  esp_light_sleep_start();
  
  digitalWrite(25,HIGH);
  
  Serial.println("Woke up from LoRa");

}

float getBatteryVoltage()
{
    uint32_t adc_mV = analogReadMilliVolts(35);

    // 100k / 100k voltage divider
    return (adc_mV / 1000.0f) * 2.0f;
}

int getBatteryLevel()
{
    float voltage = getBatteryVoltage();

    if (voltage >= 4.20f)
        return 100;

    if (voltage <= 3.20f)
        return 0;

    return (int)((voltage - 3.20f) / (4.20f - 3.20f) * 100.0f);
}


void switchSensor(std::string str){
  //the switch does a try because on many occasion the system will crash
  std::string s = str;

  Packet pk;
  try{
    switch (ps.main(s))
    {
    case ECHO:{
    // normal bash echo (/echo + text) — attempt and catch parsing errors
      std::string out = strAfterSpace(s, 1);
      sprintln(out.c_str());
      break;
    }
    case CANTINE:{
    // send packet in the private channel (attempt and catch parsing errors)
      uint8_t id[4] = {};
      randomBuffer(id, 4);
      uint8_t from[4] = {};
      strLitEndiConverter(Config::Device.name, from, sizeof(from));
    

      std::string payload = strAfterSpace(s, 1);
      sprintln(payload.c_str());
      std::vector<uint8_t> v = privateChannelMessage(payload, id, from);
      std::string msg = vectorToHexString(v);
    
      radioTx(msg); // sending packet
      break;
    }
    case PWR17:{
    //control power of Tx transmission, take second argument as new value of transmission
      std::string nr_s = strBetweenSpace(s,1);
      int value= std::stoi(nr_s);
      LoRa.setTxPower(value);
      sprintln("set power " + nr_s);
      break;
    }
    case GAIN:{
      std::string nr_s = strBetweenSpace(s,1);
      uint8_t value= std::stoi(nr_s);
      LoRa.setGain(value);
      sprintln("set gain " + nr_s);
      break;
    }
    case DHT:{
      //redundand
      dhtCall();
      break;
    };
    default:{
      sprintln(help().c_str());
      break;
    } 
    }
    }catch(const std::exception &e) {
      sprintln("passed bad command: ");
      sprintln(e.what());
      sprintln(help().c_str());
  }
}



void setup() {

  // hardware RNG enabled
  bootloader_random_enable();

  //turn on the led 
  pinMode(BOARD_LED, OUTPUT);

  //turn on the serial communication 
  Serial.begin(115200);
  
  //setup light sleep
  esp_sleep_enable_ext0_wakeup(
        (gpio_num_t)LORA_DIO0,
        1
  );

  //setup battery read 

  analogReadResolution(12);
  analogSetPinAttenuation(35, ADC_11db);

  //setup dht sensor: DHTPIN is 21
  dht.begin();
  Serial.println("DHT22 Unified Sensor Example");
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  Serial.print(F("Temperature Sensor: "));
  Serial.println(sensor.name);
  dht.humidity().getSensor(&sensor);
  Serial.print(F("Humidity Sensor:     "));
  Serial.println(sensor.name);
  uint32_t delayMS = sensor.min_delay / 1000;

  //set up LoRa
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
if (!LoRa.begin(869525024)) { //this frequency was created usign Meshtastic-lite's advice
    Serial.println("LoRa init failed!");
    while (1);
  }
  //preset for Meshtastic
  LoRa.setSpreadingFactor(9);
  LoRa.setSignalBandwidth(250E3);
  LoRa.setCodingRate4(5);
  LoRa.setSyncWord(0x2B);
  LoRa.setPreambleLength(16);
  LoRa.enableCrc();

  sprintln("compila con cpp:");
  Serial.println(__cplusplus);
  sprintln("LoRa init OK");
  sprintln("DHT sensor OK");
}

void loop() {
  sleepTillMessage();
  try{
    Packet pk;
    radioRx(pk);
    
    //evaluate if 
    if(pk.size>0){
      std::string msg = tryPrintPayload(pk.data);
      //if the channel hash matches and the message is "/dht" then send message
      if(ps.main(msg) == DHT && pk.data.at(13)==channelHash(Config::PrivateChannel.key,Config::PrivateChannel.name)){
        switchSensor(dhtCall());
        delay(1000);
    }
    }
  }catch(const std::exception &e){
    sprintln(e.what());
  }
}


/**
 * 
 * decoder build: talks on serial. Sends messages through LoRa protocol.
 * Commands are called using switchDecoder
 * Commands always begin with: /abcd ...
 * 
 * call /help to know the functionality
 */
#else


Parser ps;

/** 
 * this is the main function that handles the shell.
 *  takes as argument a cpp string, decides what to do with ps.main(). 
 *  call /help to know functions
 * 
 *  every non-hardware dependend functions are explained in function_dictionary.txt
 */
void switchDecoder(std::string str){
  std::string s = str;

  Packet pk;
  
  try{
    switch (ps.main(s))
    {
    case ECHO:{
    // normal bash echo (/echo + text) — attempt and catch parsing errors
      std::string out = strAfterSpace(s, 1);
      sprintln(out.c_str());
      break;
    }
    case SEND_KEY:{
      //send public key
      uint8_t id[4];
      randomBuffer(id, 4);
      uint8_t from[4];
      strLitEndiConverter(Config::Device.name, from, sizeof(from));
      
      std::vector<uint8_t> infos = encodePerson("NTMP", Config::Device.publicKey,"!" + Config::Device.name,"Temperature custom");
      
      radioTx(vectorToHexString(encodeMessageInfo(infos,from,id,Config::PublicChannel.key,0x03,0x00,0x00,0x03,0xfc,0x00)));
      break;
    }
    case PUB:{
    // send packet in the public channel (attempt and catch parsing errors)
      uint8_t id[4] = {};
      randomBuffer(id, 4);
      uint8_t from[4] = {};
      strLitEndiConverter(Config::Device.name, from, sizeof(from));
    
      std::string payload = strAfterSpace(s, 1);
      sprintln(payload.c_str());
      std::vector<uint8_t> v = publicMessage(payload, id, from);
      std::string msg = vectorToHexString(v);
      radioTx(msg); // sending packet
    
      break;
    }
    case CANTINE:{
    // send packet in the private channel (attempt and catch parsing errors)
      uint8_t id[4] = {};
      randomBuffer(id, 4);
      uint8_t from[4] = {};
      strLitEndiConverter(Config::Device.name, from, sizeof(from));
    

      std::string payload = strAfterSpace(s, 1);
      sprintln(payload.c_str());
      std::vector<uint8_t> v = privateChannelMessage(payload, id, from);
      std::string msg = vectorToHexString(v);
    
      radioTx(msg); // sending packet
      break;
    }
    case ADMN:{
    // send packet to LEO1
      uint8_t id[4] = {};
      randomBuffer(id, 4);
      uint8_t extra_nonce[4] = {};
      randomBuffer(extra_nonce, 4); 
      uint8_t from[4] = {};
      strLitEndiConverter(Config::Device.name, from, sizeof(from));
      uint8_t to[4] = {};      
      strLitEndiConverter(Config::Admn.name, to, sizeof(to)); 
      

      std::string payload = strAfterSpace(s, 1);
      sprintln(payload.c_str());
      std::vector<uint8_t> v = privateMessage(Config::Device.privateKey,Config::Admn.publicKey,payload,extra_nonce ,to, from,id);
      std::string msg = vectorToHexString(v);
      
      radioTx(msg); // sending packet
      break; 
    }
    case R:{
      switchDecoder(ps.last_command);
      break;
    }
    case RSSI:{
    //get rssi of the microcontroller
      int rssi = LoRa.rssi();
      std::string rssi_s = "rssi: "+ std::to_string(rssi);
      sprintln(rssi_s);
      break;
    }
    case PWR17:{
    //control power of Tx transmission, take second argument as new value of transmission
      std::string nr_s = strBetweenSpace(s,1);
      int value= std::stoi(nr_s);
      LoRa.setTxPower(value);
      sprintln("set power " + nr_s);
      break;
    }
    case GAIN:{
      std::string nr_s = strBetweenSpace(s,1);
      uint8_t value= std::stoi(nr_s);
      LoRa.setGain(value);
      sprintln("set gain " + nr_s);
      break;
    }
    case RAW:{
      std::string packet = strAfterSpace(s,1);
      if(packet.size()==0){
        throw std::invalid_argument("passed to /raw command an empty string");
      }
      radioTx(std::string(packet.begin(),packet.end()-1));
      break;
    }
    default:{
      sprintln(help().c_str());
      break;
    } 
    
    }
    }catch(const std::exception &e) {
      sprintln("passed bad command: ");
      sprintln(e.what());
      sprintln(help().c_str());
  }
}



void setup() {

// true RNG
bootloader_random_enable();

  //turn on the led 
  pinMode(BOARD_LED, OUTPUT);

  digitalWrite(25, HIGH);

  //turn on the serial communication 
  Serial.begin(115200);

  //set up LoRa
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(869525024)) { //this frequency was created usign Meshtastic-lite's advice
    Serial.println("LoRa init failed!");
    while (1);
  }
  //preset for Meshtastic
  LoRa.setSpreadingFactor(9);
  LoRa.setSignalBandwidth(250E3);
  LoRa.setCodingRate4(5);
  LoRa.setSyncWord(0x2B);
  LoRa.setPreambleLength(16);
  LoRa.enableCrc();

  sprintln("compila con cpp:");
  Serial.println(__cplusplus);
  sprintln("LoRa init OK");
}





void loop() {
  
try{
    if (Serial.available()) {
      //if it reads something from the Serial: go Tx mode and send the message heard.

      String input = Serial.readString();
      if(input.isEmpty()){
        ;
      }
      else{
        input.trim();  // Removes \n, \r, and spaces
        //convert arduino string into cpp string:
        try {
          std::string cpp_string = input.c_str();
          switchDecoder(cpp_string);
  
        }
        catch (const std::exception& e) {
          sprintln(e.what());
        }
      }
    }
    serialRadioRx();
  }catch(const std::exception &e){
    sprintln(e.what());
  }
}
#endif
