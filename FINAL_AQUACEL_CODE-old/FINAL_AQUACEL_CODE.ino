#include <SoftwareSerial.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ThreeWire.h>
#include <RtcDS1302.h>

#define SIM800_TX_PIN 11
#define SIM800_RX_PIN 10
#define SENSOR_PIN 7
#define TURBIDITY_SENSOR_PIN A0

SoftwareSerial sim800(SIM800_TX_PIN, SIM800_RX_PIN);
OneWire oneWire(SENSOR_PIN);
DallasTemperature tempSensor(&oneWire);

ThreeWire myWire(4,5,2);
RtcDS1302<ThreeWire> Rtc(myWire);

float tempCelsius;
float tempFahrenheit;
int turbidityValue;

void setup() {
  Serial.begin(9600);
  sim800.begin(9600);
  tempSensor.begin();
  Rtc.Begin();

  Serial.println("Sending data to +639934387196");
}

void loop() {
  // Get and display current time
  RtcDateTime now = Rtc.GetDateTime();
  printDateTime(now);
  Serial.println();

  // Get and display temperature
  tempSensor.requestTemperatures();
  tempCelsius = tempSensor.getTempCByIndex(0);
  tempFahrenheit = tempCelsius * 9 / 5 + 32;

  Serial.print("Temperature: ");
  Serial.print(tempCelsius);
  Serial.print("°C");
  Serial.print("  ~  ");
  Serial.print(tempFahrenheit);
  Serial.println("°F");

  // Read turbidity sensor value
  turbidityValue = analogRead(TURBIDITY_SENSOR_PIN);

  // Print turbidity sensor value
  Serial.print("Turbidity Value: ");
  Serial.println(turbidityValue);

  // Send temperature and turbidity via SMS
  String message = "Temperature: " + String(tempCelsius) + "°C, Turbidity: " + String(turbidityValue);

  sim800.println("AT+CMGF=1");
  delay(1000);
  sim800.println("AT+CMGS=\"sample number here\"");
  delay(1000);
  sim800.print(message);
  delay(1000);
  sim800.write(26);
  delay(1000);
  if (sim800.find("OK")) {
    Serial.println("Message sent successfully!");
  } else {
    Serial.println("Failed to send message.");
  }

  delay(5000);
}

#define countof(a) (sizeof(a) / sizeof(a[0]))

void printDateTime(const RtcDateTime& dt)
{
    char datestring[20];
    uint8_t hour = dt.Hour();
    bool is_pm = false;

    if (hour == 0) {
        hour = 12;
        is_pm = false;
    } else if (hour == 12) {
        hour = 12;
        is_pm = true;
    } else if (hour > 12) {
        hour -= 12;
        is_pm = true;
    } else {
        is_pm = false;
    }

    snprintf_P(datestring, 
            countof(datestring),
            PSTR("%02u/%02u/%04u %02u:%02u:%02u %s"),
            dt.Month(),
            dt.Day(),
            dt.Year(),
            hour,
            dt.Minute(),
            dt.Second(),
            is_pm ? "PM" : "AM");
    Serial.print(datestring);
}
