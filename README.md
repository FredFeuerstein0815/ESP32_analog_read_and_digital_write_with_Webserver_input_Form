Mit diesem Script liest ein ESP32 analoge Werte und schaltet in Abhängigkeit dieser Werte GPIOs an oder aus.
Die vorgegebenen Werte lassen sich auf dem Webserver einstellen. (Bis jetzt nur ein Wert(, vom delay (vorletzte Zeile) abhängig) alle 20 Sekunden).
Vermeide Spannungen von 2.8 Volt oder höher, die analogen Eingänge des ESP32 sind nicht linear.

![](https://github.com/FredFeuerstein0815/ESP32_analog_read_and_digital_write_with_Webserver_input_Form/blob/main/analogread_digitalwrite_webserver_form.png)
