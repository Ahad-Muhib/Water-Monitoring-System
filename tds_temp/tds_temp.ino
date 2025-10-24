/***************************************************
  ESP32 Water Quality Monitoring with Blynk Alerts
  Sensors: DS18B20, TDS, Turbidity
  Display: 16x2 I2C LCD
****************************************************/

#define BLYNK_TEMPLATE_ID "TMPL6MiR0o_Yw"
#define BLYNK_TEMPLATE_NAME "Water Quality Monitor"
#define BLYNK_AUTH_TOKEN "W-niNoDaI1krWxRxFGSnYNvS03CKug-J" 

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ---------- Wi-Fi ----------
char ssid[] = "Boogeyman";      // <-- Replace with your Wi-Fi
char pass[] = "notforyou";  // <-- Replace with your Wi-Fi

// ---------- Temperature Sensor ----------
#define ONE_WIRE_BUS 4
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// ---------- TDS Sensor ----------
#define TDS_PIN 34
#define VREF 3.3
#define ADC_RESOLUTION 4095.0

// ---------- Turbidity Sensor ----------
#define TURBIDITY_PIN 32
#define NUM_SAMPLES 10

// ---------- LCD ----------
LiquidCrystal_I2C lcd(0x27, 16, 2); // Change to 0x3F if your LCD uses that address

// ---------- Variables ----------
float temperature = 0.0;
float tdsValue = 0.0;
float turbidity = 0.0;

// ---------- Alert System ----------
unsigned long lastAlertTime = 0;
const unsigned long alertInterval = 60000; // 1 minute between alerts

// ---------- Function: Average Turbidity Voltage ----------
float readAverageTurbidityVoltage() {
  float sumVoltage = 0.0;
  for (int i = 0; i < NUM_SAMPLES; i++) {
    int analogValue = analogRead(TURBIDITY_PIN);
    float voltage = analogValue * VREF / ADC_RESOLUTION;
    sumVoltage += voltage;
    delay(50);
  }
  return sumVoltage / NUM_SAMPLES;
}

// ---------- Setup ----------
void setup() {
  Serial.begin(115200);
  Serial.println("Starting setup...");

  sensors.begin();
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Initializing...");
  delay(2000);
  lcd.clear();

  // ---------- Wi-Fi ----------
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, pass);
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 20) {
    delay(500);
    Serial.print(".");
    retries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n❌ WiFi connection failed!");
  }

  // ---------- Blynk ----------
  Blynk.config(BLYNK_AUTH_TOKEN);
  if (Blynk.connect()) {
    Serial.println("✅ Connected to Blynk Cloud!");
  } else {
    Serial.println("❌ Failed to connect to Blynk!");
  }
}

// ---------- Loop ----------
void loop() {
  Blynk.run();

  // --- Temperature ---
  sensors.requestTemperatures();
  temperature = sensors.getTempCByIndex(0);

  // --- TDS ---
  int tdsAnalog = analogRead(TDS_PIN);
  float tdsVoltage = tdsAnalog * VREF / ADC_RESOLUTION;
  tdsValue = (133.42 * pow(tdsVoltage, 3) 
              - 255.86 * pow(tdsVoltage, 2) 
              + 857.39 * tdsVoltage) * 0.5;

  // --- Turbidity ---
  float avgTurbidityVoltage = readAverageTurbidityVoltage();

  if (avgTurbidityVoltage < 1.0) {
    turbidity = -1; // No water
  } else {
    turbidity = map(avgTurbidityVoltage * 1000, 1000, 3000, 500, 0);
    if (turbidity < 0) turbidity = 0;
  }

  // --- Serial Output ---
  Serial.print("Temp: "); Serial.print(temperature); Serial.print(" °C\t");
  Serial.print("TDS: "); Serial.print(tdsValue); Serial.print(" ppm\t");
  Serial.print("Turbidity: "); 
  if (turbidity == -1) Serial.print("No water");
  else Serial.print(turbidity); 
  Serial.print(" NTU\tVoltage: ");
  Serial.print(avgTurbidityVoltage);
  Serial.println(" V");

  // --- LCD Output ---
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("T:"); lcd.print(temperature, 1); lcd.print("C ");
  lcd.print("TDS:"); lcd.print((int)tdsValue);

  lcd.setCursor(0, 1);
  lcd.print("Turb:");
  if (turbidity == -1) lcd.print("No water   ");
  else { lcd.print((int)turbidity); lcd.print(" NTU"); }

  // --- Blynk Virtual Write ---
  Blynk.virtualWrite(V0, temperature);
  Blynk.virtualWrite(V1, tdsValue);
  Blynk.virtualWrite(V2, turbidity);

  // --- Alert System (1-minute gap) ---
  if (millis() - lastAlertTime > alertInterval) {
    if (temperature > 40) {
      Blynk.logEvent("high_temp", "⚠️ High Temperature Detected!");
      lastAlertTime = millis();
    }
    else if (tdsValue > 500) {
      Blynk.logEvent("high_tds", "⚠️ High TDS Level - Water May Be Impure!");
      lastAlertTime = millis();
    }
    else if (turbidity > 300) {
      Blynk.logEvent("high_turbidity", "⚠️ High Turbidity - Water Not Clear!");
      lastAlertTime = millis();
    }
  }

  delay(2000);
}

