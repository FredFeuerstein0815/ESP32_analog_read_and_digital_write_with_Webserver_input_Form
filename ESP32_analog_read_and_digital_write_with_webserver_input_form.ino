#include <Adafruit_BME280.h>
#include <Adafruit_Sensor.h>
#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <Wire.h>

Adafruit_BME280 bme;

const int BattPin12V = 36;
const int BattPin24V = 39;
const int Relais1Pin = 13;
const int Relais2Pin = 12;
const int Relais3Pin = 14;
const int Relais4Pin = 27;

const float umrechnungsfaktor12V = 174.867;
const float umrechnungsfaktor24V = 90.73;

int StatusRelais1 = 0;
int StatusRelais2 = 0;
int StatusRelais3 = 0;
int StatusRelais4 = 0;

int WertPin12V = 0;
int WertPin24V = 0;

IPAddress local_IP(192, 168, 0, 99);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);
IPAddress secondaryDNS(8, 8, 4, 4);

AsyncWebServer server(80);

const char* ssid = "SSID";
const char* password = "Geheim";

// Parameter-Namen
const char* PARAM_FLOAT12Van = "Vorgabe12Van";
const char* PARAM_FLOAT12Vaus = "Vorgabe12Vaus";
const char* PARAM_FLOAT24Van = "Vorgabe24Van";
const char* PARAM_FLOAT24Vaus = "Vorgabe24Vaus";
const char* PARAM_Temperatur = "temperatur";
const char* PARAM_Luftdruck = "luftdruck";
const char* PARAM_Luftfeuchtigkeit = "luftfeuchtigkeit";
const char* PARAM_Spannung12V = "Spannung12V";
const char* PARAM_Spannung24V = "Spannung24V";

// Globale Platzhalter
float Vorgabe12Van = 14.0;
float Vorgabe12Vaus = 13.2;
float Vorgabe24Van = 28.0;
float Vorgabe24Vaus = 26.4;
float temperatur = 19.9;
float luftdruck = 999.9;
float luftfeuchtigkeit = 50.0;
float Spannung12V = 9.99;
float Spannung24V = 19.99;

// Funktion, um die HTML-Seite zu generieren, z.B. mit Platzhaltern
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
  <meta http-equiv="cache-control" content="no-cache" />
  <meta http-equiv="pragma" content="no-cache" />
  <meta http-equiv="expires" content="-1" />
  <meta http-equiv="refresh" content="60" />
  <title>Ein- Ausschaltspannung</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
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
  <form action="/get" target="hidden-form">
  <tabel style="font-size:20px; border:1px solid grey;">
  <tr>
  <td style="font-size:20px;">Spannung 12 Volt ist:</td>
  <script>document.write(%Spannung12V%);
  </script>
  </tr>
  </table>
  <br>
  <tabel style="font-size:20px; border:1px solid grey;">
  <tr>
  <td style="font-size:20px;">Einschaltspannung 12 Volt ist:</td>
  <script>document.write(%Vorgabe12Van%);
  </script>
  </tr>
  </table>
  <h2 style="font-size:20px">Einschaltspannung 12 Volt soll:</h2>
  <input style="font-size:20px; width:60px;" value="%Vorgabe12Van%" size="8" type="number" step="0.1" name="Vorgabe12Van" min="12" max="15">
  </input>
  <input style="font-size:20px;" type="submit" value="Senden" onclick="submit12Van()">
  </input>
  </form>
  <br><br><br>
  <form action="/get" target="hidden-form">
  <tabel style="font-size:20px; border:1px solid grey;">
  <tr>
  <td style="font-size:20px;">Ausschaltspannung 12 Volt ist:</td>
  <script>document.write(%Vorgabe12Vaus%);
  </script>
  </tr>
  </table>
  <h2 style="font-size:20px;">Ausschaltspannung 12 Volt soll:</h2>
  <input style="font-size:20px; width: 60px;" value="%Vorgabe12Vaus%" size="8" type="number" step="0.1" name="Vorgabe12Vaus" min="12" max="15">
  </input>
  <input style="font-size:20px;" type="submit" value="Senden" onclick="submit12Vaus()">
  </input>
  </form>
  <br><br><br>
  <form action="/get" target="hidden-form">
  <tabel style="font-size:20px; border:1px solid grey;">
  <tr>
  <td style="font-size:20px;">Spannung 24 Volt ist:</td>
  <script>document.write(%Spannung24V%);
  </script>
  </tr>
  </table>
  <br>
  <tabel style="font-size:20px; border:1px solid grey;">
  <tr>
  <td style="font-size:20px;">Einschaltspannung 24 Volt ist:</td>
  <script>document.write(%Vorgabe24Van%);
  </script>
  </tr>
  </table>
  <h2 style="font-size:20px;">Einschaltspannung 24 Volt soll:</h2>
  <input style="font-size:20px; width: 60px;" value="%Vorgabe24Van%" size="8" type="number" step="0.1" name="Vorgabe24Van" min="24" max="30">
  </input>
  <input style="font-size:20px;" type="submit" value="Senden" onclick="submit24Van()">
  </input>
  </form>
  <br><br><br>
  <form action="/get" target="hidden-form">
  <tabel style="font-size:20px; border:1px solid grey;">
  <tr>
  <td style="font-size:20px;">Ausschaltspannung 24 Volt ist:</td>
  <script>document.write(%Vorgabe24Vaus%);
  </script>
  </tr>
  </table>
  <h2 style="font-size:20px;">Ausschaltspannung 24 Volt soll:</h2>
  <input style="font-size:20px; width: 60px;" value="%Vorgabe24Vaus%" size="8" type="number" step="0.1" name="Vorgabe24Vaus" min="24" max="30">
  </input>
  <input style="font-size:20px;" type="submit" value="Senden" onclick="submit24Vaus()">
  </input>
  </form>
  <br><br><br>
  <tabel style="font-size:20px; border:1px solid grey;" colums="3">
  <tr>
  <td style="font-size:20px;">Temperatur</td>
  <td style="font-size:20px;">
  <script>document.write(%temperatur%);
  </script>
  </td>
  <td style="font-size:20px;">°C</td>
  </tr>
  </tabel>
  <br>
  <tabel style="font-size:20px; border:1px solid grey;" colums="3">
  <tr>
  <td style="font-size:20px;">Luftdruck</td>
  <script>document.write(%luftdruck%);
  </script>
  </td>
  <td style="font-size:20px;">hPa</td>
  </tr>
  </tabel>
  <br>
  <tabel style="font-size:20px; border:1px solid grey;" colums="3">
  <tr>
  <td style="font-size:20px;">Luftfeuchtigkeit</td>
  <script>document.write(%luftfeuchtigkeit%);
  </script>
  </td>
  <td style="font-size:20px;">%</td>
  </tr>
  </tabel>
  <br>
  </center>
  <iframe style="display:none" name="hidden-form"></iframe>
</body></html>)rawliteral";

// Funktion, um die HTML-Seite zu generieren und Platzhalter zu ersetzen
String processor(const String& var){
  if(var == "Vorgabe12Van"){
    return String(Vorgabe12Van);
  }
  else if(var == "Vorgabe12Vaus"){
    return String(Vorgabe12Vaus);
  }
  else if(var == "Vorgabe24Van"){
    return String(Vorgabe24Van);
  }
  else if(var == "Vorgabe24Vaus"){
    return String(Vorgabe24Vaus);
  }
    else if(var == "temperatur"){
    return String(temperatur);
  }
    else if(var == "luftdruck"){
    return String(luftdruck);
  }
    else if(var == "luftfeuchtigkeit"){
    return String(luftfeuchtigkeit);
  }
    else if(var == "Spannung12V"){
    return String(Spannung12V);
  }
    else if(var == "Spannung24V"){
    return String(Spannung24V);
  }
  return String();
}

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "404: Not found");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Start");
  WiFi.mode(WIFI_STA);
  WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS);
  Serial.print("Verbindung zu WLAN: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  Serial.println("WLAN-Verbindung wird hergestellt...");
  
  int max_attempts = 20;
  while (WiFi.status() != WL_CONNECTED && max_attempts--) {
    delay(1000);
    Serial.print(".");
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to WiFi");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect");
  }
  bme.begin(0x76);
  // Webserver starten
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String htmlResponse = index_html;
    // Platzhalter durch Variablen ersetzen
    htmlResponse.replace("%Vorgabe12Van%", String(Vorgabe12Van));
    htmlResponse.replace("%Vorgabe12Vaus%", String(Vorgabe12Vaus));
    htmlResponse.replace("%Vorgabe24Van%", String(Vorgabe24Van));
    htmlResponse.replace("%Vorgabe24Vaus%", String(Vorgabe24Vaus));
    htmlResponse.replace("%temperatur%", String(temperatur));
    htmlResponse.replace("%luftdruck%", String(luftdruck));
    htmlResponse.replace("%luftfeuchtigkeit%", String(luftfeuchtigkeit));
    htmlResponse.replace("%Spannung12V%", String(Spannung12V));
    htmlResponse.replace("%Spannung24V%", String(Spannung24V));
    request->send(200, "text/html", htmlResponse);
  });

  server.on("/get", HTTP_GET, [](AsyncWebServerRequest *request) {
    if(request->hasParam(PARAM_FLOAT12Van)) {
      String value = request->getParam(PARAM_FLOAT12Van)->value();
      Vorgabe12Van = value.toFloat();
    }
    if(request->hasParam(PARAM_FLOAT12Vaus)) {
      String value = request->getParam(PARAM_FLOAT12Vaus)->value();
      Vorgabe12Vaus = value.toFloat();
    }
    if(request->hasParam(PARAM_FLOAT24Van)) {
      String value = request->getParam(PARAM_FLOAT24Van)->value();
      Vorgabe24Van = value.toFloat();
    }
    if(request->hasParam(PARAM_FLOAT24Vaus)) {
      String value = request->getParam(PARAM_FLOAT24Vaus)->value();
      Vorgabe24Vaus = value.toFloat();
    }
    if(request->hasParam(PARAM_Temperatur)) {
      String value = request->getParam(PARAM_Temperatur)->value();
      temperatur = value.toFloat();
    }
    if(request->hasParam(PARAM_Luftdruck)) {
      String value = request->getParam(PARAM_Luftdruck)->value();
      luftdruck = value.toFloat();
    }
    if(request->hasParam(PARAM_Luftfeuchtigkeit)) {
      String value = request->getParam(PARAM_Luftfeuchtigkeit)->value();
      luftfeuchtigkeit = value.toFloat();
    }
      if(request->hasParam(PARAM_Spannung12V)) {
      String value = request->getParam(PARAM_Spannung12V)->value();
      Spannung12V = value.toFloat();
    }
      if(request->hasParam(PARAM_Spannung24V)) {
      String value = request->getParam(PARAM_Spannung24V)->value();
      Spannung24V = value.toFloat();
    }
    request->send(200, "text/plain", "OK");
  });

  server.onNotFound(notFound);
  server.begin();
  Serial.println("Webserver gestartet");
  pinMode(Relais1Pin, OUTPUT);
  pinMode(Relais2Pin, OUTPUT);
  pinMode(Relais3Pin, OUTPUT);
  pinMode(Relais4Pin, OUTPUT);
  digitalWrite(Relais1Pin, HIGH);
  digitalWrite(Relais2Pin, HIGH);
  digitalWrite(Relais3Pin, HIGH);
  digitalWrite(Relais4Pin, HIGH);
  Serial.println("Alle Relais ausgeschaltet");
}

void loop() {
  temperatur = bme.readTemperature();
  luftdruck = bme.readPressure() / 100.0F; // hPa
  luftfeuchtigkeit = bme.readHumidity();
  // Werte in Strings umwandeln
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
  Serial.print("\nEinschaltspannung 12 Volt: ");
  Serial.println(Vorgabe12Van);
  Serial.print("Ausschaltspannung 12 Volt: ");
  Serial.println(Vorgabe12Vaus);
  Serial.print("Einschaltspannung 24 Volt: ");
  Serial.println(Vorgabe24Van);
  Serial.print("Ausschaltspannung 24 Volt: ");
  Serial.println(Vorgabe24Vaus);
  StatusRelais1 = digitalRead(Relais1Pin);
  StatusRelais2 = digitalRead(Relais2Pin);
  StatusRelais3 = digitalRead(Relais3Pin);
  StatusRelais4 = digitalRead(Relais4Pin);
  Serial.print("\nRelais1 ist ");
  if (StatusRelais1 == 1) {
  Serial.print("aus\n");
  }
  else if (StatusRelais1 == 0) {
    Serial.print("an\n");
  }
  Serial.print("Relais2 ist ");
    if (StatusRelais2 == 1) {
  Serial.print("aus\n");
  }
  else if (StatusRelais2 == 0) {
  Serial.print("an\n");
  }
  Serial.print("Relais3 ist ");
  if (StatusRelais3 == 1) {
  Serial.print("aus\n");
  }
  else if (StatusRelais3 == 0) {
  Serial.print("an\n");
  }
  Serial.print("Relais4 ist ");
  if (StatusRelais4 == 1) {
  Serial.print("aus\n");
  }
  else if (StatusRelais4 == 0) {
  Serial.print("an\n");
  }
  WertPin12V = analogRead(BattPin12V);
  Serial.print("\nanaloger Wert 12 Volt:");
  Serial.print(WertPin12V);
  Spannung12V = WertPin12V/umrechnungsfaktor12V;
  if (Spannung12V < 3) {
    Serial.println("\nDas 12 Volt-System ist nicht angeschlossen oder die Batterie ist tot\n");
  }
  else {
  Serial.print("\nSpannung 12 Volt:");
  Serial.println(Spannung12V);
  if ((Spannung12V >= Vorgabe12Van) && (StatusRelais1 == LOW)){
    Serial.println("Spannung hoch, Strom ist an und bleibt an.\n");
  }
   else if ((Spannung12V >= Vorgabe12Van) && (StatusRelais1 == HIGH)) {
    Serial.println("Spannung hoch, schalte Strom ein.\n");
    digitalWrite(Relais1Pin, LOW);
    digitalWrite(Relais2Pin, LOW);
  }
    else if ((Spannung12V <= Vorgabe12Vaus) && (StatusRelais1 == HIGH)) {
    Serial.println("Spannung zu niedig, Strom bleibt aus.\n");
  }
  else if ((Spannung12V <= Vorgabe12Van) && (Spannung12V >= Vorgabe12Vaus) && (StatusRelais1 == LOW)) {
    Serial.println("Dazwischen, Strom bleibt an.\n");
  }
  else if ((Spannung12V <= Vorgabe12Vaus) && (StatusRelais1 == LOW)) {
    Serial.println("Spannung niedrig, schalte Strom aus.\n");
    digitalWrite(Relais1Pin, HIGH);
    digitalWrite(Relais2Pin, HIGH);
  }
  else if ((Spannung12V >= Vorgabe12Vaus) && (Spannung12V <= Vorgabe12Van) && (StatusRelais1 == HIGH)){
    Serial.println("Dazwischen, Strom bleibt aus.\n");
  }
  else if ((Spannung12V >= Vorgabe12Van) && (StatusRelais1 == HIGH)) {
    Serial.println("Spannung hoch genug, schalte Strom an.");
    digitalWrite(Relais1Pin, LOW);
    digitalWrite(Relais2Pin, LOW);
  }
  else {
    Serial.println("Das darf nicht passieren !!!");
  }
  }
  WertPin24V = analogRead(BattPin24V);
  Serial.print("analoger Wert 24 Volt:");
  Serial.print(WertPin24V);
  Spannung24V = WertPin24V/umrechnungsfaktor24V;
  if (Spannung24V < 3) {
    Serial.println("\nDas 24 Volt-System ist nicht angeschlossen oder die Batterie ist tot");
  }
  else{
  Serial.print("\nSpannung 24 Volt:");
  Serial.println(Spannung24V);
    if ((Spannung24V >= Vorgabe24Van) && (StatusRelais3 == LOW)){
    Serial.println("Spannung hoch, Strom ist an und bleibt an.\n");
  }
   else if ((Spannung24V >= Vorgabe24Van) && (StatusRelais3 == HIGH)) {
    Serial.println("Spannung hoch, schalte Strom ein.\n");
    digitalWrite(Relais3Pin, LOW);
    digitalWrite(Relais4Pin, LOW);
  }
    else if ((Spannung24V <= Vorgabe24Vaus) && (StatusRelais3 == HIGH)) {
    Serial.println("Spannung zu niedig, Strom bleibt aus.\n");
  }
  else if ((Spannung24V <= Vorgabe24Van) && (Spannung24V >= Vorgabe24Vaus) && (StatusRelais3 == LOW)) {
    Serial.println("Dazwischen, Strom bleibt an.\n");
  }
  else if ((Spannung24V <= Vorgabe24Vaus) && (StatusRelais3 == LOW)) {
    Serial.println("Spannung niedrig, schalte Strom aus.\n");
    digitalWrite(Relais3Pin, HIGH);
    digitalWrite(Relais4Pin, HIGH);
  }
  else if ((Spannung24V >= Vorgabe24Vaus) && (Spannung24V <= Vorgabe24Van) && (StatusRelais3 == HIGH)){
    Serial.println("Dazwischen, Strom bleibt aus.\n");
  }
  else if ((Spannung24V >= Vorgabe24Van) && (StatusRelais3 == HIGH)) {
    Serial.println("Spannung hoch genug, schalte Strom an.");
    digitalWrite(Relais3Pin, LOW);
    digitalWrite(Relais4Pin, LOW);
  }
  else {
    Serial.println("Das darf nicht passieren !!!");
  }
  }
  delay(20000);
}
