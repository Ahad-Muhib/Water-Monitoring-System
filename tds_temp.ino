#include <OneWire.h>
#include <DallasTemperature.h>

// ---------- Temperature Sensor Setup ----------
#define ONE_WIRE_BUS 4 // GPIO4 for DS18B20

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// ---------- TDS Sensor Setup ----------
#define TDS_PIN 34  // Analog pin for TDS sensor
#define VREF 3.3    // ESP32 ADC reference voltage
#define ADC_RESOLUTION 4095.0

float temperature = 0.0; // Temperature in °C
float tdsValue = 0.0;    // TDS in ppm

void setup() {
  Serial.begin(115200);
  sensors.begin();

  analogReadResolution(12); // ESP32 ADC resolution
}

void loop() {
  // --- Temperature Reading ---
  sensors.requestTemperatures();
  temperature = sensors.getTempCByIndex(0);

  // --- TDS Reading ---
  int analogValue = analogRead(TDS_PIN);
  float voltage = analogValue * VREF / ADC_RESOLUTION;

  // Example TDS calculation (depends on your sensor calibration)
  tdsValue = (133.42 * voltage * voltage * voltage 
              - 255.86 * voltage * voltage 
              + 857.39 * voltage) 
              * 0.5; // Temperature compensation factor

  // Print results
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print(" °C\t");

  Serial.print("TDS: ");
  Serial.print(tdsValue);
  Serial.println(" ppm");

  delay(2000); // Read every 2 seconds
}
