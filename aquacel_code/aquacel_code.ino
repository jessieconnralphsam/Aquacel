//libraries
#include <SoftwareSerial.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <RTClib.h>
#include <Arduino.h>
//---------------------------------------------------------
//pins
#define RX_PIN 2 //signal pins sa gsm
#define TX_PIN 3 //
//#define DO_PIN A3 // Dissolved Oxygen Pin
const int turbidityPin = A4; //turbidity pin
const int oneWireBus = 12;  //temperature pin
const int phPin = A0;  // acidity pin
float pHValue;// Variable storage sa pH value
//---------------------------------------------------------
//object
SoftwareSerial sim900(RX_PIN, TX_PIN);
OneWire oneWire(oneWireBus); //temperature
DallasTemperature sensors(&oneWire); //temperature

//----------------dissolved oxygen objects ----------------
// #define VREF 5000    
// #define ADC_RES 1024 

// //Single-point calibration Mode=0
// //Two-point calibration Mode=1
// #define TWO_POINT_CALIBRATION 0

// #define READ_TEMP (25) //Current water temperature ℃, Or temperature sensor function --- need to removed!

// //Single point calibration needs to be filled CAL1_V and CAL1_T
// #define CAL1_V (1600) //mv
// #define CAL1_T (25)   //℃
// //Two-point calibration needs to be filled CAL2_V and CAL2_T
// //CAL1 High temperature point, CAL2 Low temperature point
// #define CAL2_V (1300) //mv
// #define CAL2_T (15)   //℃

// const uint16_t DO_Table[41] = {
//     14460, 14220, 13820, 13440, 13090, 12740, 12420, 12110, 11810, 11530,
//     11260, 11010, 10770, 10530, 10300, 10080, 9860, 9660, 9460, 9270,
//     9080, 8900, 8730, 8570, 8410, 8250, 8110, 7960, 7820, 7690,
//     7560, 7430, 7300, 7180, 7070, 6950, 6840, 6730, 6630, 6530, 6410};

// uint8_t Temperaturet;
// uint16_t ADC_Raw;
// uint16_t ADC_Voltage;
// uint16_t DO;

// int16_t readDO(uint32_t voltage_mv, uint8_t temperature_c)
// {
// #if TWO_POINT_CALIBRATION == 0
//   uint16_t V_saturation = (uint32_t)CAL1_V + (uint32_t)35 * temperature_c - (uint32_t)CAL1_T * 35;
//   return (voltage_mv * DO_Table[temperature_c] / V_saturation);
// #else
//   uint16_t V_saturation = (int16_t)((int8_t)temperature_c - CAL2_T) * ((uint16_t)CAL1_V - CAL2_V) / ((uint8_t)CAL1_T - CAL2_T) + CAL2_V;
//   return (voltage_mv * DO_Table[temperature_c] / V_saturation);
// #endif
// }
//---------------------------------------------------------------------------------------------------------------------------------------

void setup() {
  // baud rate
  Serial.begin(9600);
  sensors.begin();
  //-----------------------------------rtc-----------------
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
  if (rtc.lostPower()) {
    Serial.println("RTC lost power, set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  //--------------------------------------------------------
  // SoftwareSerial communication with the SIM900 module
  sim900.begin(9600);
  delay(1000);
  Serial.println("Initializing GSM Module...");
  // checking ...
  if (sendATCommand("AT", "OK")) {
    //gsm module ready ...
    Serial.println("GSM Module is ready.");
  } else {
    //error checking
    Serial.println("Error initializing GSM Module! Check connections and power.");
    while (true);
  }
}
void loop() {
  //-----------------------------RTC-------------------------------------------------
  //enable para aha gamiton
  DateTime now = rtc.now();
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(" ");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();
  delay(1000); //hand shaking
  //---------------------------------------------------------------------------------
  //----------ACIDITY---------------------------------------------------------------
  int sensorValue = analogRead(phPin);
  pHValue = analogToPH(sensorValue);
  Serial.print("pH Value: ");
  Serial.println(pHValue);
  delay(1000); 
  //------------------------------------------------------------------------------------
  //----------------------------------------------------TURBIDITY----------------------
  int turbidityValue = analogRead(turbidityPin); // ginabasa ang analog value gikan sa sensor
  float turbidity = mapTurbidity(turbidityValue); // tapos e insert ang turbidity reading sa sensor didtoa sa function
  Serial.print("Turbidity: ");
  Serial.println(turbidity);
  delay(1000); // delay hand shaking
  //-----------------------------------------------------------------------------------
  //---------------TEMPERATURE----------------------------------------------------------
  sensors.requestTemperatures();
  //temperature in Celsius
  float temperatureCelsius = sensors.getTempCByIndex(0);
  Serial.print("Temperature: ");
  Serial.print(temperatureCelsius);
  Serial.println(" °C");
  delay(1000);  //hand shaking
  //-------------------dissolved oxygen -------------------
  //Temperaturet = temperatureCelsius; //nagkuha og temperature gikan sa reading sa temperature sensor
  // ADC_Raw = analogRead(DO_PIN);
  // ADC_Voltage = uint32_t(VREF) * ADC_Raw / ADC_RES;
  // Serial.print("Temperature:\t" + String(Temperaturet) + "\t");
  // Serial.print("ADC RAW:\t" + String(ADC_Raw) + "\t");
  // Serial.print("ADC Voltage:\t" + String(ADC_Voltage) + "\t");
  // Serial.println("Dissolved Oxygen:\t" + String(readDO(ADC_Voltage, Temperaturet)) + "\t");
  // delay(1000);

  //expected output
  //   Temperature:    gikan sa temperature    ADC RAW:    213    ADC Voltage:    1040    Dissolved Oxygen:    5362
  //------------------------------------------------------
  //-------------------------------------------sending sms------------------------
  // Check if the current time is 1:00 PM (mao ni sa sending og sms gikan check is 1pm na sa rtc)
  if (now.hour() == 13 && now.minute() == 0 && now.second() == 0) {
    // Get sensor readings and create the message
    String message = "pH: " + String(pHValue) + "\nTurbidity: " + String(turbidity) +
                     "\nTemperature: " + String(temperatureCelsius);

    Serial.println(message);

    // butang nato and data sa function
    sendSMS("+1234567890", message); //ilisan og phone number

    // Delay para sa handshaking
    delay(1000);
  }
}
//function for mapping turbidity
float mapTurbidity(int analogValue) {
  float turbidityMin = 0.0; // Minimum value sa turbidity
  float turbidityMax = 100.0; // Maximum  value sa turbidity 
  float analogMin = 0.0; // Minimum analog value gikan sa sensor
  float analogMax = 1023.0; // Maximum analog value gikan sa sensor
  // linear interpolation mapping sa analog value sa turbidity(formula)
  float turbidity = ((analogValue - analogMin) / (analogMax - analogMin)) * (turbidityMax - turbidityMin) + turbidityMin;
  return turbidity;
}

// function para mag send og AT command tapos checking for response ...
bool sendATCommand(String command, String expectedResponse) {
  sim900.println(command);
  delay(500);
  
  String response = readSerial();
  Serial.println(response);

  return response.indexOf(expectedResponse) != -1;
}
// Function para send og SMS message with sensor value
void sendSMS(String phone, String message) {
  // Set SMS mode
  sendATCommand("AT+CMGF=1", "OK");
  // Set phone number
  String setPhone = "AT+CMGS=\"" + phone + "\"";
  sendATCommand(setPhone, ">");
  // Send message
  sim900.println(message);
  // Send Ctrl+Z to indication na end na ang message
  sim900.write(0x1A);
  //hand shaking required ni sya para sa timing
  delay(500);
  // Display response ...
  Serial.println(readSerial());
}
// Function para mag basa sa response from SIM900 module
String readSerial() {
  String response = "";
  while (sim900.available()) {
    response += sim900.read();
  }
  return response;
}
// Function taga convert og analog value to pH
float analogToPH(int analogValue) {
  float slope = 3.5;
  float intercept = -550;
  float pH = slope * analogValue + intercept;
  return pH;
}