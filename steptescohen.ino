#include <DHT.h>
#include <RBDdimmer.h>

#define DHTPIN 4
#define DHTTYPE DHT22
#define ZC_PIN 25         // Pin Zero Crossing dari modul AC dimmer
#define DIMMER_PIN 26     // Pin kontrol dimmer ke gate triac

DHT dht(DHTPIN, DHTTYPE);
dimmerLamp dimmer(DIMMER_PIN, ZC_PIN);  // (pin dimmer, pin zero cross)

void setup() {
  Serial.begin(115200);
  dht.begin();
  dimmer.begin(NORMAL_MODE, ON); // Mode normal, dimmer aktif

  // Step test: Heater 30% daya
  dimmer.setPower(0);   // Power 0-100%, misal 30% = setengah daya
}

void loop() {
  float suhu = dht.readTemperature();
  if (!isnan(suhu)) {
    Serial.println(suhu);  // kirim data suhu ke serial
  }
  delay(1000); // kirim tiap 1 detik
}
