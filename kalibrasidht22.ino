#include "DHT.h"
#define DHTPIN 4       // Pin data DHT22 ke GPIO4 ESP32
#define DHTTYPE DHT22  // Jenis sensor
DHT dht(DHTPIN, DHTTYPE);
// Nilai koreksi hasil kalibrasi
const float suhu_offset = -0.20;     // °C
const float hum_offset = -7.20;     // %
void setup() {
  Serial.begin(115200);
  dht.begin();
}
void loop() {
  // Baca data sensor
  float suhu_raw = dht.readTemperature();    // Suhu tanpa koreksi
  float hum_raw = dht.readHumidity();        // Kelembapan tanpa koreksi

  // Periksa apakah pembacaan valid
  if (isnan(suhu_raw) || isnan(hum_raw)) {
    Serial.println("Gagal membaca dari DHT22!");
    delay(2000);
    return;
  }

  // Terapkan koreksi hasil kalibrasi
  float suhu_kalibrasi = suhu_raw + suhu_offset;
  float hum_kalibrasi = hum_raw + hum_offset;

  // Pastikan kelembapan tidak keluar dari 0–100%
  if (hum_kalibrasi < 0) hum_kalibrasi = 0;
  if (hum_kalibrasi > 100) hum_kalibrasi = 100;

  // Tampilkan hasil
  Serial.print("Suhu mentah: ");
  Serial.print(suhu_raw);
  Serial.print(" °C  |  Setelah kalibrasi: ");
  Serial.print(suhu_kalibrasi);
  Serial.println(" °C");

  Serial.print("Kelembapan mentah: ");
  Serial.print(hum_raw);
  Serial.print(" %  |  Setelah kalibrasi: ");
  Serial.print(hum_kalibrasi);
  Serial.println(" %");
  delay(5000); // Baca setiap 5 menit
}