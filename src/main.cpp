#include <ESP8266WiFi.h>
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
        Serial.println("Reading a new telegram.");
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