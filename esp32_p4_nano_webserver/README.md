ESP32-P4 Nano Web Server Demo
=============================

This sketch creates a simple web server with 3 pages:

- Home (/)
- Info (/info)
- Control (/control)

Setup
-----

1) Open `esp32_p4_nano_webserver.ino` in Arduino IDE.
2) Set `WIFI_SSID` and `WIFI_PASS` to your network.
3) Select the ESP32 board that matches your ESP32-P4 Nano.
4) Upload the sketch and open Serial Monitor (115200).
5) Open the IP address shown in Serial Monitor.

Notes
-----

- The `/toggle` endpoint flips a demo state and redirects to /control.
- Replace the demo toggle with real GPIO control as needed.
