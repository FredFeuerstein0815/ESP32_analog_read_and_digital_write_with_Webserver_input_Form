Mit diesem Script liest ein ESP32 analoge Werte und schaltet, in Abhängigkeit dieser Werte, GPIOs an oder aus.

Die vorgegebenen Werte lassen sich auf der HTML-Seite des eingebauten Webservers modifizieren. Dort werden auch die aktuellen Spannungen angezeigt.

Die Zeit wird von einem NTP-Server geholt und auch auf der HTML-Seite angezeigt. Der Zeitraum, wie oft sich der ESP32 die Uhrzeit vom NTP-Server holt, ist einstellbar, ebenso der refresh der HTML-Seite.

Zudem werden die Werte Temperatur, Luftdruck und Luftfeuchtigkeit eines BME280 ausgelesen und angezeigt.

Es wird Threading benutzt, so dass die Auslesezeiten für den BME280 und der analogen Eingänge des ESP32 unterschiedlich sein können.

Vorsicht Überspannung !
Benutze einen Spannungsteiler, damit die Spannung am GPIO nicht über 3,3 Volt geht. Vermeide Spannungen von 2.8 Volt oder höher, da die analogen Eingänge des ESP32 bei höheren Spannungen nicht linear sind.
Das angeschlossene Relaisboard schaltet das Relais mit LOW ein und mit HIGH aus. Bei Nutzung von MOSFETs zum schalten einer Last muss die Logik umgekehrt werden.

Beispiel eines Spannungsteilers mit empfohlener Eingangsspannung von maximal 15 Volt und einem absolutem Maximum von 19 Volt Eingangsspannung:

![](https://github.com/FredFeuerstein0815/ESP32_analog_read_and_digital_write_with_Webserver_input_Form/blob/main/Spannungsteiler_Beispiel.png)
![](https://github.com/FredFeuerstein0815/ESP32_analog_read_and_digital_write_with_Webserver_input_Form/blob/main/analogread_digitalwrite_webserver_form.png)
![](https://github.com/FredFeuerstein0815/ESP32_analog_read_and_digital_write_with_Webserver_input_Form/blob/main/ESP32_analogread_digitalwrite_Webserver_NTP_BME280_Smartphone.jpg)
