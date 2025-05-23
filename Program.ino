/*  -------------------------------------------------- AUTH TOKEN BLYNK -------------------------------------------- */
#define BLYNK_TEMPLATE_ID "TMPL61WNuqA-p"
#define BLYNK_TEMPLATE_NAME "AUTOCRAFT"
#define BLYNK_AUTH_TOKEN "ESz-S_6kw3gtzNCEWgEabOKApZvpImRs"  //REOG ROBOTIC AUTH  //REOG ROBOTIC AUTH
#define BLYNK_PRINT Serial

/*  -------------------------------------------------- LIBRARY yang digunakan (WiFi, SERVO, EEPROM (Menyimpan Data) )-------------------------------------------- */
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <ESP32Servo.h>
#include <EEPROM.h>

/*  -------------------------------------------------- DEKLARASI Alamat Pin Arduino -------------------------------------------- */
#define servoPin 13     // SERVO
#define waterSensor 34  // SENSOR HUJAN
#define trigPin 12  // ULTRASONIC
#define echoPin 14  // ULTRASONIC
#define relayPin 27 // POMPA

WidgetLCD lcd(V0);
BlynkTimer timer;
Servo myServo;

/*  -------------------------------------------------- VARIABLE DATA -------------------------------------------- */
int pos = 0;
int loopp_buka = 1,
    loopp_tutup = 1,
    loopp_pompa = 1,
    loopp_tutupp = 1;

long durasi, tinggiAir;
int KetinggianAir;
long tinggiSensor = 31;

int addr = 1;

/*  -------------------------------------------------- SSID dan Password WiFi Acces Point -------------------------------------------- */
char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "AndroidAP";
char pass[] = "12345678";

int status = WL_IDLE_STATUS;

/*  -------------------------------------------------- FUNGSI MENCARI WIFI Acces Point/Hotspot -------------------------------------------- */
void InitWiFi()
{
  Serial.println("Connecting to AP ...");
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.println("Connected to AP");
}

/*  -------------------------------------------------- FUNGSI MENYAMBUNGKAN WIFI KEMBALI Jika Koneksi WiFi Terputus -------------------------------------------- */
void reconnect()
{
  status = WiFi.status();
  if ( status != WL_CONNECTED)
  {
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED)
    {
      Serial.print(".");
      delay(500); // Menyetting SSID dan Password WiFi Tanpa direstart
    }
    Serial.print("\nConnected to AP");
  }
}

/*  --------------------------------------------------FUNGSI PROGRAM PEMBACAAN Sensor Ultrasonik (Membaca Ketinggian Air) -------------------------------------------- */
void Ultrasonic_Run() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(8);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(8);
  digitalWrite(trigPin, LOW);
  delayMicroseconds(8);

  durasi = pulseIn(echoPin, HIGH);  // Menerima suara ultrasonic
  tinggiAir = (durasi / 2) / 29.1;  // Mengubah durasi menjadi jarak (cm)
}

/*  -------------------------------------------------- RUMUS Pembacaan Ketinggian Air -------------------------------------------- */
void Baca_Ketinggian() {
  KetinggianAir = tinggiSensor - tinggiAir;
  Serial.print("Ketinggian Air : ");
  Serial.println(KetinggianAir);
}

/*  -------------------------------------------------- Kirim Data Ketinggian Air ke BLYNK -------------------------------------------- */
void sendSensor()
{
  Blynk.virtualWrite(V1, KetinggianAir);
}

/*  -------------------------------------------------- FUNGSI SETUP (Dibaca Pertama Kali Arduino Hidup) -------------------------------------------- */
void setup()
{
  /*  -------------------------------------------------- Mengaktifkan Program Serial dan EEPROM -------------------------------------------- */
  Serial.begin(115200);
  EEPROM.begin(1024);

  pos = EEPROM.read(addr);

  Serial.print("EEPROM : ");
  Serial.println(pos);

  /*  -------------------------------------------------- DEKLARASI Pin Ultrasonik -------------------------------------------- */
  pinMode(trigPin, OUTPUT);
  pinMode(relayPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(waterSensor, INPUT_PULLUP);

  digitalWrite(relayPin, HIGH);

  /*  -------------------------------------------------- Setting Servo -------------------------------------------- */
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  myServo.setPeriodHertz(50);
  myServo.attach(servoPin, 1000, 2000);
  /*  -------------------------------------------------- Pin Servo -------------------------------------------- */
  myServo.attach(servoPin);
  myServo.write(0);

  //  WiFi.mode(WIFI_STA);
  //  WiFi_Manager();

  /*  ----------------------------------------------t---- PROSES Pencarian Koneksi WiFi AP (Jika Menemukan Hotspot / AP maka Otomatis Terhubung ke BLYNK -------------------------------------------- */
  InitWiFi();
  Blynk.begin(auth, ssid, pass);
  timer.setInterval(1000L, sendSensor);
  lcd.print(0, 0, "Monitoring Air");
  delay(100);
  lcd.clear();
}

void loop()
{
  reconnect();
  /*  -------------------------------------------------- Menjalankan Program Pembacaan Ketinggian Air -------------------------------------------- */
  Ultrasonic_Run();
  Baca_Ketinggian();

  /*  -------------------------------------------------- Jika Ketinggian Air Antara 9 s/d 11cm (Maka Ketinggian Air Penuh) -------------------------------------------- */
  if (KetinggianAir >= 9 && KetinggianAir  <= 11) {
    while (loopp_tutup <= 1) {
      /*  -------------------------------------------------- Mengirim Notifikasi GMAIL dan HP Bahwa Air Penuh -------------------------------------------- */
      Blynk.email("reog.robotic@gmail.com", "Monitoring Air", "Kondisi");
      Blynk.logEvent("notifikasi_air" , "Ketinggian Air : " + String(KetinggianAir) + " " + " (Penuh)");
      lcd.clear();
      lcd.print(0, 1, "Tertutup");

      digitalWrite(relayPin, HIGH);
      pos = EEPROM.read(addr);

      loopp_buka  = 1;
      loopp_pompa = 1;
      /*  -------------------------------------------------- Atap Tertutup -------------------------------------------- */
      for (pos = 250; pos >= 0; pos--) {
        myServo.write(pos);
        Serial.print("Sudut Tutup : ");
        Serial.println(pos);
        EEPROM.write(addr, pos);
        delay(15);
      }

      loopp_tutup = 2;
    }
  }

  /*  -------------------------------------------------- Jika Ketinggian Air dibawah 2cm (Air Kosong)-------------------------------------------- */
  else if (KetinggianAir <= 2) {
    while (loopp_pompa <= 1) {
      digitalWrite(relayPin, LOW);
      /*  -------------------------------------------------- Mengirim Notifikasi GMAIL dan HP bahwa Air Kosong -------------------------------------------- */
      Blynk.email("reog.robotic@gmail.com", "Monitoring Air", "Air Kosong");
      Blynk.logEvent("notifikasi_air" , "Air Kosong");

      loopp_pompa = 2;
    }

  }
  /*  -------------------------------------------------- Jika Hujan dan Kondisi Air Dibawah 8cm -------------------------------------------- */
  if (digitalRead(waterSensor) == HIGH && KetinggianAir <= 8) {
    while (loopp_buka <= 1) {
      /*  -------------------------------------------------- Kirim Notifikasi GMAIL dan HP bahwa Kondisi Hujan -------------------------------------------- */
      Blynk.email("reog.robotic@gmail.com", "Monitoring Air", "Kondisi");
      Blynk.logEvent("notifikasi_air" , "Kondisi Hujan");
      lcd.clear();
      lcd.print(0, 1, "Terbuka");

      pos = EEPROM.read(addr);

      /*  -------------------------------------------------- Jika Kondisi Air Kosong dibawah 2cm Maka Pompa Penyala -------------------------------------------- */
      if (KetinggianAir <= 2) {
        digitalWrite(relayPin, LOW);
      }
      /*  -------------------------------------------------- Jika Kondisi Air Mencapai 9 s/d 11 cm Maka Pompa Mati -------------------------------------------- */
      else if (KetinggianAir >= 9 && KetinggianAir  <= 11) {
        digitalWrite(relayPin, HIGH);
      }

      loopp_tutup = 1;
      loopp_tutupp = 1;

      /*  -------------------------------------------------- Atap Terbuka -------------------------------------------- */
      for (pos = 0; pos <= 250; pos++) {
        myServo.write(pos);
        Serial.print("Sudut Buka : ");
        Serial.println(pos);
        EEPROM.write(addr, pos);
        delay(15);
      }
      loopp_buka = 2;
    }
  }

  /*  -------------------------------------------------- Jika Kondisi Tidak Hujan (Maka Kondisi Atap Tertutup) -------------------------------------------- */
  else if (digitalRead(waterSensor) == LOW) {
    while (loopp_tutupp <= 1) {
      lcd.clear();
      lcd.print(0, 1, "Tertutup");

      pos = EEPROM.read(addr);

      /*  -------------------------------------------------- Jika Kondisi Air Kosong dibawah 2cm Maka Pompa Penyala -------------------------------------------- */
      if (KetinggianAir <= 2) {
        digitalWrite(relayPin, LOW);
      }
      /*  -------------------------------------------------- Jika Kondisi Air Mencapai 9 s/d 11 cm Maka Pompa Mati -------------------------------------------- */
      else if (KetinggianAir >= 9 && KetinggianAir  <= 11) {
        digitalWrite(relayPin, HIGH);
      }

      loopp_buka = 1;
      /*  -------------------------------------------------- Atap Tertutup -------------------------------------------- */
      for (pos = 250; pos >= 0; pos--) {
        myServo.write(pos);
        Serial.print("Sudut Tutup : ");
        Serial.println(pos);
        EEPROM.write(addr, pos);
        delay(15);
      }
      loopp_tutupp = 2;
    }
  }

  /*  -------------------------------------------------- KIRIM DATA Ketinggian Air ke BLYNK -------------------------------------------- */
  if (KetinggianAir < 0) {
    lcd.print(0, 0, "Tinggi : 0         ");
  }
  else {
    lcd.print(0, 0, "Tinggi : " + String(KetinggianAir) + "   ");
  }

  Blynk.run();
}
