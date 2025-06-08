/*

Used Bresser Protocol from https://github.com/merbanan/rtl_433_tests/tree/master/tests/bresser_3ch
Replaces this sensor: https://www.bresser.de/Wetter-Zeit/BRESSER-Thermo-Hygro-Sensor-3CH-passend-fuer-BRESSER-Thermo-Hygrometer.html
Tested with DHT22 Sensor and generic 433Mhz chip on an Arduino Nano clone
Library for DHT22 sensor: https://github.com/adafruit/DHT-sensor-library

*/

#include <DHT.h>
#include <DHT_U.h>
#include <Adafruit_SleepyDog.h> // Watchdog sleep timer

byte SEND_PIN = 2;  //overwritten in sendbit by a direct register operation

#define DHTPIN 3
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

float humCalibration = -5;    // % Percentage offset
float tempCalibration = -5.1; // % Percentage offset

float tempCalibrationConstant;
float humCalibrationConstant;

int periodusec = 250;
byte repeats = 15;
byte randomId = 252;
unsigned long sendPeriod = 57000; // tipical period for Channel 1 (57000)

#define DEBUG true

void setup()
{
  pinMode(SEND_PIN, OUTPUT);
  pinMode(DHTPIN, INPUT_PULLUP);

  if (DEBUG)
  {
    Serial.begin(9600);
    Serial.println(F("Build 2025-06-08 - work in progess"));
    Serial.println(F("DHT22 Test!"));
  }

  dht.begin();

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  // Initialize calibration module values
  tempCalibrationConstant = initCalibration(tempCalibration);
  humCalibrationConstant = initCalibration(humCalibration);
}

void loop()
{
  delay(100);
  float h = dht.readHumidity();
  float f = dht.readTemperature(true);
  float t = dht.readTemperature();

  t = calculateCalibratedValue(t, tempCalibrationConstant, tempCalibration);
  f = calculateCalibratedValue(f, tempCalibrationConstant, tempCalibration);
  h = calculateCalibratedValue(h, humCalibrationConstant, humCalibration);

  if (DEBUG)
  {
    if (isnan(h) || isnan(t) || isnan(f))
    {
      Serial.println(F("Failed to read from DHT sensor!"));
      return;
    }

    Serial.print(F("Humidity: "));
    Serial.print(h);
    Serial.print(F("%  Temperature: "));
    Serial.print(t);
    Serial.print(F("°C  -  "));
    Serial.print(f);
    Serial.print(F("°F "));
    Serial.println();
  }

  int Temperature = (int)((f + 90) * 10);
  int Humidity = (int)h;

  sendData(Temperature, Humidity);

  // Shutdown built in led when enter in sleep state
  digitalWrite(LED_BUILTIN, LOW);

  // Activate sleepstate for (sendPeriod)
  Watchdog.sleep(sendPeriod/2);   //not good for long periods >30sec
  Watchdog.sleep(sendPeriod/2);
  //delay(sendPeriod);

  // Lit up builtin led signaling that the divice is back to active mode
  digitalWrite(LED_BUILTIN, HIGH);
}

void sendData(int Temperature, int Humidity)
{
  for (int i = repeats; i >= 0; i--)
  {
    // Send Preambel Bits
    sendPraBits();

    // Calculate Checksum
    int checkSum = 0;
    checkSum += randomId;

    int byte2 = B0001;
    byte2 = byte2 << 4;
    byte2 = byte2 | (Temperature >> 8);
    checkSum += byte2;

    int byte3 = Temperature & 0xFF;
    checkSum += byte3;

    checkSum += Humidity;

    // Send Address
    send8Bit(randomId);

    // Battery 0 = good
    // Sync/Test Bit 0 = OK
    // Channel 1=01, 2=10, 3=11
    send4Bit(B0001);

    // Send Temperature
    sendTemp(Temperature);

    // Send Humidity
    send8Bit(Humidity);

    // Send Checksum
    send8Bit(checkSum % 256);
  }
}

void send8Bit(int address)
{
  for (int i = 7; i >= 0; i--)
  {
    boolean curBit = address & 1 << i;
    sendBit(curBit);
  }
}

void send4Bit(int address)
{
  for (int i = 3; i >= 0; i--)
  {
    boolean curBit = address & 1 << i;
    sendBit(curBit);
  }
}

void sendTemp(int address)
{
  for (int i = 11; i >= 0; i--)
  {
    boolean curBit = address & 1 << i;
    sendBit(curBit);
  }
}

void sendBit(boolean isBitOne)
{
  if (isBitOne)
  {
    // Send '1'
    PORTD = PORTD | B00000100;
    // digitalWrite(SEND_PIN, HIGH);
    delayMicroseconds(periodusec * 2);
    PORTD = PORTD & B11111011;
    // digitalWrite(SEND_PIN, LOW);
    delayMicroseconds(periodusec);
  }
  else
  {
    // Send '0'
    PORTD = PORTD | B00000100;
    // digitalWrite(SEND_PIN, HIGH);
    delayMicroseconds(periodusec);
    PORTD = PORTD & B11111011;
    // digitalWrite(SEND_PIN, LOW);
    delayMicroseconds(periodusec * 2);
  }
}

void sendPraBits()
{
  for (int i = 0; i < 4; i++)
  {
    PORTD = PORTD | B00000100;
    // digitalWrite(SEND_PIN, HIGH);
    delayMicroseconds(750);
    PORTD = PORTD & B11111011;
    // digitalWrite(SEND_PIN, LOW);
    delayMicroseconds(750);
  }
}

float initCalibration(float percentSkew)
{
  float j;
  if (percentSkew < 0)
  {
    percentSkew = abs(percentSkew);
    j = (1 + (percentSkew / 100));
    return j;
  }
  else
  {
    j = (1 + (percentSkew / 100));
    return j;
  }
}

float calculateCalibratedValue(float reading, float correction, float skew)
{
  float j;
  if (skew < 0)
  {
    j = reading / correction;
    return j;
  }
  else
  {
    j = reading * correction;
    return j;
  }
}