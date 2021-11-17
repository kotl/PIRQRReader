# PIRQRReader

A trivial project that connects PIR Sensor Module, ESPCam, QR Code Reader library and Bluetooth Serial together.

What does it do?

When PIR Sensor detects motion, it enables Flashlight (there must be wire added between PIN 4 and PIN 2) and while motion is detected, 
it constantly tries to scan QR code on the camera and report that over Bluetooth connection.

That's it.
