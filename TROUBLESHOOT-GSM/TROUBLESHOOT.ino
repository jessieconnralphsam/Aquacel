#include <SoftwareSerial.h>

#define SIM800_TX_PIN 11 
#define SIM800_RX_PIN 10 

SoftwareSerial sim800(SIM800_TX_PIN, SIM800_RX_PIN);

void setup() {
  Serial.begin(9600);
  sim800.begin(9600);

  Serial.println("Enter AT commands:");
}

void loop() {
  if (Serial.available()) {
    sim800.write(Serial.read());
  }

  if (sim800.available()) {
    Serial.write(sim800.read());
  }
}
