/*
    MQTT sensor ds18b20
    ver: 20221207-20230418
    = Vlapa = v.005
*/

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266HTTPUpdateServer.h>

#define ONE_WIRE_BUS 4 // D2
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

const uint8_t pinPlusDS = 5;  // D1;  //  питание DS18b20 "+"
const uint8_t pinMinusDS = 0; // D3; //  питание DS18b20 "-"
const uint8_t pinPROGR = 13;  // D7;   //  программрование (при запуске на землю)

//**************************************************************
const char ssid[] = "link";
// const char ssid[] = "MikroTik-2-ext";
// const char ssid[] = "MikroTik-2";
const char pass[] = "dkfgf#*12091997";

const char *mqtt_server = "178.20.46.157";
const uint16_t mqtt_port = 1883;
const char *mqtt_client = "sensor_DS18b20-002";
// const char *mqtt_client = "Home_bme280";
// const char *mqtt_client = "GasBoiler_009";
const char *mqtt_client2 = "TEST_IP";
const char *mqtt_user = "mqtt";
const char *mqtt_pass = "qwe#*1243";
const char *outTopicTemp = "/Temp_Battery";
const char *outTopicVolt = "/Volt";
// const char *outTopicTemp = "/countOut";
const char *outTopicIP = "/IP";
const uint32_t pauseSleep = 30 * 1000 * 1000; //  30 секунд спим
bool flagPROGR = false;

WiFiClient espClient;
PubSubClient client(espClient);

//**************************************************************
inline bool mqtt_subscribe(PubSubClient &client, const String &topic)
{
  // Serial.print("Subscribing to: ");
  // Serial.println(topic);
  return client.subscribe(topic.c_str());
}

//**************************************************************
inline bool mqtt_publish(PubSubClient &client, const String &topic, const String &value)
{
  // Serial.print("Publishing topic ");
  // Serial.print(topic);
  // Serial.print(" = ");
  // Serial.println(value);
  // Serial.println("\n");
  return client.publish(topic.c_str(), value.c_str());
}

//**************************************************************
void mqtt_callback(char *topic, byte *payload, unsigned int length)
{
  // for (uint8_t i = 0; i < length; i++)
  //   Serial.print((char)payload[i]);
  // Serial.println();

  // char *topicBody = topic + strlen(mqtt_client) + 1;
  // if (!strncmp(topicBody, inTopiccount, strlen(inTopiccount)))
  // {
  //   for (uint8_t i = 0; i < length; i++)
  //     outcountData += (char)payload[i];
  //   Serial.print("count: ");
  //   Serial.println(outcountData);
  //   outcountData = "";
  // }
}

//**************************************************************
void reconnect()
{
  uint8_t mqttCount = 20;
  while (!client.connected())
  {
    // Serial.print("Atcountting MQTT connection...");
    // digitalWrite(pinBuiltinLed, LOW);

    if (!client.connect(mqtt_client, mqtt_user, mqtt_pass))
    {
      if (mqttCount)
      {
        mqttCount--;
        delay(300);
      }
      else
      {
        ESP.deepSleep(pauseSleep);
      }
    }
  }
}

//**************************************************************
void WiFi_init()
{
  WiFi.begin(ssid, pass);
  uint8_t wifiCount = 20;
  Serial.print("Connection to:  ");
  Serial.println(ssid);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(300);
    Serial.print('.');
    if (wifiCount)
    {
      wifiCount--;
    }
    else
    {
      ESP.deepSleep(pauseSleep);
    }
  }
}

//**************************************************************
// медиана на 3 значения со своим буфером
uint32_t median(uint32_t newVal, uint8_t offSet)
{
  uint32_t count;
  uint32_t buf[3];

  ESP.rtcUserMemoryRead(offSet, &count, sizeof(count));
  ESP.rtcUserMemoryWrite(count, &newVal, sizeof(newVal));

  uint8_t k = 0;
  for (uint8_t i = offSet - 3; i < offSet; ++i)
  {
    ESP.rtcUserMemoryRead(i, &buf[k], sizeof(buf[k]));
    Serial.print(buf[k]);
    Serial.print('\t');
    k++;
  }
  Serial.println();

  if (++count >= offSet)
  {
    count = offSet - 3;
    ESP.rtcUserMemoryWrite(offSet, &count, sizeof(count));
  }
  else
  {
    ESP.rtcUserMemoryWrite(offSet, &count, sizeof(count));
  }

  uint32_t a = (max(buf[0], buf[1]) == max(buf[1], buf[2])) ? max(buf[0], buf[2]) : max(buf[1], min(buf[0], buf[2]));
  Serial.print("a = ");
  Serial.println(a);
  return (max(buf[0], buf[1]) == max(buf[1], buf[2])) ? max(buf[0], buf[2]) : max(buf[1], min(buf[0], buf[2]));
}

//**************************************************************
void setup()
{
  Serial.begin(115200);
  Serial.println("\n");

  // pinMode(14, OUTPUT);
  // digitalWrite(14, HIGH);

  pinMode(pinPROGR, INPUT_PULLUP);
  delay(1);
  flagPROGR = digitalRead(pinPROGR);

  pinMode(pinPlusDS, OUTPUT);
  digitalWrite(pinPlusDS, HIGH);
  pinMode(pinMinusDS, OUTPUT);
  digitalWrite(pinMinusDS, LOW);

  // digitalWrite(LED_BUILTIN, HIGH);
  Serial.println("\n");

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqtt_callback);

  if (flagPROGR)
  {
    sensors.begin();
    sensors.requestTemperatures();
    uint32_t t = sensors.getTempCByIndex(0) * 100;
    t = median(t, 3);

    String topic = "/";
    // topic += mqtt_client;
    // topic += outTopicIP;
    // mqtt_publish(client2, topic, WiFi.localIP().toString());
    // Serial.print(topic);
    // Serial.print(" = ");
    // Serial.println(WiFi.localIP().toString());

    // topic = "/";
    topic += mqtt_client;
    topic += outTopicTemp;

    Serial.print(topic);
    Serial.print(" = ");
    Serial.println((String)t);
    Serial.print("ESP - sleep ");
    Serial.print(pauseSleep / 1000000);
    Serial.println("s !");

    uint32_t d1 = t;
    Serial.print("d = ");
    Serial.println(d1);
    uint32_t a1;
    ESP.rtcUserMemoryRead(4, &a1, sizeof(a1));
    Serial.print("a = ");
    Serial.println(a1);

    if (a1 == d1)
    {
      pinMode(LED_BUILTIN, OUTPUT);
      digitalWrite(LED_BUILTIN, LOW);
      Serial.println("ESP deep sleep.\n");
      delay(10);
      ESP.deepSleep(pauseSleep);
    }
    else
    {
      ESP.rtcUserMemoryWrite(4, &d1, sizeof(d1));
      Serial.println("Data Write - Ok !");
    }

    delay(1);
    float u = (1.0 / 1023) * analogRead(A0) * 4.200;
    uint32_t u1 = u * 100;
    u1 = median(u1, 10);
    Serial.print("U1 = ");
    Serial.println(u1);

    String topic1 = "/";
    topic1 += mqtt_client;
    topic1 += outTopicVolt;

    WiFi_init();
    Serial.println("\nWiFi Ok !");
    reconnect();
    Serial.println("MQTT Ok !");
    uint8_t s1 = t / 100;
    uint8_t s2 = t % 100;
    String t2 = "";
    t2 += s1;
    t2 += '.';
    if (s2 < 10)
      t2 += '0';
    t2 += s2;
    mqtt_publish(client, topic, t2);

    t2 = "";
    s1 = u1 / 100;
    s2 = u1 % 100;
    t2 += s1;
    t2 += '.';
    if (s2 < 10)
      t2 += '0';
    t2 += s2;
    Serial.print("U = ");
    Serial.println(t2);
    mqtt_publish(client, topic1, t2);

    delay(100);
    ESP.deepSleep(pauseSleep);
  }
  else
  {
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    String topic = "/";
    topic += mqtt_client2;
    topic += outTopicIP;
    Serial.print(topic);
    Serial.print(" :  ");
    Serial.println(WiFi.localIP().toString());

    WiFi_init();
    reconnect();

    mqtt_publish(client, topic, WiFi.localIP().toString());
    delay(100);

    ESP8266WebServer server(80);
    ESP8266HTTPUpdateServer httpUpdater;
    server.begin();
    httpUpdater.setup(&server);

    while (1)
    {
      server.handleClient();
      client.loop();
      delay(1);
    }
  }
}

void loop() {}