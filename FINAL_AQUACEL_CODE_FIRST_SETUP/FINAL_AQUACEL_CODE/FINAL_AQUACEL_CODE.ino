#include <Arduino.h>
#include <SoftwareSerial.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ThreeWire.h>
#include <RtcDS1302.h>
#include <Wire.h>

#define SENSOR_PIN 7
#define DO_PIN A14
#define PH_SENSOR_PIN A8

// #define GSM_TX_PIN 11
// #define GSM_RX_PIN 10
// SoftwareSerial sim800(GSM_TX_PIN, GSM_RX_PIN);
SoftwareSerial gsmSerial(10, 11);

OneWire oneWire(SENSOR_PIN);
DallasTemperature tempSensor(&oneWire);

ThreeWire myWire(4, 5, 2);
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

float calibration_value = 21.34 - 0.7;
unsigned long int avgval;
int buffer_arr[10], temp;
float ph_act;

void setup() {
  Serial.begin(9600);
  gsmSerial.begin(9600);
  delay(3000);
  tempSensor.begin();
  Rtc.Begin();
  Wire.begin();

  Serial.println("Sending data...");
}

void loop() {
  String phoneNumbers[] = {"+639667398225", "+639539786598", "+639857951346"};

  RtcDateTime now = Rtc.GetDateTime();
  printDateTime(now);
  Serial.println();

  tempSensor.requestTemperatures();
  tempCelsius = tempSensor.getTempCByIndex(0);
  tempFahrenheit = tempCelsius * 9 / 5 + 32;

  Serial.print("Temperature: ");
  Serial.print(tempCelsius);
  Serial.print("°C");
  Serial.print("  ~  ");
  Serial.print(tempFahrenheit);
  Serial.println("°F");

  Temperaturet = (uint8_t)READ_TEMP;
  ADC_Raw = analogRead(DO_PIN);
  ADC_Voltage = (uint32_t)VREF * ADC_Raw / ADC_RES;
  DO = readDO(ADC_Voltage, Temperaturet) / 1000.0;

  Serial.print("Dissolved Oxygen: ");
  Serial.print(DO);
  Serial.println(" mg/L");

  int sensorValue = analogRead(A0);
  float voltage = sensorValue * (5.0 / 1024.0);
  float turbidity = mapVoltageToTurbidity(voltage);

  Serial.print("Turbidity (NTU): ");
  Serial.println(turbidity);

  measurePH();

  String message = "TEMPERATURE: " + String(tempCelsius) + " Degrees Celsius "+ "\n" + "DISSOLVED OXYGEN: " + String(DO) + " mg/L" + "\n" + "TURBIDITY: " + String(turbidity) + " NTU" +"\n"+ "pH: " + String(ph_act);

  sendSMS("+639667398225", message);
  delay(3000);
  sendSMS("+639955575982", message);
  delay(3000);
  sendSMS("+639857951346", message);
  delay(1000*60*5);
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

void measurePH() {
  for (int i = 0; i < 10; i++) {
    buffer_arr[i] = analogRead(PH_SENSOR_PIN);
    delay(30);
  }

  for (int i = 0; i < 9; i++) {
    for (int j = i + 1; j < 10; j++) {
      if (buffer_arr[i] > buffer_arr[j]) {
        temp = buffer_arr[i];
        buffer_arr[i] = buffer_arr[j];
        buffer_arr[j] = temp;
      }
    }
  }

  avgval = 0;
  for (int i = 2; i < 8; i++)
    avgval += buffer_arr[i];
  float volt = (float)avgval * 5.0 / 1024 / 6;
  ph_act = -5.70 * volt + calibration_value;

  Serial.print("pH Value: ");
  Serial.println(ph_act);
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

void sendSMS(String phoneNumber, String message) {
  gsmSerial.println("AT+CMGF=1");

  delay(1000);


  gsmSerial.print("AT+CMGS=\"");
  gsmSerial.print(phoneNumber);
  gsmSerial.println("\"");

  delay(1000);


  gsmSerial.print(message);

  delay(1000);


  gsmSerial.println((char)26);

  delay(1000);


  while (gsmSerial.available()) {
    Serial.write(gsmSerial.read());
  }
}
