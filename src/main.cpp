#include <Arduino.h>
#include <WiegandMega2560.h>




#define FALSE 0			
#define TRUE  1	


WIEGAND wg;

void setup() {
	Serial.begin(9600);  
	wg.begin(TRUE, TRUE, FALSE);  // wg.begin(GateA , GateB, GateC)
}

void loop() {
	if(wg.available())
	{
		Serial.print("RFID # = ");
		Serial.println(wg.getCode());
        delay(1000);
    }   
    Serial.println("Waiting");
    delay(50);
} 