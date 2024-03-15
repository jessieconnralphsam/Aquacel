#include <Arduino.h>
#include <SoftwareSerial.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ThreeWire.h>
#include <RtcDS1302.h>

#define SIM800_TX_PIN 10
#define SIM800_RX_PIN 11
#define SENSOR_PIN 7
#define DO_PIN A14

SoftwareSerial sim800(SIM800_TX_PIN, SIM800_RX_PIN);
OneWire oneWire(SENSOR_PIN);
DallasTemperature tempSensor(&oneWire);

ThreeWire myWire(4,5,2);
RtcDS1302<ThreeWire> Rtc(myWire);

#define VREF 5000    //VREF (mv)
#define ADC_RES 1024 //ADC Resolution

#define READ_TEMP (25) //Current water temperature ℃, Or temperature sensor function

//Single point calibration needs to be filled CAL1_V and CAL1_T
#define CAL1_V (1074) //mv
#define CAL1_T (25)   //℃
//Two-point calibration needs to be filled CAL2_V and CAL2_T
//CAL1 High temperature point, CAL2 Low temperature point
#define CAL2_V (1300) //mv
#define CAL2_T (15)   //℃

const uint16_t DO_Table[41] = {
    14460, 14220, 13820, 13440, 13090, 12740, 12420, 12110, 11810, 11530,
    11260, 11010, 10770, 10530, 10300, 10080, 9860, 9660, 9460, 9270,
    9080, 8900, 8730, 8570, 8410, 8250, 8110, 7960, 7820, 7690,
    7560, 7430, 7300, 7180, 7070, 6950, 6840, 6730, 6630, 6530, 6410};

float tempCelsius;
float tempFahrenheit;
uint8_t Temperaturet;
uint16_t ADC_Raw;
uint16_t ADC_Voltage;
uint16_t DO;

void setup() {
  Serial.begin(9600);
  sim800.begin(9600);
  tempSensor.begin();
  Rtc.Begin();

  Serial.println("Sending data...");
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

  // Read dissolved oxygen (DO)
  Temperaturet = (uint8_t)READ_TEMP;
  ADC_Raw = analogRead(DO_PIN);
  ADC_Voltage = (uint32_t)VREF * ADC_Raw / ADC_RES;
  DO = readDO(ADC_Voltage, Temperaturet) / 1000.0;

  // Print DO
  Serial.print("Dissolved Oxygen: ");
  Serial.print(DO);
  Serial.println(" mg/L");

  // Measure turbidity
  int sensorValue = analogRead(A0);
  float voltage = sensorValue * (5.0 / 1024.0);
  float turbidity = mapVoltageToTurbidity(voltage);

  // Print turbidity
  Serial.print("Turbidity (NTU): ");
  Serial.println(turbidity);

  // Send data via SMS
  String message = "TEMPERATURE: " + String(tempCelsius) + " Degrees Celsius, DISSOLVED OXYGEN: " + String(DO) + " mg/L, TURBIDITY: " + String(turbidity) + " NTU";

  sim800.println("AT+CMGF=1");
  delay(1000);
  sim800.println("AT+CMGS=\"+639539365860\"");
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

int16_t readDO(uint32_t voltage_mv, uint8_t temperature_c) {
#if TWO_POINT_CALIBRATION == 0
  uint16_t V_saturation = (uint32_t)CAL1_V + (uint32_t)35 * temperature_c - (uint32_t)CAL1_T * 35;
  return (voltage_mv * DO_Table[temperature_c] / V_saturation);
#else
  uint16_t V_saturation = (int16_t)((int8_t)temperature_c - CAL2_T) * ((uint16_t)CAL1_V - CAL2_V) / ((uint8_t)CAL1_T - CAL2_T) + CAL2_V;
  return (voltage_mv * DO_Table[temperature_c] / V_saturation);
#endif
}

float mapVoltageToTurbidity(float voltage) {
  float m = 10.0;
  float b = 5.0;
  float turbidity = m * voltage + b;
  return turbidity;
}

void printDateTime(const RtcDateTime& dt) {
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
             sizeof(datestring),
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
