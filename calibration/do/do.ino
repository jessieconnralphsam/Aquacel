#include <Arduino.h>
 
#define VREF    5000
#define ADC_RES 1024
 
uint32_t raw;
 
void setup()
{
    Serial.begin(115200);
}
 
void loop()
{
    raw=analogRead(A14);
    Serial.println("raw:\t"+String(raw)+"\tVoltage(mv)"+String(raw*VREF/ADC_RES));
    delay(1000);
}