# WebPixelFrame
Code to control a ESP8266 and a matrix of ws2812b pixels.  

Requires these libraries:

- [NeoPixelBus](https://github.com/Makuna/NeoPixelBus)
- [NTPClient](https://github.com/arduino-libraries/NTPClient)
- [ESPAsyncTCP](https://github.com/me-no-dev/ESPAsyncTCP)
- [ESP Async Web Server](https://github.com/me-no-dev/ESPAsyncWebServer)
- [ESPAsyncWiFiManager](https://github.com/alanswx/ESPAsyncWiFiManager)

Piskel is a submodule. Make sure to clone the repo with --recursive:

``git clone --recursive git://github.com/alanswx/WebPixelFrame.git``


[New Mac driver for Sierra](https://tzapu.com/ch340-ch341-serial-adapters-macos-sierra/)

## Notes:

Make sure the pixel display on ESP8266 plugs into the RX pin - that is where the DMA is.

Make sure the SPIFFS is larger than 1M
