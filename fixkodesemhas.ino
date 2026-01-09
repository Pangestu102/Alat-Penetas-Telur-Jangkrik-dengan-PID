#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <RBDdimmer.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// === KONFIGURASI WiFi ===
const char* ssid = "Keluarga_cemara";
const char* password = "06011112";

// === ThingSpeak ===
String apiKey = "XH3F3SSIPXYUTOM6";
const char* server = "http://184.106.153.149/update";

// === PIN ===
#define DHTPIN 4
#define DHTTYPE DHT22
#define ZC_PIN 25
#define DIMMER_PIN 26
#define RELAY_FAN 27
#define RELAY_MIST 14

// === OBJEK ===
DHT dht(DHTPIN, DHTTYPE);
dimmerLamp dimmer(DIMMER_PIN, ZC_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// === PID PARAMETER (Cohen–Coon) ===
double Kp = 62.6779;
double Ki = 0.4247;
double Kd = 1575;

double setpoint = 30.00;
double input, output;
double integral = 0, lastError = 0;
unsigned long lastTime = 0;

// === OFFSET KALIBRASI ===
const float suhu_offset = -0.20;
const float hum_offset  = -7.20;

// === STATUS ===
float kelembapan = 0;
bool mistStatus = false;
bool fanStatus  = false;

// === TIMER ===
unsigned long lastSendTime   = 0;
unsigned long lastLCDUpdate  = 0;
unsigned long lastDimmerUpdate = 0;

const unsigned long sendInterval = 20000;

// === BUFFER OFFLINE ===
struct DataLog {
  unsigned long waktu;
  float suhu;
  float kelembapan;
  float output;
  float setpoint;
  bool mist;
  bool fan;
};

#define MAX_LOG 20
DataLog logBuffer[MAX_LOG];
int logCount = 0;

// === WIFI RECONNECT ===
void reconnectWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi terputus, reconnect...");
    WiFi.begin(ssid, password);
    int retry = 0;
    while (WiFi.status() != WL_CONNECTED && retry < 10) {
      delay(1000);
      Serial.print(".");
      retry++;
    }
    Serial.println(WiFi.status() == WL_CONNECTED ? "\nWiFi OK" : "\nWiFi Gagal");
  }
}

// === SIMPAN OFFLINE ===
void simpanOffline() {
  if (logCount < MAX_LOG) {
    logBuffer[logCount++] = {
      millis() / 1000,
      input,
      kelembapan,
      output,
      setpoint,
      mistStatus,
      fanStatus
    };
    Serial.println("Data disimpan (offline)");
  }
}

// === KIRIM BUFFER ===
void kirimBuffer() {
  if (logCount == 0) return;

  for (int i = 0; i < logCount; i++) {
    HTTPClient http;
    String url = String(server) + "?api_key=" + apiKey +
                 "&field1=" + logBuffer[i].suhu +
                 "&field2=" + logBuffer[i].kelembapan +
                 "&field3=" + logBuffer[i].mist +
                 "&field4=" + logBuffer[i].fan +
                 "&field5=" + logBuffer[i].setpoint +
                 "&field6=" + logBuffer[i].output;
    http.begin(url);
    if (http.GET() != 200) {
      http.end();
      return;
    }
    http.end();
    delay(1500);
  }
  logCount = 0;
}

// === SETUP ===
void setup() {
  Serial.begin(115200);

  dht.begin();
  dimmer.begin(NORMAL_MODE, ON);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Inkubator Start");

  pinMode(RELAY_FAN, OUTPUT);
  pinMode(RELAY_MIST, OUTPUT);
  digitalWrite(RELAY_FAN, HIGH);
  digitalWrite(RELAY_MIST, HIGH);

  WiFi.setSleep(false);
  WiFi.begin(ssid, password);

  lastTime = millis();
}

// === LOOP ===
void loop() {
  reconnectWiFi();

  // === BACA SENSOR ===
  float suhu_raw = dht.readTemperature();
  float hum_raw  = dht.readHumidity();

  if (isnan(suhu_raw) || isnan(hum_raw)) return;

  input = suhu_raw + suhu_offset;
  kelembapan = hum_raw + hum_offset;
  kelembapan = constrain(kelembapan, 0, 100);

  // === PID ===
  unsigned long now = millis();
  double dt = (now - lastTime) / 1000.0;
  if (dt <= 0) dt = 0.001;
  lastTime = now;

  double error = setpoint - input;
  integral += error * dt;
  double derivative = (error - lastError) / dt;
  lastError = error;

  output = Kp * error + Ki * integral + Kd * derivative;
  output = constrain(output, 0, 100);

  // === DIMMER (dibatasi) ===
  if (millis() - lastDimmerUpdate >= 200) {
    lastDimmerUpdate = millis();
    dimmer.setPower(output);
  }

  // === FAN (histeresis) ===
  static bool fanOn = false;
  if (!fanOn && input > setpoint + 0.1) {
    digitalWrite(RELAY_FAN, LOW);
    fanOn = fanStatus = true;
  } else if (fanOn && input < setpoint - 0.1) {
    digitalWrite(RELAY_FAN, HIGH);
    fanOn = fanStatus = false;
  }

  // === MIST ===
  if (kelembapan < 70) {
    digitalWrite(RELAY_MIST, LOW);
    mistStatus = true;
  } else if (kelembapan > 75) {
    digitalWrite(RELAY_MIST, HIGH);
    mistStatus = false;
  }

  // === LCD (500 ms, TANPA clear) ===
  if (millis() - lastLCDUpdate >= 500) {
    lastLCDUpdate = millis();

    lcd.setCursor(0, 0);
    lcd.print("T:");
    lcd.print(input, 1);
    lcd.print((char)223);
    lcd.print("C H:");
    lcd.print(kelembapan, 0);
    lcd.print("%   ");

    lcd.setCursor(0, 1);
    lcd.print("Heater:");
    lcd.print((int)output);
    lcd.print("% ");
    lcd.print(mistStatus ? "M:ON " : "M:OFF");
  }

  // === KIRIM DATA ===
  if (millis() - lastSendTime >= sendInterval) {
    lastSendTime = millis();

    if (WiFi.status() == WL_CONNECTED) {
      if (logCount > 0) kirimBuffer();

      HTTPClient http;
      String url = String(server) + "?api_key=" + apiKey +
                   "&field1=" + input +
                   "&field2=" + kelembapan +
                   "&field3=" + mistStatus +
                   "&field4=" + fanStatus +
                   "&field5=" + setpoint +
                   "&field6=" + output;

      http.begin(url);
      if (http.GET() != 200) simpanOffline();
      http.end();
    } else {
      simpanOffline();
    }
  }
}