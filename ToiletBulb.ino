#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
extern "C" {
#include "user_interface.h"
}
extern "C" {
#include "gpio.h"
}
//#define WIFI_WORK
#define WIFI_HOME
#define DEBUG

#ifdef DEBUG
  #define LOG(msg) Serial.println(msg);
#else
  #define LOG(msg)
#endif

#ifdef WIFI_HOME
  #define SSID "RED"
  #define PASSWORD "******"
#else
   #define SSID "InteticsGuest"
  #define PASSWORD "******"
#endif

#define timeInSeconds 40
#define ENTERED h0x01
#define OCCUPIED h0x02
#define FREE h0x03
#define SLEEP h0x04
#define MOVEMENT h0x05

enum ToiletState {
  ENTERED,
  OCCUPIED,
  FREE,
  SLEEP,
  MOVEMENT
};

void ICACHE_RAM_ATTR onMovement();

const char* ssid = SSID;
const char* password = PASSWORD;
const byte interruptPin = 2;
volatile ToiletState toiletState = SLEEP;
int currentTime = 0;
int movementTime = 0;

void setup() {
  Serial.begin(115200);

  initWifi();
  initGpio();
}
 
void onMovement() {
  if(toiletState == OCCUPIED){
     toiletState = MOVEMENT;
     return;
  }
  toiletState = ENTERED;
}

void loop() {   
  switch(toiletState){
    case ENTERED: {
      LOG("Someone entered in toilet"); 
      updateToiletStatus("Toilet is occupied now:(");
      toiletState = MOVEMENT;
      break;
    }
    case OCCUPIED:{
      //LOG("Toilet is occupied");
      currentTime = millis();
      if(isNoMovementFor(timeInSeconds)){
        LOG("There is no movement for 10 sec");
        toiletState = FREE;
      }
      break;
    }
    case MOVEMENT: {
      LOG("Movement is recognized");
      movementTime = millis();
      toiletState = OCCUPIED;
      break;
    }
    case FREE: {
      LOG("Toilet is free"); 
      updateToiletStatus("Toilet is free again:)");
      toiletState = SLEEP;
      break;
    }
    case SLEEP: {
      LOG("Go to sleep"); 
      startSleep();
      break;
    }
  }
}

void updateToiletStatus(String msg){
  if(WiFi.status()== WL_CONNECTED)
    { 
      LOG("Start updating status"); 
      
      HTTPClient https;
      BearSSL::WiFiClientSecure secureClient;
      secureClient.setInsecure();
      
      String url = "api.telegram.org";
      String query = "/bot1044489428:AAGp2-0r_Tffp6tBRwshdCeB8tfVZ7lVgNM/sendMessage?chat_id=@toiletbulb&text=" + msg;
      
      https.begin(secureClient, url, 443, query, true);
      int httpCode = https.GET();   //Send the request
 
      LOG(httpCode);   //Print HTTP return code
      https.end();  //Close connection
    }
    else
    {
      LOG("Error in WiFi connection");
    }
}

void startSleep() {
  LOG("Sleep started");
  wifi_station_disconnect();
  wifi_set_opmode(NULL_MODE);
  wifi_fpm_set_sleep_type(LIGHT_SLEEP_T);
  wifi_fpm_open();
  gpio_pin_wakeup_enable(GPIO_ID_PIN(interruptPin), GPIO_PIN_INTR_HILEVEL);
  wifi_fpm_do_sleep(0xFFFFFFF); // sleep for longest possible time
  delay(200);

  gpio_pin_wakeup_disable();
  LOG("Waked up");
  wifi_fpm_close();
  wifi_set_opmode(STATION_MODE);
  wifi_station_connect();
  initWifi();
  initGpio();
  delay(100);
  LOG("Ended sleep routine commands");
}

void initGpio() {
  LOG("Initializing port");
  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin), onMovement, RISING);
}

void initWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  LOG("Connecting to ");
  LOG(ssid);
  while ((WiFi.status() != WL_CONNECTED)) {
    delay(500);
    Serial.print(".");
  }
  LOG("WiFi connected, IP address: ");
  LOG(WiFi.localIP());
}

bool isNoMovementFor(int timeInSec){
  return currentTime - movementTime > (timeInSec*1000);
}
