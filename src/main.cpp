#include <WiFi.h>
#include <MQTT.h>
#include <SoftwareSerial.h>

#include <dsmr.h>

#include "setting.h"

#define RTS_PIN D3
#define RX_PIN RX

MQTTClient mqtt;
WiFiClient wificlient;

SoftwareSerial P1S;
P1Reader P1R(&P1S, 2);

struct Printer {
  template<typename Item>
  void apply(Item &i) {
    if (i.present()) {
      Serial.print(Item::name);
      Serial.print(F(": "));
      Serial.print(i.val());
      Serial.print(Item::unit());
      Serial.println();
    }
  }
};

void setup_wifi() {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while(WiFi.status() != WL_CONNECTED) delay(500);
}

void loop_wifi() {
}

void setup_mqtt() {
    static WiFiClient wificlient;

    mqtt.begin("192.168.1.10", 1883, wificlient);

    while(!mqtt.connect("")) delay(500);
}

void loop_mqtt() {
    mqtt.loop();
}

void setup_software_serial() {
    P1S.begin(115200, SWSERIAL_8N1, RX_PIN, -1, true, 1024);
    P1S.setTimeout(50);
}

void setup_p1_reader() {
    P1R.enable(true);
}

void loop_p1_reader() {
    static unsigned long last_telegram;
    static bool first_telegram;

    unsigned long now = millis();

    P1R.loop();

    if(now - last_telegram > 15000 || first_telegram) {
        P1R.enable(true);
        last_telegram = now;
    }

    if(P1R.available()) {
        MyData data;
        String error;

        if(P1R.parse(&data, &error)) {
            data.applyEach(Printer());
        } else {
            Serial.println(error);
        }
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("Boot");

    setup_wifi();
    setup_mqtt();
    setup_software_serial();
    setup_p1_reader();
}

void loop() {
    loop_wifi();
    loop_mqtt();

    loop_p1_reader();
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
        snprintf(str, sizeof(str), "electricity-consumption-high,room=electrical value=%f", electricity_consumption_high);
        mqtt.publish("/sensor/electricity-consumption-high", str, true);
        Serial.println(electricity_consumption_high);
        // pass
    }

    double electricity_consumption_low;
    if((electricity_consumption_low = find_electricity_consumption_low(circle))) {
        char str[128] = { 0 };
        snprintf(str, sizeof(str), "electricity-consumption-low,room=electrical value=%f", electricity_consumption_low);
        mqtt.publish("/sensor/electricity-consumption-low", str, true);
        Serial.println(electricity_consumption_low);
        // pass
    }

  double electricity_usage;
  if((electricity_usage = find_electricity_usage(circle))) {
    char str[128] = { 0 };
    snprintf(str, sizeof(str), "electricity-usage,room=electrical value=%f", electricity_usage);
    mqtt.publish("/sensor/electricity-usage", str, true);
    Serial.println(electricity_usage);
    // pass
  }

  double gas_consumption;
  if((gas_consumption = find_gas_consumption(circle))) {
    char str[128] = { 0 };
    snprintf(str, sizeof(str), "gas-consumption,room=electrical value=%f", gas_consumption);
    mqtt.publish("/sensor/gas-consumption", str, true);
    Serial.println(gas_consumption);
    // pass
  }
  
  Serial.println("resilient_parse: end");
}
