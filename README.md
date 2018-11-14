# WebPixelFrame
This repository provides the code to control a ESP8266 and a matrix of ws2812b pixels for the Web Pixel Frame. In order to properly install the software on the WeMo device, you will need to both install the software & libraries needed, and also update the Arduino IDE to be able to program the WeMo board.

## Software and Library Installation
Requires these libraries:

- [NeoPixelBus](https://github.com/Makuna/NeoPixelBus) - This can be installed by the library manager in the Arduino app.
- [NTPClient](https://github.com/arduino-libraries/NTPClient) - This can be installed by the library manager in the Arduino app.
- [ESPAsyncTCP](https://github.com/me-no-dev/ESPAsyncTCP) - Install by cloning repository (see below)
- [ESP Async Web Server](https://github.com/me-no-dev/ESPAsyncWebServer) - Install by cloning repository (see below)
- [ESPAsyncWiFiManager](https://github.com/alanswx/ESPAsyncWiFiManager) - Install by cloning repository (see below)

In order to install an Arduino library by cloning the Github Repository, in your command line navigate to the Arduino libraries directory. On a Mac it can be found at this path:

```
<home directory>/Documents/Arduino/libraries/
```

Then you can simply clone each of the required libraries:
```
git clone git@github.com:me-no-dev/ESPAsyncTCP.git
git clone git@github.com:me-no-dev/ESPAsyncWebServer.git
git clone git@github.com:alanswx/ESPAsyncWiFiManager.git
```
Restart the Arduino IDDE to activate these new libraries.

[New Mac driver for Sierra](https://tzapu.com/ch340-ch341-serial-adapters-macos-sierra/)

Next, install the Web Pixel Frame software repository anywhere else (not in the Arduino libraries folder!) simply by cloning the repository. Since Piskel is a submodule, make sure to clone the repository with `--recursive` option:

```
git clone --recursive git://github.com/alanswx/WebPixelFrame.git
```

If you are using a Mac, please be sure to install the [USB driver for the WeMo](https://tzapu.com/ch340-ch341-serial-adapters-macos-sierra/).

## Arduino IDE Update
If you Arduino IDE is not already updated to program the WeMo board, follow these steps:

- [Install WeMo Support](https://wiki.wemos.cc/tutorials:get_started:get_started_in_arduino) - Follow the instructions at this page to install get the Arduino app able to program WeMo D1 Mini boards.
- [Install SPIFFS Uploader](https://github.com/esp8266/Arduino/blob/master/doc/filesystem.rst#uploading-files-to-file-system) - This project requires this tool to uploaded the web page code to the web server we install on the WeMo board.
>>>>>>> 965b4f2aa20fff3ee15e92bcbca546d158ce5cfa


## Notes:

Make sure the pixel display on ESP8266 plugs into the RX pin - that is where the DMA is.

Make sure the SPIFFS is larger than 1M


