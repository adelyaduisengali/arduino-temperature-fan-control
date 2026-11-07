// Temperature-based fan speed control with adjustable setpoint
// TMP36 sensor + L293D motor driver + DC fan
// Features:
//  - adjustable temperature setpoint (potentiometer)
//  - hysteresis to avoid rapid on/off switching
//  - proportional fan speed
//  - moving-average filter for sensor noise
//  - LED status indication (normal / cooling / overheat)
//  - serial monitoring

const int fanPin    = 9;   // PWM -> L293D Enable 1
const int tempPin   = A0;  // TMP36 output
const int setPin    = A1;  // potentiometer (setpoint)
const int greenLed  = 4;   // normal
const int yellowLed = 5;   // fan running
const int redLed    = 6;   // overheat

const float HYSTERESIS = 2.0;   // °C below setpoint to switch fan off
const float RANGE      = 10.0;  // °C above setpoint to reach full speed

// Moving-average filter
const int N = 10;
float samples[N];
int idx = 0;
bool filled = false;

bool fanOn = false;

float readTemperature() {
  int reading = analogRead(tempPin);
  float voltage = reading * (5.0 / 1024.0);
  return (voltage - 0.5) * 100.0;
}

float filteredTemperature() {
  samples[idx] = readTemperature();
  idx = (idx + 1) % N;
  if (idx == 0) filled = true;

  int count = filled ? N : idx;
  float sum = 0;
  for (int i = 0; i < count; i++) sum += samples[i];
  return sum / count;
}

void setup() {
  pinMode(fanPin, OUTPUT);
  pinMode(greenLed, OUTPUT);
  pinMode(yellowLed, OUTPUT);
  pinMode(redLed, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  float temp = filteredTemperature();

  // Setpoint from potentiometer: 20..35 °C
  float setpoint = map(analogRead(setPin), 0, 1023, 200, 350) / 10.0;

  // Hysteresis control
  if (!fanOn && temp > setpoint)              fanOn = true;
  if (fanOn  && temp < setpoint - HYSTERESIS) fanOn = false;

  // Proportional speed
  int speed = 0;
  if (fanOn) {
    float ratio = (temp - (setpoint - HYSTERESIS)) / (RANGE + HYSTERESIS);
    ratio = constrain(ratio, 0.0, 1.0);
    speed = 100 + ratio * 155;   // 100..255
  }
  analogWrite(fanPin, speed);

  // Status LEDs
  bool overheat = temp > setpoint + RANGE;
  digitalWrite(greenLed,  !fanOn);
  digitalWrite(yellowLed, fanOn && !overheat);
  digitalWrite(redLed,    overheat);

  // Monitoring
  Serial.print("Temp: ");      Serial.print(temp, 1);
  Serial.print(" C | Set: ");  Serial.print(setpoint, 1);
  Serial.print(" C | Fan: ");  Serial.print(fanOn ? "ON " : "OFF");
  Serial.print(" | Speed: ");  Serial.print(map(speed, 0, 255, 0, 100));
  Serial.print(" %");
  if (overheat) Serial.print(" | OVERHEAT!");
  Serial.println();

  delay(200);
}
