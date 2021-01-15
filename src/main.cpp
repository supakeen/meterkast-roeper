#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <SoftwareSerial.h>

void callback(char*, byte*, unsigned int);
void reconnect();

IPAddress mqttServer(192, 168, 1, 10);
WiFiClient wifiClient = WiFiClient();
PubSubClient mqttClient(mqttServer, 1883, callback, wifiClient);

SoftwareSerial software_serial(D1, -1, true);

void setup() {
  Serial.begin(115200);
  Serial.println("setup: begin");
  
  pinMode (D1, INPUT);
  software_serial.begin(115200);

  WiFi.begin("X", "X");

  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.print("Connected, IP = ");
  Serial.println(WiFi.localIP());
  
  Serial.println("setup: done");
}

void loop() {
  yield();
  
  if (!mqttClient.connected()) {
    reconnect();
  }
  mqttClient.loop();
  
  interpret_software_serial();
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println("Callback");
  Serial.println((char) payload[0]);
}

void reconnect() {
 Serial.println("Connecting to MQTT Broker...");
 while (!mqttClient.connected()) {
     Serial.println("Reconnecting to MQTT Broker..");
     String clientId = "esp8266-electrical-a";
     clientId += String(random(0xffff), HEX);
    
     if (mqttClient.connect(clientId.c_str())) {
       Serial.println("Connected.");
       // subscribe to topic      
     }
 }
}

void interpret_software_serial() {
  static char circle[1024] = { 0 };
  static size_t index = 0;

  static bool seen_start = false;
  static bool seen_end = false;
  
  if(software_serial.available()) {
    while(software_serial.available()) {
      char c = software_serial.read();
      
      circle[index] = c;
      index++;

      if(c == '/') {
        seen_start = true;
      }
      
      if(c == '!') {
        seen_end = true;
      }

      if(seen_start && seen_end) {
        resilient_parse(circle);

        seen_start = false;
        seen_end = false;
        index = 0;
        
        memset(circle, '\0', sizeof(circle));
      }

      if(index == 1023) {
        seen_start = false;
        seen_end = false;
        
        memset(circle, '\0', sizeof(circle));
        index = 0;
      }
      
      yield();
    }
  }
}

double find_electricity_consumption_high(char *circle) {
  static double lst = 0;
  static double cur = 0;
  
  char *ss = strstr(circle, "1-0:1.8.2(");
  char v[11] = { 0 };
  
  if(!ss) return 0;

  memcpy(v, ss + 10, 10);  
  
  Serial.print("find_electricity_consumption_high: ");
  Serial.print(v);
  Serial.println();

  cur = atof(v);

  if(cur < lst) {
    return 0;
  }

  lst = cur;
  return cur;
}

double find_electricity_consumption_low(char *circle) {
  static double lst = 0;
  static double cur = 0;

  char *ss = strstr(circle, "1-0:1.8.1(");
  char v[11] = { 0 };
  
  if(!ss) return 0;

  memcpy(v, ss + 10, 10);  
  
  Serial.print("find_electricity_consumption_low: ");
  Serial.print(v);
  Serial.println();

  cur = atof(v);

  if(cur < lst) {
    return 0;
  }

  lst = cur;
  return cur;
}

double find_electricity_usage(char *circle) {
  char *ss = strstr(circle, "1-0:1.7.0(");
  char v[7] = { 0 };
  
  if(!ss) return 0;

  memcpy(v, ss + 10, 6);  
  
  Serial.print("find_electricity_usage: ");
  Serial.print(v);
  Serial.println();

  return atof(v);
}

double find_gas_consumption(char *circle) {
  static double lst = 0;
  static double cur = 0;
  
  char *ss = strstr(circle, "0-1:24.2.1(");
  char v[12] = { 0 };
  
  if(!ss) return 0;

  memcpy(v, ss + 26, 9);
  
  Serial.print("find_gas_consumption: ");
  Serial.print(v);
  Serial.println();

  cur = atof(v);

  if(cur < lst) {
    return 0;
  }

  lst = cur;
  return cur;
}

void resilient_parse(char *circle) {
  Serial.println("resilient_parse: start");

  double electricity_consumption_high;
  if((electricity_consumption_high = find_electricity_consumption_high(circle))) {
    char str[128] = { 0 };
    snprintf(str, sizeof(str), "electricity-consumption-high value=%f", electricity_consumption_high);
    mqttClient.publish("/esp8266/electricity", str, true);
    Serial.println(electricity_consumption_high);
    // pass
  }

  double electricity_consumption_low;
  if((electricity_consumption_low = find_electricity_consumption_low(circle))) {
    char str[128] = { 0 };
    snprintf(str, sizeof(str), "electricity-consumption-low value=%f", electricity_consumption_low);
    mqttClient.publish("/esp8266/electricity", str, true);
    Serial.println(electricity_consumption_low);
    // pass
  }

  double electricity_usage;
  if((electricity_usage = find_electricity_usage(circle))) {
    char str[128] = { 0 };
    snprintf(str, sizeof(str), "electricity-usage value=%f", electricity_usage);
    mqttClient.publish("/esp8266/electricity", str, true);
    Serial.println(electricity_usage);
    // pass
  }

  double gas_consumption;
  if((gas_consumption = find_gas_consumption(circle))) {
    char str[128] = { 0 };
    snprintf(str, sizeof(str), "gas-consumption value=%f", gas_consumption);
    mqttClient.publish("/esp8266/gas", str, true);
    Serial.println(gas_consumption);
    // pass
  }
  
  Serial.println("resilient_parse: end");
}
