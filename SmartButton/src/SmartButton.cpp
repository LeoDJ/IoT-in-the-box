/*  Created by Leandro Späth (c) 2016  https://github.com/leodj
 *  The following libraries were used:
 *  https://github.com/witnessmenow/ESP8266-TelegramBot
 *  https://github.com/bblanchon/ArduinoJson
 */


#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266TelegramBOT.h>
extern "C" {
  #include <user_interface.h>
}
extern "C" {
  #include <gpio.h>
}
#include "emojis.h"
emojis emoji;
ADC_MODE(ADC_VCC);
#define vcc_correction 100
const float batLowVolt = 2.7;
#define warnInterval 21600000 //remind every 6 hours

// Initialize Wifi connection to the router
const char* ssid     = "xxxxxxxx";
const char* password = "xxxxxxxx";


// Initialize Telegram BOT
#define BOTtoken "xxxxxxxxxxxxxxxxxx"  //Telegram Bot token (@botfather)
ESP8266TelegramBOT bot(BOTtoken);

const char* adminID  = "xxxx"; //chat ID of admin account
const char* sendToID  = "xxxx"; //chat ID for message recipient

const char* msg1 = "Es hat jemand geklingelt! " + emoji.bell;


const int ledPin = 16, sw1Pin = 0;
const int Bot_mtbs = 15000; //interval between checking for new Telegram messages
unsigned long Bot_lasttime=0, curMillis, ledLastTime=0, lastSW1Time=0, lowWarnLastTime=0;
bool blinkLED = false, blinkOnce = false, buttonPressed[2] = {false};



float getVcc()
{
  return ((float)ESP.getVcc()+vcc_correction)/1000;
}


void logMsg(bool isIncoming, String chat_id, String text)
{
  String m = "";
  m += String(isIncoming ? "< [" : "> [");
  m += chat_id;
  m += "]  \"";
  m += text;
  m += "\"\n";
  Serial.print(m);
}


void sendMsg(String chat_id, String text)
{
  bot.sendMessage(chat_id, text, "");
  logMsg(false, chat_id, text);
}

void sendVccMsg(String chat_id)
{
  float v = getVcc();
  String m = "";
  m += "Die Versorgungsspannung beträgt aktuell ";
  m += String(v);
  m += "V. ";
  m += v > batLowVolt ? "Ein Aufladen ist nicht nötig. " + emoji.battery : "Batterie schwach. Bitte aufladen. " + emoji.plug;
  sendMsg(chat_id, m);
}


void handleMessages(int numNewMsgs)
{
  for(int i = 0; i < numNewMsgs; i++)
  {
    String chat_id = String(bot.messages[i].chat_id);
    String text    = bot.messages[i].text;
    logMsg(true, chat_id, text);
    if(chat_id == sendToID || chat_id == adminID)
    {
      if (text=="/start")
      {
        sendMsg(chat_id, "Du hast mich gestartet. Ich sende nun auf Knopfdruck eine Nachricht.");
      }
      else if (text=="/status")
      {
        sendVccMsg(chat_id);
      }
    }
    else //reply to messages from strangers
    {
      if (text=="/start")
      {
        sendMsg(chat_id, "You have started me.%0ASadly, I'm only intended for private purpouses. For more information, contact my creator @LeoDJ.");
      }
    }
  }
}

void switchPressed()
{
  if(!digitalRead(sw1Pin) && millis() > lastSW1Time + 10000)
  {
    Serial.println("SW 1 pressed");
    blinkOnce = true;
    buttonPressed[0] = true;
    lastSW1Time = millis();
  }
}

void checkNewMsgs()
{
  int numNewMessages = bot.getUpdates(bot.last_message_recived + 1);   // launch API GetUpdates up to xxx message
  if(numNewMessages) {
    //Serial.println("got response");
    handleMessages(numNewMessages);
  }
}


void setup()
{
  Serial.begin(115200);
  Serial.println("\n>> ESP8266 SmartButton <<");
  Serial.print("VCC: ");
  Serial.print(getVcc(), 3);
  Serial.println("V");
  delay(3000);

  pinMode(ledPin, OUTPUT);
  // attempt to connect to Wifi network:
  Serial.print("Connecting Wifi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(ledPin, !digitalRead(ledPin));
    delay(250);
    Serial.print(".");
  }
  digitalWrite(ledPin, HIGH);
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  IPAddress ip = WiFi.localIP();
  Serial.println(ip);

  bot.begin();      // launch Bot functionalities
  pinMode(sw1Pin, INPUT_PULLUP);
  attachInterrupt(sw1Pin, switchPressed, FALLING);
  if(getVcc() < batLowVolt)
    sendVccMsg(sendToID);
}


void loop()
{
  curMillis = millis();


  if (curMillis > Bot_lasttime + Bot_mtbs)
  {
    Bot_lasttime = curMillis;
    checkNewMsgs();
  }


  if(curMillis > ledLastTime + 250) //won't work as good due to delay(1000), but isn't used anyways
  {
    ledLastTime = curMillis;
    if(blinkLED || blinkOnce)
      digitalWrite(ledPin, !digitalRead(ledPin));
    else
      digitalWrite(ledPin, HIGH);
    if(blinkOnce)
      blinkOnce = false;
  }

  if(buttonPressed[0])
  {
    buttonPressed[0] = false;
    sendMsg(sendToID, msg1);
  }

  if(getVcc() < batLowVolt && curMillis > lowWarnLastTime + warnInterval)
  {
    lowWarnLastTime = curMillis;
    sendVccMsg(adminID);
  }

  wifi_set_sleep_type(LIGHT_SLEEP_T);
  delay(1000);
}
