#include <Adafruit_BME280.h>
#include <Adafruit_Sensor.h>
#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <WiFi.h>
#include <WiFiAP.h>

#include <WiFiClientSecure.h>

#include <Wire.h>

#include "time.h"

Adafruit_BME280 bme;

bool apMode = false;
bool bmeAvailable = false;

unsigned long letzterVerbindungsversuch = 0;

IPAddress Client1_IP(192, 168, 0, 99);
IPAddress Client1_gateway(192, 168, 0, 1);
IPAddress Client1_subnet(255, 255, 255, 0);
IPAddress Client2_IP(192, 168, 178, 99);
IPAddress Client2_gateway(192, 168, 178, 1);
IPAddress Client2_subnet(255, 255, 255, 0);
IPAddress Client_primaryDNS(8, 8, 8, 8);
IPAddress Client_secondaryDNS(8, 8, 4, 4);

IPAddress AP_IP(192, 168, 0, 99);
IPAddress AP_gateway(192, 168, 0, 1);
IPAddress AP_subnet(255, 255, 255, 0);

AsyncWebServer server(80);

String DatumZeit;
String payload_esp32;

float temperature_bme = 0;
float luftdruck_bme = 0;
float luftfeuchtigkeit_bme = 0;
float Vorgabe12Van = 13.8;
float Vorgabe12Vaus = 12.5;
float Vorgabe24Van = 27.2;
float Vorgabe24Vaus = 25.8;
float temperatur = 0;
float luftdruck = 0;
float luftfeuchtigkeit = 0;
float Spannung12V = 0;
float Spannung24V = 0;

String StatusRelais1;
String StatusRelais2;
String StatusRelais3;
String StatusRelais4;

int WertPin12V = 0;
int WertPin24V = 0;

const uint32_t IntervallNTP = 42000000; // 11 Std 40 Minuten, absolutes Maximum = 42949672 sonst Overflow

const unsigned long checkIntervall = 600000UL; // 10 Minuten

const int httpsPort = 443;
const int DST_Offset_sec = 3600; // 1 Std
const int MessintervallBatt = 20000;  // 20 Sek
const int MessintervallBME280 = 1800000; // 30 Min
const int BattPin12V = 36;
const int BattPin24V = 39;
const int Relais1Pin = 14;
const int Relais2Pin = 27;
const int Relais3Pin = 26;
const int Relais4Pin = 25;
const int anzahlWLANs = 2;

const float umrechnungsfaktor12V = 174.867;
const float umrechnungsfaktor24V = 96.85;

const long  TZ_Offset_sec = 3600; //GMT OFFSET DE +1Std (3600 SEC)

const char* ssids[]     = {"WLAN1", "WLAN2"};
const char* passwords[] = {"geheim1", "geheim2"};
const char* uploadserver = "meine-domain.de";
//const char* uploadserver = "192.168.0.10";
const char* apiKey = "Geheim";
const char* PARAM_DatumZeit = "DatumZeit";
const char* Hostname = "ESP32";
const char* AP_ssid = "ESP32";
const char* AP_password = "geheim";
const char* NTP = "de.pool.ntp.org";
const char* PARAM_FLOAT12Van = "Vorgabe12Van";
const char* PARAM_FLOAT12Vaus = "Vorgabe12Vaus";
const char* PARAM_FLOAT24Van = "Vorgabe24Van";
const char* PARAM_FLOAT24Vaus = "Vorgabe24Vaus";
const char* PARAM_Temperatur = "temperatur";
const char* PARAM_Luftdruck = "luftdruck";
const char* PARAM_Luftfeuchtigkeit = "luftfeuchtigkeit";
const char* PARAM_Spannung12V = "Spannung12V";
const char* PARAM_Spannung24V = "Spannung24V";
const char* PARAM_StatusRelais1 = "StatusRelais1";
const char* PARAM_StatusRelais2 = "StatusRelais2";
const char* PARAM_StatusRelais3 = "StatusRelais3";
const char* PARAM_StatusRelais4 = "StatusRelais4";
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
  <meta http-equiv="cache-control" content="no-cache" />
  <meta http-equiv="pragma" content="no-cache" />
  <meta http-equiv="expires" content="-1" />
  <meta http-equiv="refresh" content="60" />
  <title>Ein- Ausschaltspannung</title>
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <script>
    function submit12Van() {
      alert("Einschaltspannung 12 Volt gespeichert");
      setTimeout(function(){ document.location.reload(false); }, 3000);
    }
    function submit12Vaus() {
      alert("Ausschaltspannung 12 Volt gespeichert");
      setTimeout(function(){ document.location.reload(false); }, 3000);
    }
    function submit24Van() {
      alert("Einschaltspannung 24 Volt gespeichert");
      setTimeout(function(){ document.location.reload(false); }, 3000);
    }
    function submit24Vaus() {
      alert("Ausschaltspannung 24 Volt gespeichert");
      setTimeout(function(){ document.location.reload(false); }, 3000);
    }
  </script>
  </head>
  <body bgcolor="#000000" text="#FFFFFF" link="#FFFFFF" vlink="#FFFFFF" alink="#FFFFFF">
  <center>
<table style="font-size:20px; border:1px solid grey; width:99%; margin-bottom:20px;">
  <tr>
    <th colspan="4" style="font-size:20px;">gemessene Spannung %DatumZeit%</th>
  </tr>
  <tr>
    <td style="width:4%; border:1px solid grey;">
    <td style="width:auto; font-size:20px;text-align: center; border:1px solid grey;">12 Volt %StatusRelais1%</td>
    <td style="width:auto; font-size:20px;text-align: left; border:1px solid grey;">%Spannung12V% Volt</td>
    <td style="width:5%; border:1px solid grey;">
  </tr>
  <tr>
    <td style="width:4%; border:1px solid grey;">
    <td style="width:auto; font-size:20px;text-align: center; border:1px solid grey;">24 Volt %StatusRelais3%</td>
    <td style="width:auto; font-size:20px;text-align: left; border:1px solid grey;">%Spannung24V% Volt</td>
    <td style="width:5%; border:1px solid grey;">
  </tr>
</table>

<table style="font-size:20px; border:1px solid grey; width:99%; margin-bottom:20px;">
  <tr>
    <th colspan="4" style="font-size:20px; border:1px solid grey;">Schaltspannung 12V</th>
  </tr>
  <tr>
    <td style="width:4%; border:1px solid grey;">
    <td style="width:auto; font-size:20px;text-align: center; border:1px solid grey;">Strom an bei:</td>
    <td style="width:auto; font-size:20px;text-align: left; border:1px solid grey;">%Vorgabe12Van% Volt</td>
    <td style="width:5%; border:1px solid grey;">
  </tr>
  <tr>
    <td style="width:4%; border:1px solid grey;">
    <td style="width:auto; font-size:20px;text-align: center; border:1px solid grey;">Strom aus bei:</td>
    <td style="width:auto; font-size:20px;text-align: left; border:1px solid grey;">%Vorgabe12Vaus% Volt</td>
    <td style="width:5%; border:1px solid grey;">
  </tr>
</table>

<table style="font-size:20px; border:1px solid grey; width:99%; margin-bottom:20px;">
  <tr>
    <th colspan="4" style="font-size:20px; border:1px solid grey;">Schaltspannung 24V</th>
  </tr>
  <tr>
    <td style="width:4%; border:1px solid grey;">
    <td style="width:auto; font-size:20px;text-align: right; border:1px solid grey;">Strom an bei:</td>
    <td style="width:auto; font-size:20px;text-align: left; border:1px solid grey;">%Vorgabe24Van% Volt</td>
    <td style="width:5%; border:1px solid grey;">
  </tr>
  <tr>
    <td style="width:4%; border:1px solid grey;">
    <td style="width:auto; font-size:20px;text-align: right; border:1px solid grey;">Strom aus bei:</td>
    <td style="width:auto; font-size:20px;text-align: left; border:1px solid grey;">%Vorgabe24Vaus% Volt</td>
  <td style="width:5%; border:1px solid grey;">
  </tr>
</table>

<table style="font-size:20px; border:1px solid grey; width:99%; margin-bottom:20px;">
  <tr>
    <th colspan="4" style="font-size:20px; border:1px solid grey;">Ändern der Schaltspannungen</th>
  </tr>
  <!-- 12V Eingaben -->
  <tr>
    <td style="width:4%; border:1px solid grey;">
    <td style="width:auto; font-size:20px; text-align: right; border:1px solid grey;">12 Volt ein:</td>
    <td style="width:auto;font-size:20px">
      <form action="/get" target="hidden-form" style="margin:0;">
        <input style="font-size:20px;text-align: left; width:70px; border:1px solid grey;" value="%Vorgabe12Van%" size="5" type="number" step="0.1" name="Vorgabe12Van" min="12" max="15" />
        <input style="font-size:20px;text-align: left; width:auto; border:1px solid grey;" type="submit" value="Speichern" onclick="submit12Van()" />
      </form>
      <td style="width:5%; border:1px solid grey;"></td>
    </td>
  </tr>
  <tr>
    <td style="width:4%; border:1px solid grey;">
    <td style="width:auto; font-size:20px; text-align: right; border:1px solid grey;">12 Volt aus:</td>
    <td style="width:auto; font-size:20px;">
      <form action="/get" target="hidden-form" style="margin:0;">
        <input style="font-size:20px;text-align: left; width:70px; border:1px solid grey;" value="%Vorgabe12Vaus%" size="5" type="number" step="0.1" name="Vorgabe12Vaus" min="12" max="15" />
        <input style="font-size:20px;text-align: left; width:auto; border:1px solid grey;" type="submit" value="Speichern" onclick="submit12Vaus()" />
      </form>
    <td style="width:5%; border:1px solid grey;">
    </td>
  </tr>
  <!-- 24V Eingaben -->
  <tr>
    <td style="width:4%; border:1px solid grey;">
    <td style="width:auto; font-size:20px;text-align: right; border:1px solid grey;">24 Volt ein:</td>
    <td style="width:auto; font-size:20px">
      <form action="/get" target="hidden-form" style="margin:0;">
        <input style="font-size:20px;text-align: left; width:70px; border:1px solid grey;" value="%Vorgabe24Van%" size="5" type="number" step="0.1" name="Vorgabe24Van" min="24" max="30" />
        <input style="font-size:20px;text-align: left; width:auto; border:1px solid grey;" type="submit" value="Speichern" onclick="submit24Van()" />
      </form>
    <td style="width:5%; border:1px solid grey;">
    </td>
  </tr>
  <tr>
    <td style="width:4%; border:1px solid grey;">
    <td style="width:auto; font-size:20px;text-align: right; border:1px solid grey;">24 Volt aus:</td>
    <td style="width:auto; font-size:20px">
      <form action="/get" target="hidden-form" style="margin:0;">
        <input style="font-size:20px;text-align: left; width:70px; border:1px solid grey;" value="%Vorgabe24Vaus%" size="5" type="number" step="0.1" name="Vorgabe24Vaus" min="24" max="30" />
        <input style="font-size:20px;text-align: left; width:auto; border:1px solid grey;" type="submit" value="Speichern" onclick="submit24Vaus()" />
      </form>
    <td style="width:5%; border:1px solid grey;">
    </td>
  </tr>
</table>

<!-- Tabelle 5: Sensorwerte -->
<table style="font-size:20px; border:1px solid grey; width:90%; margin-bottom:20px;">
  <tr>
    <th colspan="5" style="font-size:20px;">Sensorwerte %DatumZeit%</th>
  </tr>
  <tr>
    <td style="width:10%;font-size:20px"></td>
    <td style="width:30%;font-size:20px;text-align: center; border:1px solid grey;">Temperatur</td>
    <td style="width:20%;font-size:20px;text-align: left; border:1px solid grey;">%temperatur%</td>
    <td style="width:20%;font-size:20px;text-align: left; border:1px solid grey;">°C</td>
    <td style="width:10%;font-size:20px"></td>
  </tr>
  <tr>
  <td style="width:10%;font-size:20px"></td>
    <td style="width:30%;font-size:20px;text-align: center; border:1px solid grey;">Luftdruck</td>
    <td style="width:20%;font-size:20px;text-align: left; border:1px solid grey;">%luftdruck%</td>
    <td style="width:20%;font-size:20px;text-align: left; border:1px solid grey;">hPa</td>
    <td style="width:10%;font-size:20px"></td>
  </tr>
  <tr>
  <td style="width:10%;font-size:20px"></td>
    <td style="width:30%;font-size:20px;text-align: center; border:1px solid grey;">Luftfeuchtigkeit</td>
    <td style="width:20%;font-size:20px;text-align: left; border:1px solid grey;">%luftfeuchtigkeit%</td>
    <td style="width:20%;font-size:20px;text-align: left; border:1px solid grey;">%</td>
    <td style="width:10%;font-size:20px"></td>
  </tr>
</table>
%DatumZeit%
</center></body></html>)rawliteral";

String processor(const String& var){
  if(var == "DatumZeit"){ return String(DatumZeit); }
  if(var == "Vorgabe12Van"){ return String(Vorgabe12Van); }
  if(var == "Vorgabe12Vaus"){ return String(Vorgabe12Vaus); }
  if(var == "Vorgabe24Van"){ return String(Vorgabe24Van); }
  if(var == "Vorgabe24Vaus"){ return String(Vorgabe24Vaus); }
  if(var == "temperatur"){ return String(temperatur); }
  if(var == "luftdruck"){ return String(luftdruck); }
  if(var == "luftfeuchtigkeit"){ return String(luftfeuchtigkeit); }
  if(var == "Spannung12V"){ return String(Spannung12V); }
  if(var == "Spannung24V"){ return String(Spannung24V); }
  if(var == "StatusRelais1"){ return String(StatusRelais1); }
  if(var == "StatusRelais3"){ return String(StatusRelais3); }
  return String();
}

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "404: Not found");
}

void uploadzumserver(){
  WiFiClientSecure client;
  client.setInsecure();
  Serial.println("Verbinde zum Uploadserver...");
  if (!client.connect(uploadserver, httpsPort)) {
    Serial.println("Verbindung zum Uploadserver fehlgeschlagen!");
    return;
  }
  Serial.println("Verbunden. Sende Daten...");
  String request =
    "POST /dir/esp32/upload_esp32.php?key=" + String(apiKey) + " HTTP/1.1\r\n" +
    "Host: meine-domain.de\r\n" +
    "Content-Type: text/plain\r\n" +
    "Content-Length: " + String(payload_esp32.length()) + "\r\n" +
    "Connection: close\r\n\r\n" +
    payload_esp32;
  Serial.print("Payload, der gesendet wird: >");
  Serial.print(payload_esp32);
  Serial.print("<");
  Serial.print("Payload Länge: ");
  Serial.println(payload_esp32.length());
  client.print(request);
  Serial.println("Warte auf Antwort...");
  while (client.connected() || client.available()) {
    if (client.available()) {
      Serial.println(client.readStringUntil('\n'));
    }
  }
  client.stop();
  Serial.println("Fertig.");
}

void bmeTask(void *pvParameters) {
  const TickType_t delay = pdMS_TO_TICKS(MessintervallBME280);
  for(;;) {
    vTaskDelay(500);
    float t, p, h;
    if (bmeAvailable) {
      t = bme.readTemperature();
      p = bme.readPressure() / 100.0F;
      h = bme.readHumidity();
      if (isnan(t) || isnan(p) || isnan(h)) {
        Serial.println("Falsche BME280-Daten, nutze Standard-Werte");
        t = 22.22;
        p = 999.99;
        h = 55.55;
      }
    } else {
      t = 22.22;
      p = 999.99;
      h = 55.55;
    }
    temperatur = t;
    luftdruck = p;
    luftfeuchtigkeit = h;
    char tempchar[8], druckchar[8], feuchtchar[8];
    dtostrf(t, 1, 2, tempchar);
    dtostrf(p, 1, 2, druckchar);
    dtostrf(h, 1, 2, feuchtchar);
    Serial.print("\nTemperatur : ");
    Serial.print(tempchar);
    Serial.print(" °C\n");
    Serial.print("Luftdruck : ");
    Serial.print(druckchar);
    Serial.print(" hPa\n");
    Serial.print("Luftfeuchtigkeit : ");
    Serial.print(feuchtchar);
    Serial.print(" %\n");
    payload_esp32 =
      String(tempchar) + ";" +
      String(druckchar) + ";" +
      String(feuchtchar) + ";" +
      String(Spannung12V) + ";" +
      String(Spannung24V) + ";" +
      String(StatusRelais1) + ";" +
      String(StatusRelais3) + "\n";
    uploadzumserver();
    vTaskDelay(delay);
  }
}

void analogTask(void *pvParameters) {
  const TickType_t delay = pdMS_TO_TICKS(MessintervallBatt);
  for(;;) {
    WertPin12V = analogRead(BattPin12V);
    Spannung12V = WertPin12V / umrechnungsfaktor12V;
    WertPin24V = analogRead(BattPin24V);
    Spannung24V = WertPin24V / umrechnungsfaktor24V;
    String str;
    if (Spannung12V >= Vorgabe12Van && digitalRead(Relais1Pin) == LOW) {
      Serial.println( str + "Spannung ist " + Spannung12V + ", also über " + Vorgabe12Van + ", Strom ist an und bleibt an.");
    }
    else if (Spannung12V <= Vorgabe12Van && Vorgabe12Vaus <= Spannung12V && digitalRead(Relais1Pin) == LOW) {
      Serial.println( str + "Spannung ist " + Spannung12V + ",also unter " + Vorgabe12Van + ", aber über " + Vorgabe12Vaus + ", Strom ist an und bleibt an.");
    }
    else if (Spannung12V <= Vorgabe12Vaus && digitalRead(Relais1Pin) == LOW) {
      Serial.println( str + "Spannung ist " + Spannung12V + "ist also unter " + Vorgabe12Vaus + "und Strom ist noch an, schalte Strom aus.");
      Relais1und2aus();
    }
    else if (Spannung12V >= Vorgabe12Vaus && Spannung12V <= Vorgabe12Van && digitalRead(Relais1Pin) == HIGH) {
      Serial.println( str + "Spannung ist " + Spannung12V + ", also über " + Vorgabe12Vaus + " aber unter " + Vorgabe12Van + ", Strom ist aus und bleibt aus.");
    }
    else if (Spannung12V <= Vorgabe12Vaus && Spannung12V <= Vorgabe12Van && digitalRead(Relais1Pin) == HIGH) {
//      Serial.println( str + "Spannung ist " + Spannung12V + ", also unter " + Vorgabe12Vaus + ", Strom ist aus und bleibt aus.");
    }
    else if (Spannung12V >= Vorgabe12Van && digitalRead(Relais1Pin) == HIGH) {
      Serial.println( str + "Spannung ist " + Spannung12V + ", also über " + Vorgabe12Van + ", Strom ist aus, schalte Strom ein.");
      Relais1und2an();
    }
    else {
      Serial.println("Fehler beim Zugriff auf GPIOs");
    }
        if (Spannung24V >= Vorgabe24Van && digitalRead(Relais3Pin) == LOW) {
      Serial.println( str + "Spannung ist " + Spannung24V + ",also über " + Vorgabe24Van + ", Strom ist an und bleibt an.");
    }
    else if (Spannung24V <= Vorgabe24Van && Vorgabe24Vaus <= Spannung24V && digitalRead(Relais3Pin) == LOW) {
      Serial.println( str + "Spannung ist " + Spannung24V + ",also unter " + Vorgabe24Van + ", aber über " + Vorgabe24Vaus + ", Strom ist an und bleibt an.");
    }
    else if (Spannung24V <= Vorgabe24Vaus && digitalRead(Relais3Pin) == LOW) {
      Serial.println( str + "Spannung ist " + Spannung24V + ", also unter " + Vorgabe24Vaus + " und Strom ist noch an, schalte Strom aus");
      Relais3und4aus();
    }
    else if (Spannung24V >= Vorgabe24Vaus && Spannung24V <= Vorgabe24Van && digitalRead(Relais3Pin) == HIGH) {
      Serial.println( str + "Spannung ist " + Spannung24V + ",also über " + Vorgabe24Vaus + "aber unter " + Vorgabe24Van + ", Strom ist aus und bleibt aus.");
    }
    else if (Spannung24V <= Vorgabe24Vaus && Spannung24V <= Vorgabe24Van && digitalRead(Relais3Pin) == HIGH) {
//      Serial.println( str + "Spannung ist " + Spannung24V + ", also unter " + Vorgabe24Vaus + ", Strom ist aus und bleibt aus.");
    }
    else if (Spannung24V >= Vorgabe24Van && digitalRead(Relais3Pin) == HIGH) {
      Serial.println( str + "Spannung ist " + Spannung24V + ", also über " + Vorgabe24Van + ", Strom ist aus, schalte Strom ein.");
      Relais3und4an();
    }
    else {
      Serial.println("Fehler beim Zugriff auf GPIOs");
    }
    Serial.print("\n\n");
    vTaskDelay(delay);
  }
}

void Relais1und2an() {
  pinMode(Relais1Pin, OUTPUT);
  pinMode(Relais2Pin, OUTPUT);
  digitalWrite(Relais1Pin, LOW);
  StatusRelais1 = "an";
  digitalWrite(Relais2Pin, LOW);
  StatusRelais2 = "an";
}

void Relais1und2aus() {
  pinMode(Relais1Pin, OUTPUT);
  pinMode(Relais2Pin, OUTPUT);
  digitalWrite(Relais1Pin, HIGH);
  StatusRelais1 = "aus";
  digitalWrite(Relais2Pin, HIGH);
  StatusRelais2 = "aus";
}

void Relais3und4an() {
  pinMode(Relais3Pin, OUTPUT);
  pinMode(Relais4Pin, OUTPUT);
  digitalWrite(Relais3Pin, LOW);
  StatusRelais3 = "an";
  digitalWrite(Relais4Pin, LOW);
  StatusRelais4 = "an";
}

void Relais3und4aus() {
  pinMode(Relais3Pin, OUTPUT);
  pinMode(Relais4Pin, OUTPUT);
  digitalWrite(Relais3Pin, HIGH);
  StatusRelais3 = "aus";
  digitalWrite(Relais4Pin, HIGH);
  StatusRelais4 = "aus";
}

void NTPTask(void *pvParameters) {
  const TickType_t ntpDelay = pdMS_TO_TICKS(IntervallNTP);
  for(;;) {
    struct tm timeinfo;
    bool validTime = false;
    if (WiFi.status() == WL_CONNECTED) {
      // NTP nur versuchen, wenn WLAN da ist
      configTime(TZ_Offset_sec, DST_Offset_sec, NTP);
      Serial.println("Warte auf Zeitsynchronisation...");
      int retries = 10;
      while (retries-- > 0) {
        if (getLocalTime(&timeinfo)) {
          Serial.println("Zeit synchronisiert");
          validTime = true;
          break;
        }
        vTaskDelay(pdMS_TO_TICKS(500));
      }
    }
    if (!validTime) {
      Serial.println("Keine NTP-Zeit, setze eigene Zeit");
      timeinfo.tm_year = 2026 - 1900;
      timeinfo.tm_mon  = 0;
      timeinfo.tm_mday = 1;
      timeinfo.tm_hour = 12;
      timeinfo.tm_min  = 0;
      timeinfo.tm_sec  = 0;
      time_t t = mktime(&timeinfo);
      struct timeval now = { .tv_sec = t };
      settimeofday(&now, NULL);
    }
    char timeStringBuff[20];
    strftime(timeStringBuff, sizeof(timeStringBuff), "%d.%m.%y %H:%M:%S", &timeinfo);
    DatumZeit = String(timeStringBuff);
    Serial.println(DatumZeit);
    vTaskDelay(ntpDelay);
  }
}

void verbindungsversuch() {

  if (!apMode && WiFi.status() == WL_CONNECTED) {
    if (millis() - letzterVerbindungsversuch > checkIntervall) {
      letzterVerbindungsversuch = millis();
      Serial.println("WLAN ist noch verbunden.");
    }
    return;
  }
  if (!apMode && WiFi.status() != WL_CONNECTED) {
    Serial.println("WLAN verloren, versuche 1 Minute lang wieder zu verbinden...");
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 60000) {
      if (verbindungWLAN()) return;
    }
    Serial.println("Verbindung zum WLAN fehlgeschlagen fehlgeschlagen, starte AP");
    starteAP();
    return;
  }
  if (apMode && millis() - letzterVerbindungsversuch > checkIntervall) {
    letzterVerbindungsversuch = millis();
    Serial.println("AP ist noch aktiv, prüfe, ob WLAN wieder verfügbar ist...");
    stoppeAP();
    if (!verbindungWLAN()) {
      Serial.println("Kein WLAN erreichbar, starte AP");
      starteAP();
    }
  }
}

bool verbindungWLAN() {
  Serial.println("WLAN-Stack wird neu gestartet...");
  WiFi.disconnect();
  delay(1000);
  WiFi.mode(WIFI_OFF);
  delay(1000);
  WiFi.mode(WIFI_STA);
  delay(1000);
  for (int i = 0; i < anzahlWLANs; i++) {
    const char* ssid = ssids[i];
    const char* pass = passwords[i];
    Serial.printf("SSID: %s\n", ssid);
    Serial.printf("Passwort: %s\n", pass);
    if (strcmp(ssid, "WLAN1") == 0) {
      Serial.println("Setze IP für WLAN1");
      WiFi.config(Client1_IP, Client1_gateway, Client1_subnet,
                  Client_primaryDNS, Client_secondaryDNS);
    }
    else if (strcmp(ssid, "WLAN2") == 0) {
      Serial.println("Setze IP für WLAN2");
      WiFi.config(Client2_IP, Client2_gateway, Client2_subnet,
                  Client_primaryDNS, Client_secondaryDNS);
    }
    else {
      Serial.println("Unbekannte SSID, nutze DHCP");
      WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);  // DHCP
    }
    WiFi.begin(ssid, pass);
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
      Serial.print(".");
      delay(500);
    }
    Serial.println();
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("Verbunden, IP: ");
      Serial.println(WiFi.localIP());
      return true;
    }
    Serial.println("Verbindungsversuch fehlgeschlagen, probiere nächstes WLAN...");
    WiFi.disconnect();
    delay(1000);
  }
  return false;
}


void starteAP() {
  Serial.println("Starte AP...");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_ssid, AP_password);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
  apMode = true;
  Serial.println("AP gestartet.");
}

void stoppeAP() {
  if (apMode) {
    Serial.println("Stoppe AP...");
    WiFi.softAPdisconnect(true);
    delay(500);
    apMode = false;
    Serial.println("AP gestoppt.");
  }
}

void resetWLAN() {
  Serial.println("WLAN wird zurückgesetzt...");
  WiFi.disconnect();
  delay(500);
  WiFi.mode(WIFI_OFF);
  delay(500);
  WiFi.mode(WIFI_STA);
  delay(500);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\nVerbinde mit WLAN ...");
  if (!verbindungWLAN()) {
    Serial.println("Kein WLAN erreichbar, starte AP.");
    starteAP();
    Serial.println("AP gestartet.");
  }
  if (bme.begin(0x76)) {
    Serial.println("BME280 gefunden.");
    bmeAvailable = true;
  } else {
    Serial.println("Kein BME280 gefunden! Nutze Standard-Werte.");
    bmeAvailable = false;
  }
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String htmlResponse = index_html;
    htmlResponse.replace("%DatumZeit%", String(DatumZeit));
    htmlResponse.replace("%Vorgabe12Van%", String(Vorgabe12Van));
    htmlResponse.replace("%Vorgabe12Vaus%", String(Vorgabe12Vaus));
    htmlResponse.replace("%Vorgabe24Van%", String(Vorgabe24Van));
    htmlResponse.replace("%Vorgabe24Vaus%", String(Vorgabe24Vaus));
    htmlResponse.replace("%temperatur%", String(temperatur));
    htmlResponse.replace("%luftdruck%", String(luftdruck));
    htmlResponse.replace("%luftfeuchtigkeit%", String(luftfeuchtigkeit));
    htmlResponse.replace("%Spannung12V%", String(Spannung12V));
    htmlResponse.replace("%Spannung24V%", String(Spannung24V));
    htmlResponse.replace("%StatusRelais1%", String(StatusRelais1));
    htmlResponse.replace("%StatusRelais3%", String(StatusRelais3));
    request->send(200, "text/html", htmlResponse);
  });

  server.on("/get", HTTP_GET, [](AsyncWebServerRequest *request){
    if(request->hasParam(PARAM_DatumZeit)){
      DatumZeit = request->getParam(PARAM_DatumZeit)->value();
    }
    if(request->hasParam(PARAM_FLOAT12Van)){
      Vorgabe12Van = request->getParam(PARAM_FLOAT12Van)->value().toFloat();
    }
    if(request->hasParam(PARAM_FLOAT12Vaus)){
      Vorgabe12Vaus = request->getParam(PARAM_FLOAT12Vaus)->value().toFloat();
    }
    if(request->hasParam(PARAM_FLOAT24Van)){
      Vorgabe24Van = request->getParam(PARAM_FLOAT24Van)->value().toFloat();
    }
    if(request->hasParam(PARAM_FLOAT24Vaus)){
      Vorgabe24Vaus = request->getParam(PARAM_FLOAT24Vaus)->value().toFloat();
    }
    if(request->hasParam(PARAM_Temperatur)){
      temperatur = request->getParam(PARAM_Temperatur)->value().toFloat();
    }
    if(request->hasParam(PARAM_Luftdruck)){
      luftdruck = request->getParam(PARAM_Luftdruck)->value().toFloat();
    }
    if(request->hasParam(PARAM_Luftfeuchtigkeit)){
      luftfeuchtigkeit = request->getParam(PARAM_Luftfeuchtigkeit)->value().toFloat();
    }
    if(request->hasParam(PARAM_Spannung12V)){
      Spannung12V = request->getParam(PARAM_Spannung12V)->value().toFloat();
    }
    if(request->hasParam(PARAM_Spannung24V)){
      Spannung24V = request->getParam(PARAM_Spannung24V)->value().toFloat();
    }
    if(request->hasParam(PARAM_StatusRelais1)){
      StatusRelais1 = request->getParam(PARAM_StatusRelais1)->value();
    }
    if(request->hasParam(PARAM_StatusRelais3)){
      StatusRelais3 = request->getParam(PARAM_StatusRelais3)->value();
    }
    request->send(200, "text/plain", "OK");
  });

  server.onNotFound(notFound);
  server.begin();
  Serial.println("Programmstart, schalte alle Relais aus.");
  pinMode(Relais1Pin, OUTPUT);
  pinMode(Relais2Pin, OUTPUT);
  pinMode(Relais3Pin, OUTPUT);
  pinMode(Relais4Pin, OUTPUT);
  Relais1und2aus();
  Relais3und4aus();
  Serial.println("Alle Relais ausgeschaltet");
  xTaskCreatePinnedToCore(NTPTask, "NTP_Task", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(bmeTask, "BME280_Task", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(analogTask, "Analog_Task", 4096, NULL, 1, NULL, 1);
}

void loop() {
  verbindungsversuch();
}
