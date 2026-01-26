ESP32-P4 Nano Web Server Demo
=============================

This sketch creates a simple web server with 3 pages:

- Home (/)
- Info (/info)
- Control (/control)

Setup
-----

1) Open `esp32_p4_nano_webserver.ino` in Arduino IDE.
2) Update the `ETH_*` constants to match your IP101 PHY wiring.
3) Connect the Ethernet cable to the board.
4) Select the ESP32 board that matches your ESP32-P4 Nano.
5) Upload the sketch and open Serial Monitor (115200).
6) Open the IP address shown in Serial Monitor.

Notes
-----

- The `/toggle` endpoint flips a demo state and redirects to /control.
- This example uses the internal IP101 PHY via Ethernet (no WiFi).
- Replace the demo toggle with real GPIO control as needed.
