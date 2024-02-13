// BlueTooth_Test_softwareSerial

#include <SoftwareSerial.h>

#define RXPin 2
#define TXPin 3

SoftwareSerial BlueToothSerial(RXPin, TXPin);

char Serial_data, BlueToothSerial_data;
void setup()
{
  Serial.begin(9600);
  BlueToothSerial.begin(9600);
}

void loop()
{
  if(Serial.available())
  {
    Serial_data = Serial.read();
    Serial.write(Serial_data);
    BlueToothSerial.write(Serial_data);
  }
  if(BlueToothSerial.available())
  {
    BlueToothSerial_data = BlueToothSerial.read();
    Serial.write(BlueToothSerial_data);
  }
}


