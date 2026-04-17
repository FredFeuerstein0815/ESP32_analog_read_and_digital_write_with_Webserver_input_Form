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

IPAddress Client_IP(192, 168, 0, 99);
IPAddress Client_gateway(192, 168, 0, 1);
IPAddress Client_subnet(255, 255, 255, 0);
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

int Verbindungsversuche = 5;
String StatusRelais1;
String StatusRelais2;
String StatusRelais3;
String StatusRelais4;

int WertPin12V = 0;
int WertPin24V = 0;

const uint32_t IntervallNTP = 42000000; // 11 Std 40 Minuten, absolutes Maximum = 42949672 sonst Overflow

const int httpsPort = 443;
const int DST_Offset_sec = 3600; // 1 Std
const int MessintervallBatt = 200000;  // 20 Sek
const int MessintervallBME280 = 1800000; // 30 Min
const int BattPin12V = 36;
const int BattPin24V = 39;
const int Relais1Pin = 14;
const int Relais2Pin = 27;
const int Relais3Pin = 26;
const int Relais4Pin = 25;

const float umrechnungsfaktor12V = 174.867;
const float umrechnungsfaktor24V = 96.85;

const long  TZ_Offset_sec = 3600; //GMT OFFSET DE +1Std (3600 SEC)
//const char* uploadserver = "meine-domain.de"; // meine-domain.de oder
const char* uploadserver = "192.168.0.10"; // IP des Hosts
const char* apiKey = "Geheim"; // derselbe apiKey muss in der upload_esp32.php eingetragen sein
const char* PARAM_DatumZeit = "DatumZeit";
const char* Client_ssid = "SSID";
const char* Client_password = "Geheim";
const char* Hostname = "ESP32";
const char* AP_ssid = "AP_SSID";
const char* AP_password = "AP_Geheim";
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
  Serial.println("Verbinde zu HTTPS...");
  if (!client.connect(uploadserver, httpsPort)) {
    Serial.println("Verbindung fehlgeschlagen!");
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
    temperatur = bme.readTemperature();
    luftdruck = bme.readPressure() / 100.0F; // hPa
    luftfeuchtigkeit = bme.readHumidity();
    char tempchar[8], druckchar[8], feuchtchar[8];
    dtostrf(temperatur, 1, 2, tempchar);
    dtostrf(luftdruck, 1, 2, druckchar);
    dtostrf(luftfeuchtigkeit, 1, 2, feuchtchar);
    Serial.print("\nTemperatur : ");
    Serial.print(tempchar);
    Serial.print(" °C\n");
    Serial.print("Luftdruck : ");
    Serial.print(druckchar);
    Serial.print(" hPa\n");
    Serial.print("Luftfeuchtigkeit : ");
    Serial.print(feuchtchar);
    Serial.print(" %\n");
    String tempString = String(tempchar);
    String druckString = String(druckchar);
    String feuchtString = String(feuchtchar);
    payload_esp32 = String(tempchar) + ";" + String(druckchar) + ";" + String(feuchtchar) + ";" + String(Spannung12V) + ";" + String(Spannung24V) + ";" + String(StatusRelais1) + ";" + String(StatusRelais3) + "\n";
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
      Serial.println( str + "Spannung ist " + Spannung12V + ", also unter " + Vorgabe12Vaus + ", Strom ist aus und bleibt aus.");
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
      Serial.println( str + "Spannung ist " + Spannung24V + ", also unter " + Vorgabe24Vaus + ", Strom ist aus und bleibt aus.");
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
  const TickType_t delay = pdMS_TO_TICKS(IntervallNTP);
  for(;;) {
    configTime(TZ_Offset_sec, DST_Offset_sec, NTP);
    Serial.println("Warte auf Zeitsynchronisation...");
    struct tm timeinfo;
    int retries = 10;
    while (retries-- > 0) {
    if (getLocalTime(&timeinfo)) {
      Serial.println("\nZeit synchronisiert");
    break;}
    }
    if (retries <= 0) {
      Serial.println("Zeit konnte nicht synchronisiert werden");
    }
    char timeStringBuff[20];
    strftime(timeStringBuff, sizeof(timeStringBuff), "%d.%m.%y %H:%M:%S", &timeinfo);
    DatumZeit = String(timeStringBuff);
    Serial.println(DatumZeit);
    vTaskDelay(delay);
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  WiFi.mode(WIFI_STA);
  WiFi.config(Client_IP, Client_gateway, Client_subnet, Client_primaryDNS, Client_secondaryDNS);
  Serial.print("Verbindung zu WLAN: ");
  Serial.println(Client_ssid);
  WiFi.begin(Client_ssid, Client_password);

  while (WiFi.status() != WL_CONNECTED && Verbindungsversuche--) {
    delay(1000);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nVerbunden");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nKeine Verbindung zum WLAN, eröffne AP");
    WiFi.softAP(AP_ssid, AP_password);
    Serial.print("Konfiguriere AP ... ");
    Serial.println(WiFi.softAPConfig(AP_IP, AP_gateway, AP_subnet));
    WiFi.softAPsetHostname(Hostname);
    Serial.print("Starte AP");
    Serial.print("AP SSID: ");
    Serial.println(WiFi.softAPSSID());
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
    Serial.print("AP Hostname: ");
    Serial.println(WiFi.softAPgetHostname());
    Serial.print("AP Mac: ");
    Serial.println(WiFi.softAPmacAddress());
    Serial.print("AP Subnet: ");
    Serial.println(WiFi.softAPSubnetCIDR());
  }

  bme.begin(0x76);
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
}
