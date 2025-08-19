#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <Servo.h>

// Transmission control messages
#define START_OF_TRANSMISSION 0xFD      // Marker of a start of a transmission
#define END_OF_TRANSMISSION   0xFC      // Marker of an end of a transmission
#define START_MARKER          0xFF      // Marking of the start of a packet
#define END_MARKER            0xFE      // Marking of an end of a packet
#define ACK                   0x06      // Acknowledgement of sent packet
#define NAK                   0x15      // Negative-acknowledgement of sent packet ( retry the measurment )

// Scan accuracy settings
#define N_SAMPLES 5                     // Number of samples taken from a single measurment
#define TURN_VALUE 10                   // Value of the turns ( in degrees ) taken in a single iteration by the device ( in range (0, 180] )
#define SERVO_DELAY 100                 // Value (in ms) of servo movements delay

// Arduino Uno R3 Pinout
const int trigPin = 6;  
const int echoPin = 11; 
const int servo1Pin = 9;
const int servo2Pin = 10;
const int startScanButtonPin = 2;

Servo servo1;
Servo servo2;
LiquidCrystal_I2C lcd(0x27, 16, 2);
const int pos1 = 180, pos2 = 130;
bool turn = true;
float duration, distance, progress, ser2, complete;

void sortFloats(float arr[], int n) {
  for (int i = 0; i < n - 1; i++) {
    for (int j = 0; j < n - i - 1; j++) {
      if (arr[j] > arr[j + 1]) {
        float temp = arr[j];
        arr[j] = arr[j + 1];
        arr[j + 1] = temp;
      }
    }
  }
}

float measureMedian()
{
  int validCount = 0;
  float distances[N_SAMPLES];
  for(int i = 0; i < N_SAMPLES; i++)
  {
    float d = measureDistance();
    if(d >= 0)
    {
      if(d > 200) distances[validCount++] = 201;
      else distances[validCount++] = d;
    }
    delay(60);
  }
  if(validCount == 0) return -1;
  sortFloats(distances, N_SAMPLES);
  return distances[validCount / 2];
}

float measureDistance()
{
  digitalWrite(trigPin, LOW);  
  delayMicroseconds(2);  
  digitalWrite(trigPin, HIGH);  
  delayMicroseconds(10);  
  digitalWrite(trigPin, LOW); 

  return (pulseIn(echoPin, HIGH, 20000) * 0.0343) / 2;
}

bool sendData(int x_rotation, int y_rotation, float distance)
{
  //packets consist of 6 bytes: B5 B4 B3 B2 B1 B0, where:
  //B5 - Start marker (0xFF)
  //B4 - Rotation in x-axis
  //B3 - Rotation in y-axis
  //B2 - Integer part of distance value
  //B1 - Fractional part of distance value
  //B0 - End marker (0xFE)
  //Scanner won't continue work until program in Python sends back the ACK message

  byte x_rot = byte(x_rotation);
  byte y_rot = byte(y_rotation);
  byte distInt = byte(distance);
  byte distFrac = byte(((distance - distInt) * 100));

  Serial.write(START_MARKER);
  Serial.write(x_rot);
  Serial.write(y_rot);
  Serial.write(distInt);
  Serial.write(distFrac);
  Serial.write(END_MARKER);

  while (true) 
  {
    if (Serial.available()) 
    {
      byte response = Serial.read();
      if (response == ACK) return true;
      else if(response == NAK) return false;
    }
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  lcd.init();    
  lcd.backlight();  
  lcd.setCursor(0, 0);
  lcd.print("");

  pinMode(startScanButtonPin, INPUT_PULLUP);

  pinMode(trigPin, OUTPUT);  
	pinMode(echoPin, INPUT);

  servo1.attach(servo1Pin);
  servo2.attach(servo2Pin);

  servo1.write(pos1);
  servo2.write(pos2);
}

void loop() {
  if(digitalRead(2) == LOW)
  {
    ser2 = 0;
    complete = ((180 / TURN_VALUE) + 1) * ((80 / TURN_VALUE) + 1);
    lcd.setCursor(0, 0);
    lcd.print("Start scan...");
    delay(2000);
    lcd.clear();
    lcd.print("Scanning...");

    Serial.write(START_OF_TRANSMISSION);
    Serial.write(TURN_VALUE);

    for(int i = 130; i >= 50; i -= TURN_VALUE)
    {
      servo2.write(i);
      delay(SERVO_DELAY);

      if(turn){
        for(int j = 180; j >= 0; j -= TURN_VALUE)
        {
          servo1.write(j);
          ser2++;
          progress = (ser2 * 100) / complete;
          lcd.setCursor(0, 1);
          lcd.print("             ");
          lcd.setCursor(2, 1);
          lcd.print(String(int(progress)) + "%");
          delay(SERVO_DELAY);
          while(!sendData(j, i, measureMedian()));
        }
        turn = !turn;
      } 
      else
      {
        for(int j = 0; j <= 180; j += TURN_VALUE)
        {
          servo1.write(j);
          ser2++;
          progress = (ser2 * 100) / complete;
          lcd.setCursor(0, 1);
          lcd.print("           ");
          lcd.setCursor(2, 1);
          lcd.print(String(int(progress)) + "%");
          delay(SERVO_DELAY);
          while(!sendData(j, i, measureMedian()));
        }
        turn = !turn;
      }

      Serial.write(END_OF_TRANSMISSION);
    }
    servo1.write(pos1);
    servo2.write(pos2);
    lcd.clear();
    lcd.print("Scan complete.");
    delay(2000);
    lcd.clear();
  }
}
