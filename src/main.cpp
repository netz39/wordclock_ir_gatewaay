#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "Hardware.h"
#include "WordConversion.h"
#include "Config.h"

#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <IRsend.h>

#define IR_LED D1  //D1 für WörterUhr
IRsend irsend(IR_LED);
decode_results results;

const uint16_t kRecvPin = D7;
IRrecv irrecv(kRecvPin);


bool test_IR=false;

#ifndef STASSID
#define STASSID "NETZ39"
#define STAPSK  "******"
#endif

const char* ssid = STASSID;
const char* password = STAPSK;

#include <PubSubClient.h>
char msg[50];
volatile bool mqttSpaceStatus =false;
const char *mqtt_server = "mqtt.n39.eu";
const char *in_topic = "Netz39/Things/StatusSwitch/Lever/State";
const char *in_topic_IR = "Netz39/Things/WordClock/IR";
WiFiClient espClient;
PubSubClient mqtt_client(espClient);

void mqtt_callback(char *topic, byte *payload, unsigned int length) {
  String m;
  for (size_t i = 0; i < length; ++i)m += String((char)payload[i]);
  Serial.print("MQTT Topic: ");
  Serial.println(topic);
  Serial.print("MQTT Message: ");
  Serial.println(m);  
  
  if (strcmp(topic,in_topic)==0){
    if(m.equals(String("open"))) mqttSpaceStatus=true;
    if(m.equals(String("closed"))) mqttSpaceStatus=false;
    Serial.println("Topic: Netz39/SpaceAPI/isOpen");
  }

  if (strcmp(topic,in_topic_IR)==0){
    if(m.equals(String("+"))) irsend.sendNEC(0x5EA158A7);
    if(m.equals(String("-"))) irsend.sendNEC(0x5EA1D827);
    if(m.equals(String("m"))) irsend.sendNEC(0x5EA138C7);
    if(m.equals(String("s_on"))) irsend.sendNEC(0x5EA1B847);
    if(m.equals(String("s_off"))) irsend.sendNEC(0x5EA17887);
    if(m.equals(String("ch1"))) irsend.sendNEC(0x5EA1AA55);    
    if(m.equals(String("ch2"))) irsend.sendNEC(0x5EA1837C); 
    if(m.equals(String("ch3"))) irsend.sendNEC(0x5EA1A857); 
    
    if(m.equals(String("1"))) irsend.sendNEC(0xFFAA55);
    if(m.equals(String("2"))) irsend.sendNEC(0xFFA857);
    if(m.equals(String("3"))) irsend.sendNEC(0xFF58A7);
    if(m.equals(String("4"))) irsend.sendNEC(0xFFBA45);
    if(m.equals(String("5"))) irsend.sendNEC(0xFF38C7);
    if(m.equals(String("u"))) irsend.sendNEC(0xFF7887);  
    if(m.equals(String("d"))) irsend.sendNEC(0xFF3AC5);

    if(m.equals(String("b_pow"))) irsend.sendNEC(0x10C8E11E);  
    if(m.equals(String("b_f"))) irsend.sendNEC(0x10C8718E);    
    

    if(m.equals(String("OFF"))){
      irsend.sendNEC(0x10C8E11E);
      irsend.sendNEC(0x5EA17887);
      irsend.sendNEC(0x10C8E11E);
    }
    
    //irsend.sendNEC(0x10C8E11E);
    //irsend.sendNEC(0x5EA17887);
    Serial.println("Mute Receiver");
    //test_IR =!test_IR;
    //digitalWrite(IR_LED, test_IR);    
  }

}

void mqtt_reconnect() {
  // Loop until we're reconnected
  Serial.print("Attempting MQTT connection...");
  // Attempt to connect
  if (mqtt_client.connect("WordClock")) {
    Serial.println("connected");
    // ... and resubscribe
    mqtt_client.subscribe(in_topic);
    mqtt_client.subscribe(in_topic_IR);
  } else {
    Serial.print("failed, rc=");
    Serial.print(mqtt_client.state());
  }
}

#define NEOPIXEL_BRIGHTNESS 20 //[0 - 255]; the normal ligth level if the Led Band
enum BrighnessLevel {normal, high, low};
uint32_t getColor(uint8_t uww, uint8_t cw, uint8_t ww, BrighnessLevel levelOfBrightness = normal){
  float temp0;
  if (levelOfBrightness == normal) temp0 = 1;
  if (levelOfBrightness == high) temp0 = 1;
  if (levelOfBrightness == low) temp0 = 1;
  uint8_t temp1 = (uww * temp0);
  uint8_t temp2 = (cw * temp0);
  uint8_t temp3 = (ww * temp0);
  return ((uint32_t)temp1 << 16) | ((uint32_t)temp2 <<  8) | (uint32_t)temp3;
}

uint32_t wordColor, wordColorNew;

#include <Adafruit_NeoPixel.h>
Adafruit_NeoPixel LED_Strip_Words = Adafruit_NeoPixel(LED_Strip_Words_NumberofLEDs, LED_Strip_Words_Pin, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel LED_Strip_Status = Adafruit_NeoPixel(LED_Strip_Status_NumberofLEDs, LED_Strip_Status_Pin, NEO_GRB + NEO_KHZ800);

#include "NTPClient.h"
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
const uint32_t fiveMinutes = (5*60*1000);
const uint32_t numberOfFiveMinutes = 12*60/5; //12h * 60min / 5min
uint16_t counterOfFiveMinutes = 0;
bool hourOffset = true;
int8_t timeZoneOffset = 1;
bool newNTPUpdate;
uint32_t delayNTPUpdate;

uint16_t calculate_numberOfFiveMinutes(uint8_t _hours, uint8_t _minutes){
  return (uint16_t)(((_hours*60)+_minutes)/5);
}
void activateLedsForHours(){
  uint16_t temp_hour = (uint16_t) (counterOfFiveMinutes*5/60);
  //if(hourOffset)
  temp_hour++;
  temp_hour %= 12;
  //Serial.print("temp_hour:\t");
  //Serial.println(temp_hour);
  for(uint8_t i=0;i<hours[temp_hour].numberOfLeds;i++)
    LED_Strip_Words.setPixelColor(hours[temp_hour].led_position+i,wordColor);
}

void activateLedsForMinutes(){
  uint16_t temp_minute = counterOfFiveMinutes%12;
  //Serial.print("temp_minute:\t");
  //Serial.println(temp_minute);
  for(uint8_t i=0;i<7;i++)
    if(minutes[temp_minute]&(1<<i))
      for(uint8_t j=0;j<rest[i].numberOfLeds;j++)
        LED_Strip_Words.setPixelColor(rest[i].led_position+j,wordColor);
  if(minutes[temp_minute]&(1<<7)) hourOffset=true;
    else hourOffset = false;
}

void activateLedsForText(){
  for(uint8_t j=0;j<2;j++)
        LED_Strip_Words.setPixelColor(rest[7+j].led_position,wordColor);  
}


uint32_t lastTime =0;
uint32_t diffTime =0;

void setup() {
  irrecv.enableIRIn();  // Start the receiver
  pinMode(IR_LED,OUTPUT);
  Serial.begin(115200);
  irsend.begin();
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.hostname("WordClock");
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  ArduinoOTA.setHostname("WordClock");
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  mqtt_client.setServer(mqtt_server, 1883);
  mqtt_client.setCallback(mqtt_callback);
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  //*********************************************************
  #define COLOR_DELAY 500
  LED_Strip_Status.begin();
  LED_Strip_Status.setBrightness(50);
  LED_Strip_Status.fill(LED_Strip_Status.Color(255,0,0));
  LED_Strip_Status.show();
  
  LED_Strip_Words.begin();
  LED_Strip_Words.setBrightness(255);
  LED_Strip_Words.fill(getColor(255,0,0));
  LED_Strip_Words.show();
  delay(COLOR_DELAY);

  LED_Strip_Status.fill(LED_Strip_Status.Color(0,255,0));
  LED_Strip_Status.show();
  LED_Strip_Words.fill(getColor(0,255,0));
  LED_Strip_Words.show();
  delay(COLOR_DELAY);

  LED_Strip_Status.fill(LED_Strip_Status.Color(0,0,255));
  LED_Strip_Status.show();
  LED_Strip_Words.fill(getColor(0,0,255));
  LED_Strip_Words.show();
  delay(COLOR_DELAY);

  LED_Strip_Status.fill(LED_Strip_Status.Color(255,255,255));
  LED_Strip_Status.show();  
  LED_Strip_Words.fill(getColor(255,255,255));
  LED_Strip_Words.show();  
  delay(COLOR_DELAY);
  wordColor = getColor(255,255,255);
//*********************************************************

  timeClient.begin();
  timeClient.setTimeOffset(2);
  timeClient.update();
  lastTime = millis();
  Serial.print("aktuelle Uhrzeit:\t");
  Serial.print(timeClient.getHours());
  Serial.print(":");
  Serial.print(timeClient.getMinutes());
  Serial.print(":");
  Serial.println(timeClient.getSeconds());
  delayNTPUpdate =  (4-timeClient.getMinutes()%5)*60+(60-timeClient.getSeconds());
  newNTPUpdate = true;
  Serial.print("Delay to Next 5Min Update: ");
  Serial.println(delayNTPUpdate);
  delayNTPUpdate *=1000;
  counterOfFiveMinutes = calculate_numberOfFiveMinutes(timeClient.getHours()+timeZoneOffset, timeClient.getMinutes());
  LED_Strip_Words.clear();
  activateLedsForMinutes();
  activateLedsForHours();
  activateLedsForText();
  LED_Strip_Words.show();
}

#define SPACE_CLOSE_MAX_BLUE 200
#define SPACE_GREEN_BORDER 120
#define SPACE_OPEN_MAX_BLUE 255

uint32_t getSpaceColor(){
  static int16_t red=255 , green=0, blue=0;
  static bool colorDirection =false; //true =>right, false=>left
  static bool statusColor;

  if(!statusColor){
    if(green>SPACE_GREEN_BORDER)green--;
    else{
      if(!colorDirection){
        if(green) green--;
          else blue++;
        if(blue>=SPACE_CLOSE_MAX_BLUE) colorDirection =true;
      }else{
        if(blue) blue--;
          else green++;
        if(green==SPACE_GREEN_BORDER){
          if(mqttSpaceStatus) statusColor = true;
            else colorDirection =false;
        }
      }
    }
  }else{
    if(green<255){
      green++;
    }else{
      if(colorDirection){
        if(red) red--;
          else blue++;
        if(blue>=SPACE_CLOSE_MAX_BLUE) colorDirection =false;
      }else{
        if(blue) blue--;
          else red++;
        if(red==255)
          if(!mqttSpaceStatus) statusColor = false;
            else colorDirection =true;      
      }
    }
  }
  /*
  Serial.print("RGB Status:\t");
  Serial.print(red);
  Serial.print(".");
  Serial.print(green);
  Serial.print(".");
  Serial.println(blue);
  Serial.print("colorDirection:\t");
  Serial.println(colorDirection);
  Serial.print("statusColor:\t");
  Serial.println(statusColor);  */
  return LED_Strip_Status.Color(red, green, blue);  
}



void spaceStatusColor(){
  LED_Strip_Status.setPixelColor(0, getSpaceColor());
  for(uint8_t i=0;i<5;i++) getSpaceColor();
  for(uint8_t a=1; a<LED_Strip_Status.numPixels(); a++) {
    LED_Strip_Status.setPixelColor(a,LED_Strip_Status.getPixelColor(a-1));
    //for(uint8_t i=0;i<10;i++) getSpaceColor();
  }
  LED_Strip_Status.show();
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {  //255 >= Input > 170
    return LED_Strip_Status.Color(255 - WheelPos * 3, 0, WheelPos * 3); //only red & blue
  }
  if(WheelPos < 170) { //170 >= Input > 85
    WheelPos -= 85;
    return LED_Strip_Status.Color(0, WheelPos * 3, 255 - WheelPos * 3);//only green & blue
  }
  WheelPos -= 170;    //85 >= Input >= 0 
  return LED_Strip_Status.Color(WheelPos * 3, 255 - WheelPos * 3, 0);  //only red & green
}

void rainbow() {
  static uint16_t b=0;
  static bool colorDirection =true;

  for(uint16_t a=0; a<LED_Strip_Status.numPixels(); a++) {
    LED_Strip_Status.setPixelColor(a, Wheel((a+b)));
  }

  b=(b+1)%256;
  LED_Strip_Status.show();
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle() {
  static uint16_t d;
  for(uint16_t i=0; i< LED_Strip_Status.numPixels(); i++) {
    LED_Strip_Status.setPixelColor(i, Wheel(((i * 256 / LED_Strip_Status.numPixels()) + d) & 0xFF));
  }
  LED_Strip_Status.show();
  d=(d+1)%(256*5);// 5 cycles of all colors on wheel
}

uint16_t timeDiff_AnalogRead,lastTime_AnalogRead, Value_AnalogRead;
bool AnalogChange = false;

long lastReconnectAttempt = 0;

boolean reconnect() {
  if (mqtt_client.connect("WordClock")) {
    // ... and resubscribe
    mqtt_client.subscribe("inTopic");
  }
  return mqtt_client.connected();
}

uint32_t timeDiff_rainbow, lastTime_rainbow;
#define rainbowDelay 200
void loop() {
  
  ArduinoOTA.handle();

  static uint32_t lastIRRec;
  if(millis() - lastIRRec >= 100){
    if (irrecv.decode(&results)) {
      // print() & println() can't handle printing long longs. (uint64_t)
      serialPrintUint64(results.value, HEX);
      Serial.println("");
      if(results.value==0x2FD52AD){
        static bool relais = true;
        relais=!relais;
        if(relais) mqtt_client.publish("/Lobby/Sonoff9796700/Relais", "OFF");
          else mqtt_client.publish("/Lobby/Sonoff9796700/Relais", "ON");
      }
      /*if(results.value==0x2FD48B7){
        irsend.sendNEC(0x10C8E11E);
        irsend.sendNEC(0x5EA17887);
        irsend.sendNEC(0x10C8E11E);
      }
      if(results.value==0x2FD807F){ //1
        irsend.sendNEC(0xFFAA55);
      }
      if(results.value==0x2FD40BF){ //2
        irsend.sendNEC(0xFFA857);
      }
      if(results.value==0x2FDC03F){ //3
        irsend.sendNEC(0xFF58A7);
      }
      if(results.value==0x2FD20DF){ //4
        irsend.sendNEC(0xFFBA45);
      }     
      if(results.value==0x2FDA05F){ //5
        irsend.sendNEC(0xFF38C7);
      }       */  
      irrecv.resume();  // Receive the next value
    }
    lastIRRec = millis();
  }
  
  /*
  if (!mqtt_client.connected()) {
    long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      // Attempt to reconnect
      if (reconnect()) {
        lastReconnectAttempt = 0;
      }
    }
  } else {
    // Client connected

    mqtt_client.loop();
  }
  */
  if (!mqtt_client.connected()) {
    mqtt_reconnect();
  }
  mqtt_client.loop();
  
  timeDiff_rainbow = millis() - lastTime_rainbow;
  if(timeDiff_rainbow >= rainbowDelay){
    //rainbow();
    //rainbowCycle();
    spaceStatusColor();
    lastTime_rainbow = millis();
  //Serial.print("timeDiff_rainbow:\t");
  //Serial.println(timeDiff_rainbow);
  }

#define HIGH_Ambient 400
#define MIDDLE_Ambient 8

  timeDiff_AnalogRead = millis() - lastTime_AnalogRead;
  if(timeDiff_AnalogRead >= 200){
    Value_AnalogRead = analogRead(A0);
    if(Value_AnalogRead>=HIGH_Ambient) wordColor = getColor(0,255,0);
    if(Value_AnalogRead>=MIDDLE_Ambient && Value_AnalogRead<HIGH_Ambient) wordColor = getColor(0,0,255);
    if( Value_AnalogRead<MIDDLE_Ambient) wordColor = getColor(255,0,0);
    AnalogChange = true;
    lastTime_AnalogRead = millis();
    //snprintf (msg, 50, "Helligkeit: %ld", Value_AnalogRead);
    //mqtt_client.publish("helligkeit", msg);
  }

  if(AnalogChange){
    LED_Strip_Words.clear();
    activateLedsForMinutes();
    activateLedsForHours();    
    activateLedsForText();
    LED_Strip_Words.show();
    AnalogChange = false;
  }
  
  diffTime = millis() - lastTime;
  if((diffTime >= fiveMinutes) || (newNTPUpdate && (diffTime>delayNTPUpdate))){ //wait 5min
    Serial.println("Aktualisiere Words");
    newNTPUpdate=false;
    counterOfFiveMinutes++;
    counterOfFiveMinutes%=numberOfFiveMinutes;
    if(counterOfFiveMinutes==0){
      timeClient.update();
      counterOfFiveMinutes = calculate_numberOfFiveMinutes(timeClient.getHours()+timeZoneOffset, timeClient.getMinutes());
      delayNTPUpdate =  (4-timeClient.getMinutes()%5)*60+(60-timeClient.getSeconds());
      delayNTPUpdate*=1000;
      //snprintf (msg, 50, "Helligkeit: %ld", delayNTPUpdate);
      //mqtt_client.publish("helligkeit", msg);
      newNTPUpdate = true;
    }
    LED_Strip_Words.clear();
    activateLedsForMinutes();
    activateLedsForHours();    
    activateLedsForText();
    LED_Strip_Words.show();
    
    lastTime = millis();
  }

  static uint32_t lastIRSend;
  if((millis() - lastIRSend >= 2000) && test_IR){
    //test_IR=(test_IR+1)%3;
    irsend.sendNEC(0x10C8718E);
    irsend.sendNEC(0x5EA138C7);
    lastIRSend = millis();
   // if(test_IR) digitalWrite(IR_LED,HIGH);
    //else digitalWrite(IR_LED,LOW);       
  }
  
}
