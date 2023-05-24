#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "EmonLib.h"                   // Include Emon Library
#include "CTBot.h"                    // use library json version=5.13.5
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TimeLib.h>
#include <EEPROM.h>
#include <ESP8266HTTPClient.h>

#define buzzer D6

CTBot myBot;
EnergyMonitor emon1;
LiquidCrystal_I2C lcd(0x3f, 20, 4);

String ssid  = "CekBiayaListrik";           
String pass  = "ribkaaprilia";            
String token = "1310163127:AAHbfhy1JwNl8fLEXEOl_cPTNDfcFHsbyxw";

const long utcOffsetInSeconds = 28800; // WITA        //25200 UTC + 7 (jakarta) 7x60x60
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

long idBot = 653127052;             // id chat
//unsigned long updtBOT;
unsigned long wkt;
unsigned long hargaPerKWH = 415.0;       // harga perKWH
unsigned long biayaLstrik;
float pwrHour;
float KWH;
float consvolt = 230.0;                 // konstanta voltase
int sample;
String Hour, Min;
byte xReset;

//thingspeak http://api.thingspeak.com/update?api_key=P47FVG53NRFH7HYX&field1=0&field2=0
String URLserver = "http://api.thingspeak.com/update?api_key=P47FVG53NRFH7HYX";

void setup()
{
  Serial.begin(115200);
  Wire.begin(D2, D1);

  EEPROM.begin(512);  //Initialize EEPROM

  pinMode(buzzer, OUTPUT);
  BEEP(2, 500, 500);

  Serial.println("  KWH Meter ONLINE ");
  Serial.print("Menghubungkan SSID ");
  Serial.println(ssid);

  lcd.begin();
  lcd.home ();
  lcd.print("  KWH Meter ONLINE ");
  lcd.setCursor(0, 1);
  lcd.print("Menghubungkan SSID");
  lcd.setCursor(0, 2);
  lcd.print(ssid);

  myBot.wifiConnect(ssid, pass);
  myBot.setTelegramToken(token);

  // check if all things are ok
  if (myBot.testConnection()) {
    Serial.println("\ntestConnection OK");
    lcd.setCursor(0, 3);
    lcd.print("Terhubung");
    delay(2000);
  }
  else {
    Serial.println("\ntestConnection Not OK");
    lcd.print("Terjadi masalah");
    delay(2000);
  }

  lcd.clear();
  lcd.home ();
  lcd.print("  KWH Meter ONLINE ");

  int satuan = EEPROM.read(1);
  int pecahan = EEPROM.read(0);

  KWH = satuan + (pecahan / 100.0);

  lcd.setCursor(0, 3);
  lcd.print("Today: ");
  lcd.print(KWH);
  lcd.print("KWH ");

  emon1.current(0, 41.5);             // Current: input pin, calibration.

  for (byte c = 0; c < 10; c++) {
    double Irms = emon1.calcIrms(1480);  // Calculate Irms only

    float pwr = Irms * consvolt;

    Serial.print("Daya semu ");
    Serial.print(pwr);              // Apparent power
    Serial.print("watt - ");
    Serial.print(Irms);          // Irms
    Serial.println("A");          // Irms

    lcd.setCursor(0, 2);
    lcd.print(Irms);
    lcd.print("A - ");
    lcd.print(pwr);
    lcd.print("Watt ");
    delay(100);
    yield();
  }

  delay(100);

  timeClient.begin();
  Serial.println("Starting ... ");

  //updtBOT = millis();
  wkt = millis();
}

void loop()
{
  timeClient.update();
  showDateandTime();
  TBMessage msg;

  if (myBot.getNewMessage(msg)) {
    Serial.println(msg.sender.id);
    Serial.println(msg.sender.username);
    Serial.println(msg.text);

    if (msg.text.equalsIgnoreCase("AMBIL DATA KWH")) {
      BEEP(2, 250, 250);
      biayaLstrik = hargaPerKWH * KWH;
      String pesan = "Jumlah pemakaian listrik hari ini " + String(KWH) + "KWH - Tagihan listrik Rp. " + String(biayaLstrik);
      myBot.sendMessage(idBot, pesan);
    }
    else if (msg.text.equalsIgnoreCase("RESET KWH")) {
      BEEP(2, 250, 250);
      EEPROM.write(0, 0);     // pecahan
      EEPROM.write(1, 0);     // satuan
      delay(10);
      EEPROM.commit();
      delay(100);

      int satuan = EEPROM.read(1);
      int pecahan = EEPROM.read(0);
      Serial.println(satuan);
      Serial.println(pecahan);
      KWH = satuan + (pecahan / 100.0);

      lcd.setCursor(0, 3);
      lcd.print("Today: ");
      lcd.print(KWH);
      lcd.print("KWH ");

      String pesan = "KWH berhasil di reset";
      myBot.sendMessage(idBot, pesan);
    } else if (msg.text.equalsIgnoreCase("sample")) {
      BEEP(2, 250, 250);
      String pesan = "Jumlah pemakaian listrik hari ini " + String(pwrHour) + "Watt dengan banyak sample " + String(sample) + " = " + String(pwrHour / sample) + "watt sesungguhnya" ;
      myBot.sendMessage(idBot, pesan);
    }
    else {
      BEEP(1, 1000, 0);
      String reply;
      reply = (String)"Welcome " + msg.sender.username + (String)". Coba 'AMBIL DATA KWH' atau 'RESET KWH'.";
      myBot.sendMessage(idBot, reply);
    }
  }

  double Irms = emon1.calcIrms(1480);  // Calculate Irms only

  float pwr = Irms * consvolt;

  pwrHour += pwr;
  sample++;

  Serial.print("Daya semu ");
  Serial.print(pwr);              // Apparent power
  Serial.print("watt - ");
  Serial.print(Irms);          // Irms
  Serial.println("A");          // Irms

  lcd.setCursor(0, 2);
  lcd.print(Irms);
  lcd.print("A - ");
  lcd.print(pwr);
  lcd.print("Watt ");

  //  if(millis() - updtBOT > 3600000){
  //    String pesan = String(KWH) + " watt";
  //    myBot.sendMessage(idBot, pesan);
  //    Serial.println("Kirim data KWH ke Bot Telegram");
  //    updtBOT = millis();
  //  }

  //Serial.println(millis());
  //Serial.println(wkt);
  if (millis() - wkt > 3600000) {                 // average Watt in hour 3600000
    pwrHour = pwrHour / sample;
    pwrHour /= 1000.0;                            // convert Watt to Kilo Watt

    Serial.print(">>> Power dalam 1 Jam = ");
    Serial.print(pwrHour);
    Serial.println(" KWH");

    int satuanOld = EEPROM.read(1);
    //Serial.println(satuanOld);
    int satuan = pwrHour;
    //Serial.println(satuan);
    satuan += satuanOld;
    //Serial.println(satuan);
    //Serial.println("====");

    float calculate;
    if (pwrHour > satuan) {
      calculate = (pwrHour - satuan) * 100;   //rubah pecahan menjadi satuan untuk d simpan dalam eeprom
      //Serial.println(calculate);
    } else {
      calculate = pwrHour * 100;   //rubah pecahan menjadi satuan untuk d simpan dalam eeprom
      //Serial.println(calculate);
    }
    int pecahanOld = EEPROM.read(0);
    //Serial.println(pecahanOld);
    int pecahan = calculate;
    //Serial.println(pecahan);
    pecahan += pecahanOld;
    //Serial.println(pecahan);
    if (pecahan >= 100) {
      Serial.println("convert data");
      satuan += pecahan / 100;
      pecahan -= (pecahan / 100) * 100;
    }
    Serial.print("satuan ");
    Serial.println(satuan);
    Serial.print("pecahan ");
    Serial.println(pecahan);

    EEPROM.write(0, pecahan);
    EEPROM.write(1, satuan);
    delay(10);
    EEPROM.commit();
    delay(100);

    pwrHour = 0;
    sample = 0;

    satuan = EEPROM.read(1);
    pecahan = EEPROM.read(0);

    KWH = satuan + (pecahan / 100.0);

    lcd.setCursor(0, 3);
    lcd.print("Today: ");
    lcd.print(KWH);
    lcd.print("KWH ");

    biayaLstrik = hargaPerKWH * KWH;

    String pesan = String(KWH) + " KWH - Tagihan listrik Rp. " + String(biayaLstrik);
    myBot.sendMessage(idBot, pesan);
    Serial.println("Kirim data KWH ke Bot Telegram");

    wkt = millis();
  }

  Serial.println(" ----------------------- ");
  Serial.println();


  Serial.println("Waktu Sekarang " + Hour + ":" + Min);

  if (Hour + ":" + Min == "0:00") {
    if (xReset == 0) {
      Serial.println("Prepare Send data to server...");
      lcd.setCursor(0, 3);
      lcd.print("Send data to SERVER");

      BEEP(4, 200, 200);

      int satuan = EEPROM.read(1);
      int pecahan = EEPROM.read(0);
  
      KWH = satuan + (pecahan / 100.0);
      biayaLstrik = hargaPerKWH * KWH;
      
      String host = URLserver + "&field1=" + String(KWH) + "&field2=" + String(biayaLstrik);

      HTTPClient http;

      Serial.print("Request Link:");
      Serial.println(host);

      http.begin(host);

      int httpCode = http.GET();            //Send the request
      String payload = http.getString();    //Get the response payload from server

      Serial.print("Response Code:"); //200 is OK
      Serial.println(httpCode);       //Print HTTP return code

      Serial.print("Returned data from Server:");
      Serial.println(payload);    //Print request response payload

      if (httpCode == 200)
      {
        Serial.println("success upload data to thingspeak");
        lcd.setCursor(0, 3);
        lcd.print("                    ");
        lcd.setCursor(0, 3);
        lcd.print("SUCCESS");
      }else{
        Serial.println("error code : "+httpCode);
        lcd.setCursor(0, 3);
        lcd.print("                    ");
        lcd.setCursor(0, 3);
        lcd.print("error code: ");
      }
      delay(3000);
      
      lcd.setCursor(0, 3);
      lcd.print("                    ");
      
      xReset++;
      Serial.println("Reset KWH Meter Onlie");
      EEPROM.write(0, 0);     // pecahan
      EEPROM.write(1, 0);     // satuan
      delay(10);
      EEPROM.commit();
      delay(100);

      satuan = EEPROM.read(1);
      pecahan = EEPROM.read(0);
      Serial.println(satuan);
      Serial.println(pecahan);
      KWH = satuan + (pecahan / 100.0);

      lcd.setCursor(0, 3);
      lcd.print("Today: ");
      lcd.print(KWH);
      lcd.print("KWH ");

      String pesan = "KWH reset otomatis jam 00:00";
      myBot.sendMessage(idBot, pesan);
    }
  } else {
    xReset = 0;
  }
}

void showDateandTime() {
  Serial.println();
  //Serial.println(timeClient.getEpochTime());
  Serial.println(timeClient.getFormattedTime());
  Serial.printf("Date: %4d-%02d-%02d %02d:%02d:%02d\n", year(timeClient.getEpochTime()), month(timeClient.getEpochTime()), day(timeClient.getEpochTime()), hour(timeClient.getEpochTime()), minute(timeClient.getEpochTime()), second(timeClient.getEpochTime()));

  Hour = String(hour(timeClient.getEpochTime()));
  if (minute(timeClient.getEpochTime()) < 10) {
    Min = "0" + String(minute(timeClient.getEpochTime()));
  } else {
    Min = String(minute(timeClient.getEpochTime()));
  }

  //lcd.setCursor(0,1);
  //lcd.print("                    ");
  lcd.setCursor(0, 1);
  if (hour(timeClient.getEpochTime()) < 10) {
    lcd.print("0");
    lcd.print(hour(timeClient.getEpochTime()));
  } else {
    lcd.print(hour(timeClient.getEpochTime()));
  }
  lcd.print(":");
  if (minute(timeClient.getEpochTime()) < 10) {
    lcd.print("0");
    lcd.print(minute(timeClient.getEpochTime()));
  } else {
    lcd.print(minute(timeClient.getEpochTime()));
  }
  lcd.print(":");
  if (second(timeClient.getEpochTime()) < 10) {
    lcd.print("0");
    lcd.print(second(timeClient.getEpochTime()));
  } else {
    lcd.print(second(timeClient.getEpochTime()));
  }
  lcd.print("  ");
  lcd.print(day(timeClient.getEpochTime()));
  lcd.print("/");
  lcd.print(month(timeClient.getEpochTime()));
  lcd.print("/");
  lcd.print(year(timeClient.getEpochTime()));
  lcd.print(" ");
}

// function sound beep = BEEP(how many, delay on in ms, delay off in ms);
void BEEP(byte c, int wait1, int wait2) {
  for (byte b = 0; b < c; b++) {
    digitalWrite(buzzer, HIGH);
    delay(wait1);
    digitalWrite(buzzer, LOW);
    delay(wait2);
  }
}
