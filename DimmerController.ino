#include <Arduino.h>
#include <TimerOne.h>
#include <SoftwareSerial.h>
#include <Wire.h>

#define IS_MAIN
#define I2C_ADDRESS 11

//Sync pin is pin3 (interrupt 1)

uint8_t pins[8] {11, 10, 9, 8, 7, 6, 5, 4};
uint8_t values[8];

uint8_t clock_tick; // variable for Timer1

String buffer;
int checkIndex = 0;


// Krida code
void set(int index, float value, bool passIfOverflow = false)
{
  if (index >= 1 && index <= 8)
  {
    //Serial.println("Set " + String(index) + " to " + String(value));
    values[index - 1] = map(constrain(value, 0.f, 1.f) * 100, 0, 100, 90, 5);
    Serial.println(String(index)+" "+values[index-1]);
  } else if (index > 8 && passIfOverflow)
  {
    Serial.println("pass to next board with index " + String(index - 8));
//    Wire.beginTransmission(I2C_ADDRESS);
//
//    byte* v = (byte*)&value;
//    Wire.write((byte)index-8);
//    Wire.write(v, 4);
//    Wire.endTransmission();
  }
}

void processMessage(const String &buffer)
{
  if (int i = buffer.indexOf(" "))
  {
    int id = buffer.substring(0, i).toInt();
    float val = buffer.substring(i + 1).toFloat();

    set(id, val, true);
  }
}

void processSerial()
{
  while (Serial.available())
  {
    char c = Serial.read();

    if (c == '\n')
    {

      processMessage(buffer);

      buffer = "";
    }
    else
    {
      buffer += c;
    }
  }
}

void receiveEvents(int numBytes)
{
  buffer = "";
  Serial.println("Received " + String(numBytes));
  byte index = Wire.read();
  
  union u_tag {
      byte b[4];
      float val;
    } data;

  for(int i=0;i<4;i++) data.b[i] = Wire.read();
  
  Serial.println(" > " + String(index)+" : "+String(data.val));
  set(index, data.val);
}



void interruptCallback()
{
  clock_tick = 0;
  //Serial.println(String(millis()) + " Clock " + String(clock_tick));
}

void isrCallback()
{
  //clock_tick++;
  //Serial.println(String(millis()) + " ISR_Clock " + String(clock_tick));
  for (int i = 0; i < 8; i++)
  {
    // Serial.println(String(values[i])+" / "+String(clock_tick));
    if (values[i] == clock_tick)
    {
      digitalWrite(pins[i], HIGH); // triac firing
      delayMicroseconds(8.33);				   // triac On propogation delay (for 60Hz use 8.33)
      digitalWrite(pins[i], LOW);  // triac Off
    }
  }
}

void setup()
{
  Serial.begin(115200);
  
  delay(100);
  Serial.println("Dimmer Control !");
  
  #ifdef IS_MAIN
  Serial.println("Master");
  //Wire.begin();
  #else
  //Wire.begin(I2C_ADDRESS);
  #endif
  //Wire.onReceive(receiveEvents);
  
  for (int i = 0; i < 8; i++)
  {
    pinMode(pins[i], OUTPUT); // Set AC Load pin as output
    values[i] = 80;
  }

  attachInterrupt(1, interruptCallback, RISING);

  Timer1.initialize(83);				 // set a timer of length 100 microseconds for 50Hz or 83 microseconds for 60Hz;
  Timer1.attachInterrupt(isrCallback); // attach the service routine here
}

void loop()
{
  processSerial();
  //Serial.println(checkIndex++);
}
