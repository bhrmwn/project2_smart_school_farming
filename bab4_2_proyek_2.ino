#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID "YOUR_TEMPLATE_ID"
#define BLYNK_DEVICE_NAME "Hidroponik Sekolah"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// --- KREDENSIAL PENGGUNA ---
char auth[] = "YOUR_BLYNK_AUTH_TOKEN"; 
char ssid[] = "NAMA_WIFI_ANDA";
char pass[] = "PASSWORD_WIFI_ANDA";

// --- KONFIGURASI PIN ---
const int PIN_SUHU_AIR = 4;
const int PIN_RELAY_POMPA = 18;
const int PIN_PH = 34;
const int PIN_TDS = 35;
const int PIN_TRIG = 13;
const int PIN_ECHO = 12;

// --- INISIALISASI OBJEK ---
LiquidCrystal_I2C lcd(0x27, 16, 2);
OneWire oneWire(PIN_SUHU_AIR);
DallasTemperature sensors(&oneWire);
BlynkTimer timer;

// --- PENGATURAN JADWAL POMPA (NON-BLOCKING) ---
unsigned long waktuPompaNyala = 15 * 60 * 1000; // 15 menit
unsigned long waktuPompaMati = 45 * 60 * 1000; // 45 menit
unsigned long waktuSebelumnya = 0;
bool statusPompa = false; // false = mati, true = nyala

void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();
  sensors.begin();
  pinMode(PIN_RELAY_POMPA, OUTPUT);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  digitalWrite(PIN_RELAY_POMPA, HIGH); // Pompa mati di awal

  Blynk.begin(auth, ssid, pass);
  timer.setInterval(5000L, kirimDataSensor); // Kirim data setiap 5 detik
}

void loop() {
  Blynk.run();
  timer.run();
  jalankanJadwalPompa();
  tampilkanDataKeLayar();
}

void kirimDataSensor() {
  float suhu = bacaSuhuAir();
  float ph = bacaPH();
  float tds = bacaTDS();
  long level = bacaLevelAir();

  Blynk.virtualWrite(V0, suhu);
  Blynk.virtualWrite(V1, ph);
  Blynk.virtualWrite(V2, tds);
  Blynk.virtualWrite(V3, level);
  Blynk.virtualWrite(V4, statusPompa ? 255 : 0); // Kirim status pompa
}

// Tombol manual untuk pompa
BLYNK_WRITE(V5) {
  digitalWrite(PIN_RELAY_POMPA, param.asInt() == 1 ? LOW : HIGH);
}

void jalankanJadwalPompa() {
  unsigned long waktuSekarang = millis();
  if (statusPompa) { // Jika pompa sedang nyala
    if (waktuSekarang - waktuSebelumnya >= waktuPompaNyala) {
      statusPompa = false;
      waktuSebelumnya = waktuSekarang;
      digitalWrite(PIN_RELAY_POMPA, HIGH); // Matikan pompa
    }
  } else { // Jika pompa sedang mati
    if (waktuSekarang - waktuSebelumnya >= waktuPompaMati) {
      statusPompa = true;
      waktuSebelumnya = waktuSekarang;
      digitalWrite(PIN_RELAY_POMPA, LOW); // Nyalakan pompa
    }
  }
}

// --- FUNGSI-FUNGSI PEMBACAAN SENSOR ---

float bacaSuhuAir() {
  sensors.requestTemperatures(); 
  return sensors.getTempCByIndex(0);
}

long bacaLevelAir() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  long durasi = pulseIn(PIN_ECHO, HIGH);
  return durasi * 0.034 / 2; // Konversi ke cm
}

float bacaPH() {
  int nilaiAnalog = analogRead(PIN_PH);
  // PENTING: Rumus di bawah ini HANYA CONTOH dan HARUS diganti
  // dengan hasil kalibrasi Anda sendiri!
  float tegangan = nilaiAnalog * (3.3 / 4095.0);
  float ph = 7.0 - ((2.5 - tegangan) / 0.18); // Contoh rumus linier
  return ph;
}

float bacaTDS() {
  int nilaiAnalog = analogRead(PIN_TDS);
  // PENTING: Lakukan kalibrasi dengan larutan TDS standar
  // untuk mendapatkan rumus konversi yang akurat.
  return nilaiAnalog * 0.5; // Contoh rumus sangat sederhana
}

void tampilkanDataKeLayar() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("pH:");
  lcd.print(bacaPH(), 1);
  lcd.print(" TDS:");
  lcd.print(bacaTDS(), 0);
  
  lcd.setCursor(0,1);
  lcd.print("Suhu:");
  lcd.print(bacaSuhuAir(), 1);
  lcd.print(" Pompa:");
  lcd.print(statusPompa ? "ON" : "OFF");
}
