#include "DHT.h"

#define DHTPIN 4
#define DHTTYPE DHT11
#define GREEN_LED 18
#define RED_LED 19

const float TEMP_LIMIT = 32.0;
const int FAIL_LIMIT = 3;
const int HIGH_TEMP_LIMIT = 3;
const int RECOVERY_LIMIT = 3;

const unsigned long READ_INTERVAL = 2000;
const unsigned long BLINK_INTERVAL = 250;

DHT dht(DHTPIN, DHTTYPE);

int failCount = 0;
int highTempCount = 0;
int recoveryCount = 0;
int goodReadings = 0;

bool sensorFault = false;
bool temperatureFault = false;
bool redState = false;
bool previousFault = false;

unsigned long lastRead = 0;
unsigned long lastBlink = 0;
unsigned long systemStart = 0;
unsigned long faultStart = 0;
unsigned long lastFaultDuration = 0;
unsigned long totalFaultDuration = 0;
unsigned long faultEvents = 0;

void setup() {
  Serial.begin(115200);
  dht.begin();

  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);

  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);

  systemStart = millis();

  Serial.println("\n==============================");
  Serial.println(" SELF-DIAGNOSING EDGE DEVICE");
  Serial.println("==============================");
  Serial.println("Temperature limit: 32 C");
  Serial.println("Monitoring started");
}

void loop() {
  unsigned long now = millis();

  if (now - lastRead >= READ_INTERVAL) {
    lastRead = now;

    // Measure sensor read execution time
    unsigned long readStart = micros();

    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    unsigned long readDuration = micros() - readStart;

    Serial.println("\n--- Diagnostic Report ---");

    // System uptime
    Serial.print("System uptime: ");
    Serial.print((now - systemStart) / 1000);
    Serial.println(" seconds");

    // Sensor read duration
    Serial.print("Sensor read duration: ");
    Serial.print(readDuration);
    Serial.println(" microseconds");

    // Sensor communication fault detection
    if (isnan(temperature) || isnan(humidity)) {
      failCount++;
      goodReadings = 0;
      recoveryCount = 0;
      highTempCount = 0;

      Serial.print("Consecutive sensor failures: ");
      Serial.println(failCount);

      if (failCount >= FAIL_LIMIT) {
        sensorFault = true;
      }
    } else {
      failCount = 0;

      Serial.print("Temperature: ");
      Serial.print(temperature);
      Serial.println(" C");

      Serial.print("Humidity: ");
      Serial.print(humidity);
      Serial.println(" %");

      // High temperature: 3 consecutive readings above 32 C
      if (temperature > TEMP_LIMIT) {
        highTempCount++;
        recoveryCount = 0;

        Serial.print("High-temperature count: ");
        Serial.println(highTempCount);

        if (highTempCount >= HIGH_TEMP_LIMIT) {
          temperatureFault = true;
        }
      } else {
        highTempCount = 0;

        // Clear temperature fault after 3 safe readings
        if (temperatureFault) {
          recoveryCount++;

          if (recoveryCount >= RECOVERY_LIMIT) {
            temperatureFault = false;
            recoveryCount = 0;
            Serial.println("Temperature fault recovered.");
          }
        }
      }

      // Clear sensor fault after 3 successful readings
      if (sensorFault) {
        goodReadings++;

        if (goodReadings >= RECOVERY_LIMIT) {
          sensorFault = false;
          goodReadings = 0;
          Serial.println("Sensor fault recovered.");
        }
      } else {
        goodReadings = 0;
      }
    }

    // Combined fault state
    bool faultNow = sensorFault || temperatureFault;

    // Start a new fault event
    if (faultNow && !previousFault) {
      faultStart = millis();
      faultEvents++;
      Serial.println("FAULT EVENT STARTED");
    }

    // End a fault event
    if (!faultNow && previousFault) {
      lastFaultDuration = millis() - faultStart;
      totalFaultDuration += lastFaultDuration;

      Serial.print("Last fault duration: ");
      Serial.print(lastFaultDuration / 1000.0, 2);
      Serial.println(" seconds");

      Serial.print("Total fault duration: ");
      Serial.print(totalFaultDuration / 1000.0, 2);
      Serial.println(" seconds");

      Serial.println("FAULT EVENT ENDED");
    }

    previousFault = faultNow;

    Serial.print("Sensor fault: ");
    Serial.println(sensorFault ? "YES" : "NO");

    Serial.print("Temperature fault: ");
    Serial.println(temperatureFault ? "YES" : "NO");

    Serial.print("Total fault events: ");
    Serial.println(faultEvents);

    // Show current fault duration while fault is active
    if (faultNow) {
      Serial.print("Current fault duration: ");
      Serial.print((millis() - faultStart) / 1000.0, 2);
      Serial.println(" seconds");
    }

    Serial.println("------------------------------");
  }

  // LED indicators
  bool fault = sensorFault || temperatureFault;

  if (fault) {
    digitalWrite(GREEN_LED, LOW);

    if (now - lastBlink >= BLINK_INTERVAL) {
      lastBlink = now;
      redState = !redState;
      digitalWrite(RED_LED, redState);
    }
  } else {
    digitalWrite(RED_LED, LOW);
    digitalWrite(GREEN_LED, HIGH);
    redState = false;
  }
}