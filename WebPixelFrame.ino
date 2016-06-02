/* 
  FSWebServer - Example WebServer with SPIFFS backend for esp8266
  Copyright (c) 2015 Hristo Gochkov. All rights reserved.
  This file is part of the ESP8266WebServer library for Arduino environment.
 
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
  
  upload the contents of the data folder with MkSPIFFS Tool ("ESP8266 Sketch Data Upload" in Tools menu in Arduino IDE)
  or you can upload the contents of a folder if you CD in that folder and run the following command:
  for file in `ls -A1`; do curl -F "file=@$PWD/$file" esp8266fs.local/edit; done
  
  access the sample web page at http://esp8266fs.local
  edit the page by going to http://esp8266fs.local/edit
*/
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>
#include "NewBitmap.h"

#define DBG_OUTPUT_PORT Serial

const char* ssid = "Alan";
const char* password = "***REMOVED***";
const char* host = "esp8266fs";

// make sure to set these panel values to the sizes of yours
const uint8_t PanelWidth = 8;  // 8 pixel x 8 pixel matrix of leds
const uint8_t PanelHeight = 8;
const uint8_t TileWidth = 1;  // laid out in 4 panels x 2 panels mosaic
const uint8_t TileHeight = 1;

const uint16_t PixelCount = PanelWidth * PanelHeight * TileWidth * TileHeight;

 typedef RowMajorAlternatingLayout MyPanelLayout;
typedef NeoGrbFeature MyPixelColorFeature;

NeoMosaic <MyPanelLayout> mosaic(
    PanelWidth,
    PanelHeight,
    TileWidth,
    TileHeight);

uint16_t ourLayoutMapCallback(int16_t x, int16_t y)
{

  return mosaic.Map(x,y);

}
long setupStartMillis;
#include "C64FontUpper.h"
#include "C64FontLower.h"
#include "font5x7.h"
#define TOTALFONTS    3
#define FONTHEADERSIZE    6
const unsigned char *fontsPointer[TOTALFONTS] = {
   font5x7
  , C64FontUpper
  , C64FontLower 
  };
uint8_t foreColor, drawMode, fontWidth, fontHeight, fontType, fontStartChar, fontTotalChar, cursorX, cursorY;
uint16_t fontMapWidth;

const uint16_t AnimCount = 1; // we only need one

String message = "Hello World";
int textStartX=8;


void pixel(uint8_t x, uint8_t y,  RgbColor color, uint8_t mode);
uint8_t setFontType(uint8_t type);
int drawString(uint8_t x, uint8_t y, String text, RgbColor color);
int  drawChar(uint8_t x, uint8_t y, uint8_t c, RgbColor color, uint8_t mode) ;

NeoPixelBus<MyPixelColorFeature, Neo800KbpsMethod> strip(PixelCount, 2);
NeoPixelAnimator animations(AnimCount); // NeoPixel animation management object

// our NeoBitmapFile will use the same color feature as NeoPixelBus and
// we want it to use the SD File object 
NeoBitmapFileAl<MyPixelColorFeature,File> image;
RgbColor red(128, 0, 0);
RgbColor green(0, 128, 0);
RgbColor blue(0, 0, 128);
RgbColor white(128);
// if using NeoRgbwFeature above, use this white instead to use
// the correct white element of the LED
//RgbwColor white(128); 
RgbColor black(0);

ESP8266WebServer server(80);
//holds the current upload
File fsUploadFile;

//format bytes
String formatBytes(size_t bytes){
  if (bytes < 1024){
    return String(bytes)+"B";
  } else if(bytes < (1024 * 1024)){
    return String(bytes/1024.0)+"KB";
  } else if(bytes < (1024 * 1024 * 1024)){
    return String(bytes/1024.0/1024.0)+"MB";
  } else {
    return String(bytes/1024.0/1024.0/1024.0)+"GB";
  }
}

String getContentType(String filename){
  if(server.hasArg("download")) return "application/octet-stream";
  else if(filename.endsWith(".htm")) return "text/html";
  else if(filename.endsWith(".html")) return "text/html";
  else if(filename.endsWith(".css")) return "text/css";
  else if(filename.endsWith(".js")) return "application/javascript";
  else if(filename.endsWith(".png")) return "image/png";
  else if(filename.endsWith(".gif")) return "image/gif";
  else if(filename.endsWith(".jpg")) return "image/jpeg";
  else if(filename.endsWith(".ico")) return "image/x-icon";
  else if(filename.endsWith(".xml")) return "text/xml";
  else if(filename.endsWith(".pdf")) return "application/x-pdf";
  else if(filename.endsWith(".zip")) return "application/x-zip";
  else if(filename.endsWith(".gz")) return "application/x-gzip";
    else if(filename.endsWith(".bmp")) return "image/x-windows-bmp";
  return "text/plain";
}

bool handleShow(String path){
    DBG_OUTPUT_PORT.println("handleShow: " + path);
     ESP.wdtFeed();
  File bitmapFile = SPIFFS.open(path, "r");
    if (!bitmapFile)
    {
        Serial.println("File open fail, or not present");
        // don't do anything more:
        return false;
    }
   ESP.wdtFeed();
    ESP.wdtFeed();
    // initialize the image with the file
    if (!image.Begin(bitmapFile))
    {
        Serial.println("File format fail, not a supported bitmap");
        // don't do anything more:
        return false;
    }
  server.send(200, "text/plain", "");
  return true;

}
bool handleFileRead(String path){
  DBG_OUTPUT_PORT.println("handleFileRead: " + path);
  if(path.endsWith("/")) path += "index.htm";
  String contentType = getContentType(path);
  if (contentType=="image/x-windows-bmp")
    return handleShow(path);
  String pathWithGz = path + ".gz";
  if(SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)){
    if(SPIFFS.exists(pathWithGz))
      path += ".gz";
    File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}

void handleFileUpload(){
  if(server.uri() != "/edit") return;
  HTTPUpload& upload = server.upload();
  if(upload.status == UPLOAD_FILE_START){
    String filename = upload.filename;
    if(!filename.startsWith("/")) filename = "/"+filename;
    DBG_OUTPUT_PORT.print("handleFileUpload Name: "); DBG_OUTPUT_PORT.println(filename);
    fsUploadFile = SPIFFS.open(filename, "w");
    filename = String();
  } else if(upload.status == UPLOAD_FILE_WRITE){
    //DBG_OUTPUT_PORT.print("handleFileUpload Data: "); DBG_OUTPUT_PORT.println(upload.currentSize);
    if(fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize);
  } else if(upload.status == UPLOAD_FILE_END){
    if(fsUploadFile)
      fsUploadFile.close();
    DBG_OUTPUT_PORT.print("handleFileUpload Size: "); DBG_OUTPUT_PORT.println(upload.totalSize);
  }
}

void handleFileDelete(){
  if(server.args() == 0) return server.send(500, "text/plain", "BAD ARGS");
  String path = server.arg(0);
  DBG_OUTPUT_PORT.println("handleFileDelete: " + path);
  if(path == "/")
    return server.send(500, "text/plain", "BAD PATH");
  if(!SPIFFS.exists(path))
    return server.send(404, "text/plain", "FileNotFound");
  SPIFFS.remove(path);
  server.send(200, "text/plain", "");
  path = String();
}

void handleFileCreate(){
  if(server.args() == 0)
    return server.send(500, "text/plain", "BAD ARGS");
  String path = server.arg(0);
  DBG_OUTPUT_PORT.println("handleFileCreate: " + path);
  if(path == "/")
    return server.send(500, "text/plain", "BAD PATH");
  if(SPIFFS.exists(path))
    return server.send(500, "text/plain", "FILE EXISTS");
  File file = SPIFFS.open(path, "w");
  if(file)
    file.close();
  else
    return server.send(500, "text/plain", "CREATE FAILED");
  server.send(200, "text/plain", "");
  path = String();
}


void showImageList()
{
  Dir dir = SPIFFS.openDir("/");
 

  String output = "";
  while(dir.next()){
    File entry = dir.openFile("r");

    String contentType = getContentType(entry.name());
    if (contentType=="image/x-windows-bmp")
    {
      output+= String("<a href=\"")+String(entry.name())+"\">"+String(entry.name())+String("</a><br/>");
    }
    entry.close();
  }
  server.send(200, "text/html", output);
}

void setScrollText()
{
  if(!server.hasArg("text")) {server.send(500, "text/plain", "BAD ARGS"); return;}
  message = server.arg("text");
  textStartX=8;
  String output = "Setting scroll to: ";
  output+=message;
  
  server.send(200, "text/html", output);

}

void handleFileList() {
  if(!server.hasArg("dir")) {server.send(500, "text/plain", "BAD ARGS"); return;}
  
  String path = server.arg("dir");
  DBG_OUTPUT_PORT.println("handleFileList: " + path);
  Dir dir = SPIFFS.openDir(path);
  path = String();

  String output = "[";
  while(dir.next()){
    File entry = dir.openFile("r");
    if (output != "[") output += ',';
    bool isDir = false;
    output += "{\"type\":\"";
    output += (isDir)?"dir":"file";
    output += "\",\"name\":\"";
    output += String(entry.name()).substring(1);
    output += "\"}";
    entry.close();
  }
  
  output += "]";
  server.send(200, "text/json", output);
}

void setup(void){
  setupStartMillis = millis();
  DBG_OUTPUT_PORT.begin(115200);
  DBG_OUTPUT_PORT.print("\n");
  DBG_OUTPUT_PORT.setDebugOutput(true);
 setFontType(2);
 strip.Begin();
    strip.Show();
  
  SPIFFS.begin();
  {
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {    
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      DBG_OUTPUT_PORT.printf("FS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
    }
    DBG_OUTPUT_PORT.printf("\n");
  }
  

  //WIFI INIT
  DBG_OUTPUT_PORT.printf("Connecting to %s\n", ssid);
  if (String(WiFi.SSID()) != String(ssid)) {
    WiFi.begin(ssid, password);
  }
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    DBG_OUTPUT_PORT.print(".");
  }
  DBG_OUTPUT_PORT.println("");
  DBG_OUTPUT_PORT.print("Connected! IP address: ");
  DBG_OUTPUT_PORT.println(WiFi.localIP());

  MDNS.begin(host);
  DBG_OUTPUT_PORT.print("Open http://");
  DBG_OUTPUT_PORT.print(host);
  DBG_OUTPUT_PORT.println(".local/edit to see the file browser");
  
  
  //SERVER INIT
  server.on("/scroll",HTTP_GET,setScrollText);
  server.on("/image",HTTP_GET,showImageList);
  //list directory
  server.on("/list", HTTP_GET, handleFileList);
  //load editor
  server.on("/edit", HTTP_GET, [](){
    if(!handleFileRead("/edit.htm")) server.send(404, "text/plain", "FileNotFound");
  });
  //create file
  server.on("/edit", HTTP_PUT, handleFileCreate);
  //delete file
  server.on("/edit", HTTP_DELETE, handleFileDelete);
  //first callback is called after the request has ended with all parsed arguments
  //second callback handles file uploads at that location
  server.on("/edit", HTTP_POST, [](){ server.send(200, "text/plain", ""); }, handleFileUpload);


  //called when the url is not defined here
  //use it to load content from SPIFFS
  server.onNotFound([](){
    if(!handleFileRead(server.uri()))
      server.send(404, "text/plain", "FileNotFound");
  });

  //get heap status, analog input value and all GPIO statuses in one json call
  server.on("/all", HTTP_GET, [](){
    String json = "{";
    json += "\"heap\":"+String(ESP.getFreeHeap());
    json += ", \"analog\":"+String(analogRead(A0));
    json += ", \"gpio\":"+String((uint32_t)(((GPI | GPO) & 0xFFFF) | ((GP16I & 0x01) << 16)));
    json += "}";
    server.send(200, "text/json", json);
    json = String();
  });
  server.begin();
  DBG_OUTPUT_PORT.println("HTTP server started");

}

 
void loop(void){
  server.handleClient();
   image.Blt(strip,0,0,0,0,8,8,ourLayoutMapCallback);
   // drawString(textStartX,0,message,red);
  int now = millis();
   if (now -   setupStartMillis  >100000/1000 )
   {
    setupStartMillis=now;
   textStartX--;
     int end = message.length()*fontWidth * -1;
   if (textStartX <  end) textStartX = 8; 
   }
    strip.Show();
    
}



/** \brief Draw character with color and mode.
  Draw character c using color and draw mode at x,y.
*/
int  drawChar(uint8_t x, uint8_t y, uint8_t c, RgbColor color, uint8_t mode) {
  // TODO - New routine to take font of any height, at the moment limited to font height in multiple of 8 pixels

  uint8_t rowsToDraw, row, tempC;
  uint8_t i, j, temp;
  uint16_t charPerBitmapRow, charColPositionOnBitmap, charRowPositionOnBitmap, charBitmapStartPosition;

  if ((c < fontStartChar) || (c > (fontStartChar + fontTotalChar - 1))) // no bitmap for the required c
    return fontWidth;

  tempC = c - fontStartChar;

  // each row (in datasheet is call page) is 8 bits high, 16 bit high character will have 2 rows to be drawn
  rowsToDraw = fontHeight / 8; // 8 is LCD's page size, see SSD1306 datasheet
  if (rowsToDraw <= 1) rowsToDraw = 1;

  // the following draw function can draw anywhere on the screen, but SLOW pixel by pixel draw
  if (rowsToDraw == 1) {
    for  (i = 0; i < fontWidth + 1; i++) {
      if (i == fontWidth) // this is done in a weird way because for 5x7 font, there is no margin, this code add a margin after col 5
        temp = 0;
      else
        temp = pgm_read_byte(fontsPointer[fontType] + FONTHEADERSIZE + (tempC * fontWidth) + i);

      for (j = 0; j < 8; j++) { // 8 is the LCD's page height (see datasheet for explanation)
        if (temp & 0x1) {
          pixel(x + i, y + j, color, mode);

        }
        else {
          pixel(x + i, y + j, black, mode);

        }

        temp >>= 1;
      }
    }
    return fontWidth;
  }

  // font height over 8 bit
  // take character "0" ASCII 48 as example
  charPerBitmapRow = fontMapWidth / fontWidth; // 256/8 =32 char per row
  charColPositionOnBitmap = tempC % charPerBitmapRow; // =16
  charRowPositionOnBitmap = int(tempC / charPerBitmapRow); // =1
  charBitmapStartPosition = (charRowPositionOnBitmap * fontMapWidth * (fontHeight / 8)) + (charColPositionOnBitmap * fontWidth) ;

  // each row on LCD is 8 bit height (see datasheet for explanation)
  for (row = 0; row < rowsToDraw; row++) {
    for (i = 0; i < fontWidth; i++) {
      temp = pgm_read_byte(fontsPointer[fontType] + FONTHEADERSIZE + (charBitmapStartPosition + i + (row * fontMapWidth)));
      for (j = 0; j < 8; j++) { // 8 is the LCD's page height (see datasheet for explanation)
        if (temp & 0x1) {
          pixel(x + i, y + j + (row * 8), color, mode);

        }
        else {
          pixel(x + i, y + j + (row * 8), black, mode);

        }
        temp >>= 1;
      }
    }
  }

  /*
    fast direct memory draw but has a limitation to draw in ROWS
    // only 1 row to draw for font with 8 bit height
    if (rowsToDraw==1) {
    for (i=0; i<fontWidth; i++ ) {
      screenmemory[temp + (line*LCDWIDTH) ] = pgm_read_byte(fontsPointer[fontType]+FONTHEADERSIZE+(c*fontWidth)+i);
      temp++;
    }
    return;
    }
    // font height over 8 bit
    // take character "0" ASCII 48 as example
    charPerBitmapRow=fontMapWidth/fontWidth;  // 256/8 =32 char per row
    charColPositionOnBitmap=c % charPerBitmapRow;  // =16
    charRowPositionOnBitmap=int(c/charPerBitmapRow); // =1
    charBitmapStartPosition=(fontMapWidth * (fontHeight/8)) + (charColPositionOnBitmap * fontWidth);

    temp=x;
    for (row=0; row<rowsToDraw; row++) {
    for (i=0; i<fontWidth; i++ ) {
      screenmemory[temp + (( (line*(fontHeight/8)) +row)*LCDWIDTH) ] = pgm_read_byte(fontsPointer[fontType]+FONTHEADERSIZE+(charBitmapStartPosition+i+(row*fontMapWidth)));
      temp++;
    }
    temp=x;
    }
  */
 return fontWidth;
}

void pixel(uint8_t x, uint8_t y,  RgbColor color, uint8_t mode)
{
 // uint8_t i = (y * ledsPerStrip + x);
  if (x>= PanelWidth)
  {
    /* Serial.print("overflow:");
     Serial.print(x);
     Serial.print(",");
     Serial.println(y);
     */
     return;
  } else if (x<0)
  {
    Serial.println("less than 0");
    return;
  }
    strip.SetPixelColor(mosaic.Map(x, y), color);
 /*
Serial.print("setPixel:");
Serial.print(x);
Serial.print(",");
Serial.println(y);
*/

}

uint8_t setFontType(uint8_t type) {
  if ((type >= TOTALFONTS) || (type < 0))
    return false;

  fontType = type;
  fontWidth = pgm_read_byte(fontsPointer[fontType] + 0);
  fontHeight = pgm_read_byte(fontsPointer[fontType] + 1);
  fontStartChar = pgm_read_byte(fontsPointer[fontType] + 2);
  fontTotalChar = pgm_read_byte(fontsPointer[fontType] + 3);
  fontMapWidth = (pgm_read_byte(fontsPointer[fontType] + 4) * 100) + pgm_read_byte(fontsPointer[fontType] + 5); // two bytes values into integer 16
  return true;
}


int drawString(uint8_t x, uint8_t y, String text, RgbColor color)
{
  // start at x, and call drawChar for each.. increment x, and return it..
  int length = text.length();
  int last = x;
  int totalWidth = 0;
  for (int i =0; i < length; i++)
  {
     int width=drawChar(last,0, text.charAt(i), color,0);
     last+=width;
     totalWidth+=width;
  }
 return totalWidth; 
}

