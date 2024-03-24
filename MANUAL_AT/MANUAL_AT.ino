#include <SoftwareSerial.h>

const int GSM_RX_PIN = 10;
const int GSM_TX_PIN = 11;

SoftwareSerial gsmSerial(GSM_RX_PIN, GSM_TX_PIN);

void setup() {
  Serial.begin(9600);
  gsmSerial.begin(9600);
}

void loop() {
  if (Serial.available()) {
    char data = Serial.read();
    gsmSerial.write(data);
  }

  if (gsmSerial.available()) {
    char data = gsmSerial.read();
    Serial.write(data);
  }
}
