# ESP-DHT22-Server-Sensor
ESP-DHT22 Sensor und Webserver

Sensor:
Code for the sensor with an ESP-01 and a DHT22 Module. GPIO2 is used to receive Data from the sensor.
The ESP sends every 30 minutes to a webserver at a fixed ip 192.168.1.50
The SSID and PW are hardcoded and need to be adjusted

Webserver:
The Webserver runs at a fixed IP 192.168.1.50
The Webserver displays multiple sensors from different IPs
The SSID and PW are hardcoded and need to be adjusted
