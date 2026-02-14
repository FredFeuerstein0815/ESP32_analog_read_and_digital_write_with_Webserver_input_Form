#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

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
float Spannung12V = 0;
float Spannung24V = 0;

// WLAN-Konfiguration
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

// Globale Variablen, um die Werte im RAM zu speichern
float Vorgabe12Van = 14.0;
float Vorgabe12Vaus = 13.0;
float Vorgabe24Van = 28.0;
float Vorgabe24Vaus = 26.4;

// Funktion, um die HTML-Seite zu generieren, z.B. mit Platzhaltern
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
  <meta http-equiv="cache-control" content="no-cache" />
  <meta http-equiv="pragma" content="no-cache" />
  <meta http-equiv="expires" content="-1" />
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
  <br>
  <h2 style="font-size:20px">Einschaltspannung 12 Volt:</h2>
  <br>
  <input style="font-size:20px; width:60px;" value="%Vorgabe12Van%" size="8" type="number" step="0.1" name="Vorgabe12Van" min="12" max="15">
  <br><br>
  <input style="font-size:20px;" type="submit" value="Senden" onclick="submit12Van()">
  </form><br>
  <form action="/get" target="hidden-form">
  <h2 style="font-size:20px;">Ausschaltspannung 12 Volt:</h2>
  <br>
  <input style="font-size:20px; width: 60px;" value="%Vorgabe12Vaus%" size="8" type="number" step="0.1" name="Vorgabe12Vaus" min="12" max="15">
  <br><br>
  <input style="font-size:20px;" type="submit" value="Senden" onclick="submit12Vaus()">
  </form><br>
  <form action="/get" target="hidden-form">
  <h2 style="font-size:20px;">Einschaltspannung 24 Volt:</h2>
  <br>
  <input style="font-size:20px; width: 60px;" value="%Vorgabe24Van%" size=8 type="number" step="0.1" name="Vorgabe24Van" min="24" max="30">
  <br><br>
  <input style="font-size:20px;" type="submit" value="Senden" onclick="submit24Van()">
  </form><br>
  <form action="/get" target="hidden-form">
  <h2 style="font-size:20px;">Ausschaltspannung 24 Volt:</h2>
  <br>
  <input style="font-size:20px; width: 60px;" value="%Vorgabe24Vaus%" size=8 type="number" step="0.1" name="Vorgabe24Vaus" min="24" max="30">
  <br><br>
  <input style="font-size:20px;" type="submit" value="Senden" onclick="submit24Vaus()">
  </form><br>
  </center>
  <iframe style="display:none" name="hidden-form"></iframe>
</body></html>)rawliteral";

// Keine Datei-Operationen mehr notwendig, da Variablen im RAM

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

  // Webserver starten
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String htmlResponse = index_html;
    // Platzhalter durch Variablen ersetzen
    htmlResponse.replace("%Vorgabe12Van%", String(Vorgabe12Van));
    htmlResponse.replace("%Vorgabe12Vaus%", String(Vorgabe12Vaus));
    htmlResponse.replace("%Vorgabe24Van%", String(Vorgabe24Van));
    htmlResponse.replace("%Vorgabe24Vaus%", String(Vorgabe24Vaus));
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
  Serial.println("Alles Relais ausgeschaltet");
}

void loop() {
  Serial.print("Einschaltspannung 12 Volt: ");
  Serial.println(Vorgabe12Van);
  Serial.print("Ausschaltspannung 12 Volt: ");
  Serial.println(Vorgabe12Vaus);
  Serial.print("Einschaltspannung 24 Volt: ");
  Serial.println(Vorgabe24Van);
  Serial.print("Ausschaltspannung 24 Volt: ");
  Serial.println(Vorgabe24Vaus);
  Serial.println("\n");
  StatusRelais1 = digitalRead(Relais1Pin);
  StatusRelais2 = digitalRead(Relais2Pin);
  StatusRelais3 = digitalRead(Relais3Pin);
  StatusRelais4 = digitalRead(Relais4Pin);
  Serial.println("Status Relais1:");
  Serial.println(StatusRelais1);
  Serial.println("Status Relais2:");
  Serial.println(StatusRelais2);
  Serial.println("Status Relais3:");
  Serial.println(StatusRelais3);
  Serial.println("Status Relais4:");
  Serial.println(StatusRelais4);
  Serial.println("\n");
  WertPin12V = analogRead(BattPin12V);
  Serial.println("analoger Wert 12 Volt:");
  Serial.println(WertPin12V);
  Spannung12V = WertPin12V/umrechnungsfaktor12V;
  Serial.print("\nSpannung 12 Volt:");
  Serial.println(Spannung12V);
  if ((Spannung12V >= Vorgabe12Van) && (StatusRelais1 == LOW)){
    Serial.println("Spannung hoch, Strom ist an und bleibt an.\n");
  }
   else if ((Spannung12V >= Vorgabe12Van) && (StatusRelais1 == HIGH)) {
    Serial.println("Spannung hoch, schalte Strom ein.\n");
    digitalWrite(Relais1Pin, LOW);
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
  }
  else if ((Spannung12V >= Vorgabe12Vaus) && (Spannung12V <= Vorgabe12Van) && (StatusRelais1 == HIGH)){
    Serial.println("Dazwischen, Strom bleibt aus.\n");
  }
  else if ((Spannung12V >= Vorgabe12Van) && (StatusRelais1 == HIGH)) {
    Serial.println("Spannung hoch genug, schalte Strom an.");
  }
  else {
    Serial.println("Das darf nicht passieren !!!");
  }
  WertPin24V = analogRead(BattPin24V);
  Serial.println("analoger Wert 24 Volt:");
  Serial.println(WertPin24V);
  Spannung24V = WertPin24V/umrechnungsfaktor24V;
  Serial.print("\nSpannung 24 Volt:");
  Serial.println(Spannung24V);
    if ((Spannung24V >= Vorgabe24Van) && (StatusRelais3 == LOW)){
    Serial.println("Spannung hoch, Strom ist an und bleibt an.\n");
  }
   else if ((Spannung24V >= Vorgabe24Van) && (StatusRelais3 == HIGH)) {
    Serial.println("Spannung hoch, schalte Strom ein.\n");
    digitalWrite(Relais1Pin, LOW);
  }
    else if ((Spannung24V <= Vorgabe24Vaus) && (StatusRelais3 == HIGH)) {
    Serial.println("Spannung zu niedig, Strom bleibt aus.\n");
  }
  else if ((Spannung24V <= Vorgabe24Van) && (Spannung24V >= Vorgabe24Vaus) && (StatusRelais3 == LOW)) {
    Serial.println("Dazwischen, Strom bleibt an.\n");
  }
  else if ((Spannung24V <= Vorgabe24Vaus) && (StatusRelais3 == LOW)) {
    Serial.println("Spannung niedrig, schalte Strom aus.\n");
    digitalWrite(Relais1Pin, HIGH);
  }
  else if ((Spannung24V >= Vorgabe24Vaus) && (Spannung24V <= Vorgabe24Van) && (StatusRelais3 == HIGH)){
    Serial.println("Dazwischen, Strom bleibt aus.\n");
  }
  else if ((Spannung24V >= Vorgabe24Van) && (StatusRelais3 == HIGH)) {
    Serial.println("Spannung hoch genug, schalte Strom an.");
    digitalWrite(Relais1Pin, LOW);
  }
  else {
    Serial.println("Das darf nicht passieren !!!");
  }
  Serial.println("\n");
  delay(20000);
}
