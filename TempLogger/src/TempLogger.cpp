/*  Created by Leandro Späth (c) 2016  https://github.com/leoDJ
 *  The following libraries were used:
 *  https://www.pjrc.com/teensy/td_libs_OneWire.html
 *  https://github.com/milesburton/Arduino-Temperature-Control-Library
 *  (both are available through the Arduino library manager)
 */
// Include the libraries we need
#include <ESP8266WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
extern "C" {
  #include <user_interface.h>
}

#define DEBUG false

#define vcc_correction 100
ADC_MODE(ADC_VCC);

// Data wire is plugged into GPIO 2 of the ESP8266
#define ONE_WIRE_BUS     2
const byte sensorCount = 2;
const byte avgCount    = 5; //number of readings to take per datapoint
const long sleepTime   = 300000; //time to sleep between readings in ms

const char* ssid     = "xxxxxx";
const char* password = "xxxxxxxx";
const char* host     = "api.thingspeak.com";
const char* apiKey   = "XXXXXXXXXXXXXXXX"; //API write key from ThingSpeak.com Channel

float fields[sensorCount+1];

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

float getVcc()
{
  return ((float)ESP.getVcc()+vcc_correction)/1000;
}

float getAvgVcc(int count)
{
  double vccSum = 0;
  for (int i = 0; i < count; ++i)
  {
    vccSum += getVcc();
    delay(1);
  }
  return (float)(vccSum / count);
}

void sendData()
{
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }

  String url = "/update?api_key=";
  url += apiKey;
  for(int i = 0; i < sizeof(fields)/sizeof(fields[0]); ++i)
  {
    url += "&field";
    url += String(i+1) + "=";
    url += String(fields[i], i==0 ? 3 : 2); //3 decimal places for voltage, 2 for temperature
  }
  Serial.println();

  Serial.print("Requesting URL: ");
  Serial.println(host+url);

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");
  delay(10);

  // Read all the lines of the reply from server and print them to Serial
  #if DEBUG
  Serial.println("Respond:");
  while(client.available()){
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }
  Serial.println();
  #endif


  Serial.println("closing connection");
  Serial.println();
}

void getAvgTemp(byte count)
{
  for(int s = 0; s < sensorCount; ++s)
  {
    fields[s+1] = 0;
  }
  for(int i = 0; i < count; ++i)
  {
    sensors.requestTemperatures();
    for(int s = 0; s < sensorCount; ++s)
    {
      fields[s+1] += sensors.getTempCByIndex(s);
    }
  }
  for(int s = 0; s < sensorCount; ++s)
  {
    fields[s+1] = (float)(fields[s+1] / count);
  }
}

void setup(void)
{
  // start serial port
  Serial.begin(115200);
  Serial.println("\n >> ESP8266 temperature logger << ");
  Serial.print("VCC: ");
  Serial.print(getVcc(), 3);
  Serial.println("V");

  sensors.begin();

  delay(500);
  // attempt to connect to Wifi network:
  Serial.print("Connecting Wifi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  IPAddress ip = WiFi.localIP();
  Serial.println(ip);
}

/*
 * Main function, get and show the temperature
 */
void loop(void)
{
  unsigned long start = millis();
  float vcc = getVcc();
  fields[0] = vcc;
  Serial.print(vcc, 3);
  Serial.print("V, ");
  getAvgTemp(avgCount);
  for(int i = 0; i < sensorCount; ++i)
  {
    float temp = fields[i+1];
    Serial.print(temp);
    Serial.print("°C ");
  }
  Serial.println();
  sendData();

  unsigned long execTime = millis() - start;
  delay(sleepTime - execTime); //sleep 5 min (with calibration) (ESP will go into light sleep automatically)
}
