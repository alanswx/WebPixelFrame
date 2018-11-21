
#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>

#include <ESPmDNS.h>

#define os_printf(...) Serial.printf( __VA_ARGS__ )




#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESP8266mDNS.h>

#if 0
 #ifdef DEBUG_ESP_PORT
   #define DEBUG_MSG(...) DEBUG_ESP_PORT.printf( __VA_ARGS__ )
 #else
   #define DEBUG_MSG(...)
 #endif

  #define os_printf(...) DEBUG_ESP_PORT.printf( __VA_ARGS__ )
#endif

#else
#error Platform not supported
#endif



#include <ArduinoOTA.h>
#include <FS.h>

//#include <ESPAsyncTCP.h>   // https://github.com/me-no-dev/ESPAsyncTCP
#include <ESPAsyncWebServer.h>  // https://github.com/me-no-dev/ESPAsyncWebServer/
#include <ESPAsyncWiFiManager.h>  // https://github.com/alanswx/ESPAsyncWiFiManager
#include <SPIFFSEditor.h>

#include <NeoPixelBus.h>   // https://github.com/Makuna/NeoPixelBus
#include <NTPClient.h>   // https://github.com/arduino-libraries/NTPClient
#include <DNSServer.h>

#include <libb64/cdecode.h>



#include "DisplayPixelsLive.h"
#include "DisplayPixelsText.h"
#include "DisplayPixelsAnimatedGIF.h"

#define DBG_OUTPUT_PORT Serial

NeoPixelBus<MyPixelColorFeature, Neo800KbpsMethod> *strip = new NeoPixelBus<MyPixelColorFeature, Neo800KbpsMethod> (PixelCount, 2);




#if 0

uint16_t ourLayoutMapCallback(int16_t x, int16_t y)
{

  return mosaic.Map(x, y);

}

#endif



#ifndef ESP32
//Use the internal hardware buffer
static void _u0_putc(char c) {
  while (((U0S >> USTXC) & 0x7F) == 0x7F);
  U0F = c;
}
#endif


class DisplayHandler: public AsyncWebHandler {
    DisplayPixelsText *pixelText;
    DisplayPixelsAnimatedGIF *pixelGIF;
    DisplayPixelsLive * pixelLive;
    DisplayPixels *curPixel ;
    DisplayClock *pixelClock ;
    WiFiUDP ntpUDP;

#ifdef ESP8266
    const int relayPin = D1;
#endif
    const long interval = 200;  // pause for two seconds

    // By default 'time.nist.gov' is used with 60 seconds update interval and
    // no offset
    NTPClient *timeClient;
    bool sendmorse;
    int morsecount;
#define useNTP 1

  public:
    DisplayHandler()
    {



      timeClient = new NTPClient(ntpUDP, -28800 + (60 * 60/*DST*/));
      pixelText = new DisplayPixelsText();
      pixelGIF = new DisplayPixelsAnimatedGIF();
      pixelLive = new DisplayPixelsLive();
      pixelClock = new DisplayClock(timeClient);
      curPixel = pixelText;
      strip->Begin();
      //  strip->Show();
      #ifdef ESP8266
      sendmorse = false;
      pinMode(relayPin, OUTPUT);
      #endif

    }
    virtual bool isRequestHandlerTrivial() override final {return false;}

    void stop()
    {
      curPixel->stop();
    }

    void ourloop()
    {
      timeClient->update();
      if (curPixel) curPixel->UpdateAnimation();
      strip->Show();

#ifdef ESP8266
      if (sendmorse && morsecount)
      {
        os_printf("inside loop - sendmorse\n");
        digitalWrite(relayPin, HIGH); // turn on relay with voltage HIGH
        os_printf("high\n");
        delay(interval);              // pause
        digitalWrite(relayPin, LOW);  // turn off relay with voltage LOW
        os_printf("low\n");
        delay(interval);              // pause
  
        if (morsecount == 0) sendmorse = false;
        morsecount--;
      }
#endif

    }

    bool canHandle(AsyncWebServerRequest *request) {

      //   os_printf("inside canHandle DisplayHandler %s \n", request->url().c_str());

      if (request->method() == HTTP_GET && request->url() == "/displayclock")
        return true;
      else if (request->method() == HTTP_GET && request->url() == "/clearscreen")
        return true;
      else if (request->method() == HTTP_GET && request->url() == "/scroll")
        return true;
      else if (request->method() == HTTP_GET && request->url() == "/morse")
        return true;
      else if (request->method() == HTTP_GET && request->url() == "/gifs")
        return true;
      else if (request->method() == HTTP_GET && request->url().startsWith("/gif/"))
        return true;
      if (request->method() == HTTP_POST && request->url() == "/setpixels")
        return true;
      return false;
    }

    void displayClock(AsyncWebServerRequest *request)
    {
      curPixel->stop();
      curPixel = pixelClock;
      pixelClock->UpdateAnimation();
      request->send(200, "text/html", "clock started");

    }

    void handleMorse(AsyncWebServerRequest *request)
    {
      os_printf("handleMorse\n");
      sendmorse = true;
      morsecount = 5;
      request->send(200, "text/html", "morse ticked");
    }

    void handleClearScreen(AsyncWebServerRequest *request)
    {
      pixelLive->Clear();
      curPixel->stop();
      curPixel = pixelLive;

      request->send(200, "text/plain", "clear screen");
      return ;
    }
    unsigned char h2int(char c)
    {
      if (c >= '0' && c <= '9') {
        return ((unsigned char)c - '0');
      }
      if (c >= 'a' && c <= 'f') {
        return ((unsigned char)c - 'a' + 10);
      }
      if (c >= 'A' && c <= 'F') {
        return ((unsigned char)c - 'A' + 10);
      }
      return (0);
    }

    unsigned long hex2int(char *a, unsigned int len)
    {
      int i;
      unsigned long val = 0;

      for (i = 0; i < len; i++)
        val += h2int(a[i]) * (1 << (4 * (len - 1 - i)));

      return val;
    }

    void setScrollText(String text)
    {
      pixelText->SetText(text.c_str());
    }
    void setScrollText(AsyncWebServerRequest *request)
    {

      if (request->hasParam("text") ) {
        String text = request->getParam("text")->value();
        if (request->hasParam("color") )
        {
          String colorS = request->getParam("color")->value();

          int pos = 0;
          char hr[4];
          hr[0] = colorS[pos++];
          hr[1] = colorS[pos++];
          hr[2] = 0;
          int r = hex2int(hr, 2);
          char hg[4];
          hg[0] = colorS[pos++];
          hg[1] = colorS[pos++];
          hg[2] = 0;
          int g = hex2int(hg, 2);
          char hb[4];
          hb[0] = colorS[pos++];
          hb[1] = colorS[pos++];
          hb[2] = 0;
          int b = hex2int(hb, 2);
          pixelText->SetColor(RgbColor(r, g, b));
        }

        pixelText->SetText(text.c_str());

        String output = "Setting scroll to: ";
        output += text;

        curPixel->stop();
        curPixel = pixelText;
        curPixel->UpdateAnimation();
        request->send(200, "text/html", output);
        //request->send(200, "text/html", "set text");
      }
      else
        request->send(500, "text/plain", "BAD ARGS");
    }

    void handleGif(AsyncWebServerRequest *request)
    {
      os_printf("inside handleGif \n");

      curPixel->stop();
      pixelGIF->SetGIF(request->url());
      curPixel = pixelGIF;
      request->send(SPIFFS, request->url());

    }

    void handleGifs(AsyncWebServerRequest *request)
    {
      os_printf("inside handleGifs \n");
      if (request->hasParam("dir") ) {
        String path = request->getParam("dir")->value();
        curPixel->stop();
        pixelGIF->SetDIR(path);

        curPixel = pixelGIF;
        request->send(200, "text/html", "started gif loop");

      }
      else
        request->send(500, "text/plain", "BAD ARGS");
    }
    void handleSetPixels(AsyncWebServerRequest * request)
    {

  os_printf("inside handleSetPixels\n");
  os_printf("inside handleSetPixels request=%x\n",request);
int args = request->args();
for(int i=0;i<args;i++){
  os_printf("ARG[%s]: %s\n", request->argName(i).c_str(), request->arg(i).c_str());
}
  os_printf("inside handleSetPixels request=%x\n",request);

            int params = request->params();
  os_printf("params = %d\n",params);
            for (int i = 0; i < params; i++) {
              AsyncWebParameter* p = request->getParam(i);
              if (p->isFile()) {
                os_printf("_FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
              } else if (p->isPost()) {
                os_printf("_POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
              } else {
                os_printf("_GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
              }
            }
      
      AsyncWebParameter* p = NULL;
      if (request->hasParam("pixels", true))
        p = request->getParam("pixels", true);
      if (p)
      {
        String pixels = p->value();

        //pixels.toUpperCase();
        int pos = 0;
        // let's pull the hex values out
        for (int y = 0; y < 8; y++)
        {
          for (int x = 0; x < 8; x++)
          {
            char hr[4];
            hr[0] = pixels[pos++];
            hr[1] = pixels[pos++];
            hr[2] = 0;
            int r = hex2int(hr, 2);
            char hg[4];
            hg[0] = pixels[pos++];
            hg[1] = pixels[pos++];
            hg[2] = 0;
            int g = hex2int(hg, 2);
            char hb[4];
            hb[0] = pixels[pos++];
            hb[1] = pixels[pos++];
            hb[2] = 0;
            int b = hex2int(hb, 2);

            os_printf("%d %x %x%x%x\n",x,y,r,g,b);
            pixelLive->SetPixel(x, y, r, g, b);
          }
        }
        curPixel->stop();
        curPixel = pixelLive;

        request->send(200, "text/html", "setpixels");

      }
    }

    void handleRequest(AsyncWebServerRequest * request) {

      if (request->method() == HTTP_GET && request->url() == "/displayclock")
        displayClock(request);
      else if (request->method() == HTTP_GET && request->url() == "/clearscreen")
        handleClearScreen(request);
      else if (request->method() == HTTP_GET && request->url() == "/scroll")
        setScrollText(request);
      else if (request->method() == HTTP_GET && request->url() == "/gifs")
        handleGifs(request);
      else if (request->method() == HTTP_GET && request->url().startsWith("/gif/"))
        handleGif(request);
      else if (request->method() == HTTP_GET && request->url() == "/morse")
        handleMorse(request);
      else if (request->method() == HTTP_POST && request->url() == "/setpixels")
        handleSetPixels(request);
    }



};




/*
    The SPIFFS system has a limit to the number of characters in a file name

    We rewrite the files with a shell script to be a number, or number.gz and
    then use an index file to map them.
*/
class POHandler: public AsyncWebHandler {
    String  _setContentType(String path) {
      String _contentType;
      if (path.endsWith(".html")) _contentType = "text/html";
      else if (path.endsWith(".htm")) _contentType = "text/html";
      else if (path.endsWith(".css")) _contentType = "text/css";
      else if (path.endsWith(".js")) _contentType = "application/javascript";
      else if (path.endsWith(".png")) _contentType = "image/png";
      else if (path.endsWith(".gif")) _contentType = "image/gif";
      else if (path.endsWith(".jpg")) _contentType = "image/jpeg";
      else if (path.endsWith(".ico")) _contentType = "image/x-icon";
      else if (path.endsWith(".svg")) _contentType = "image/svg+xml";
      else if (path.endsWith(".xml")) _contentType = "text/xml";
      else if (path.endsWith(".pdf")) _contentType = "application/pdf";
      else if (path.endsWith(".zip")) _contentType = "application/zip";
      else if (path.endsWith(".gz")) _contentType = "application/x-gzip";
      else _contentType = "text/plain";
      return _contentType;
    }
    bool canHandle(AsyncWebServerRequest *request) {
      if (request->method() == HTTP_GET && request->url().startsWith("/p/"))
        return true;
      return false;
    }
    void handleRequest(AsyncWebServerRequest *request) {
      String path = request->url();
      handleRequest(request, path);
    }
  public:
    void handleRequest(AsyncWebServerRequest *request, String path)
    {
      if (path.endsWith("/"))
        path += "index.html";
      os_printf("handleFileReadPO: path  = %s\n", path.c_str());
      String short_name = "";
      int found = 0;

      // read the file from disk that is our index:
      File file = SPIFFS.open("/pindex.txt", "r");
      String l_line = "";
      //open the file here
      while (file.available() != 0) {
        //A inconsistent line length may lead to heap memory fragmentation
        l_line = file.readStringUntil('\n');
        if (l_line == "") //no blank lines are anticipated
          break;
        //
        int pipePos = l_line.indexOf("|");
        String long_name = l_line.substring(pipePos + 1);
        long_name = "/" + long_name;
        short_name = l_line.substring(0, pipePos);

        //parse l_line here
        //DBG_OUTPUT_PORT.println("got line: "+l_line);
        //DBG_OUTPUT_PORT.println("["+long_name+"]["+short_name+"]");

        if (path == long_name)
        {
          os_printf("***** FOUND IT ***\n");
          os_printf("[%s][%s]\n", long_name.c_str(), short_name.c_str());

          found = 1;
          #ifdef ESP32
          //short_name = "/po/" + short_name+".gz";
          short_name = "/po/" + short_name;
          #else
          short_name = "/po/" + short_name;
          #endif
          break;
        }
      }
      file.close();

      if (found == 0)
      {
        request->send(404);
        return;
      }

      os_printf("send: SPIFFS [%s] [%s] [blank] [download?] [%s]\n", short_name.c_str(), path.c_str(),_setContentType(path).c_str());


       //AsyncWebServerResponse *response = request->beginResponse(SPIFFS, short_name, _setContentType(path));
      //request->send(response);
      //request->_tempFile = SPIFFS.open(short_name, "r");
      //os_printf("FILEINFO:%d %s\n",request->_tempFile.size(),request->_tempFile.name());
      //request->send(request->_tempFile, request->_tempFile.name(), String(), request->hasParam("download"));
      //request->send(theFile, short_name, _setContentType(path), 0);

      //File theFile = SPIFFS.open(short_name,"r");
      //if (theFile
      //request->send(theFile, short_name, _setContentType(path), 0);

      // original:
       request->send(SPIFFS, short_name, _setContentType(path),false);
      //request->send(SPIFFS, short_name, String(), request->hasParam("download"));


      return ;




    }
};

/*
     The PiskelHandler handles all the loading and saving of data for the piskel
     javascript pixel editor.

     It looks like:

     /piskel/  -- this loads gallery.html, and some javascript to display the gallery
     /piskel/json   -- gallery.html requests this json which gives us a list of all the piskel files stored in /piskeldata/
     /piskel/1/ -- this would load the piskel javascript, for 1.json
     /piskel/l/load
     /piskel/1/save
     /piskel/1/upload
     /piskel/1/delete

*/
class PiskelHandler: public AsyncWebHandler {

    POHandler *_thePOHandler;
    DisplayHandler *_theDisplayHandler;

  public:
    PiskelHandler(POHandler *thePOHandler, DisplayHandler *theDisplay)
    {
      _thePOHandler = thePOHandler;
      _theDisplayHandler = theDisplay;
    }

    virtual bool isRequestHandlerTrivial() override final {return false;}



    bool canHandle(AsyncWebServerRequest *request) {
      // os_printf("inside canHandle piskel  %s \n", request->url().c_str());
      if (request->method() == HTTP_GET && request->url().startsWith("/piskel/"))
        return true;
      else   if (request->method() == HTTP_POST && request->url().startsWith("/piskel/"))
        return true;
      return false;
    }


    /*
       If we are autosaving, then we want to increment the next javascript number

    */

    String numberFromPath(String path)
    {
      int pos = path.lastIndexOf(".json");
      int slashpos = path.lastIndexOf("/");
      String filenumberS = path.substring(slashpos + 1, pos);

      return filenumberS;
    }

    int findNextFileNumber()
    {
      int largestnumber = 0;

      #ifdef ESP8266
      Dir dir = SPIFFS.openDir("/piskeldata/");
      while (dir.next()) {
        String filenumberS = numberFromPath(dir.fileName());

        //DBG_OUTPUT_PORT.println(filenumberS);
        int filenumber = filenumberS.toInt();
        filenumber++;
        if (filenumber > largestnumber) largestnumber = filenumber;
      }
      DBG_OUTPUT_PORT.print("largestnumber: ");
      DBG_OUTPUT_PORT.println(largestnumber);
      return largestnumber;
      #else
 if (!SPIFFS.exists("/piskeldata/"))
 {
  return 0;
 }
      
        File dir = SPIFFS.open("/piskeldata/");


    if(!dir){
        Serial.println("- failed to open directory");
        return 0;
    }
    if(!dir.isDirectory()){
        Serial.println(" - not a directory");
        return 0;
    }

        
        dir.rewindDirectory();
        while (File entry = dir.openNextFile())
        {
             String filenumberS = numberFromPath(entry.name());
              int filenumber = filenumberS.toInt();
              filenumber++;
              if (filenumber > largestnumber) largestnumber = filenumber;
        }
          return largestnumber;

      #endif
    }

    void handlePiskelUpload(AsyncWebServerRequest *request, String id) {
      os_printf("handlePiskelUpload\n");
      
      int params = request->params();
      os_printf("handlePiskelUpload: params:%d\n",params);
      for (int i = 0; i < params; i++) {
        AsyncWebParameter* p = request->getParam(i);
        if (p->isFile()) {
          os_printf("_FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
        } else if (p->isPost()) {
          os_printf("hrspiff _POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
        } else {
          os_printf("_GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
        }
      }


      // not sure the name - let's just use default.gif for right now
      if (request->hasParam("data" , true)) {
        String data = request->getParam("data", true)->value();
        os_printf("data: %s\n", data.c_str());
        int len = data.length();
        int newbufsize = base64_decode_expected_len(data.length());
        uint8_t *newbuf = (uint8_t *)malloc(newbufsize);
        if (newbuf)
        {
          int finallen = base64_decode_chars((char *)data.c_str() + 22, len - 22, (char *)newbuf);
          //
          // write the gif
          //
          String filename = "/gif/" + id + ".gif";
          File file = SPIFFS.open(filename, "w");
          if (file)
            file.write((const uint8_t *)newbuf, finallen);
          if (file)
            file.close();
          free(newbuf);
        }
        request->send(200, "text/html", "/gif/default.gif");

      }
    }

    void handlePiskelDelete(AsyncWebServerRequest * request, String id) {
      String  filename = "/piskeldata/" + id + ".json";
      SPIFFS.remove(filename);
      request->send(200, "text/html", "file removed");

    }

    void handlePiskelSave(AsyncWebServerRequest * request, String id) {
      DBG_OUTPUT_PORT.println("handlePiskelSave");

      
          int params = request->params();
          for (int i = 0; i < params; i++) {
            AsyncWebParameter* p = request->getParam(i);
            if (p->isFile()) {
              os_printf("_FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
            } else if (p->isPost()) {
              os_printf("_POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
            } else {
              os_printf("_GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
            }
          }

           //String result;
      String filename = "";
      if (id)
      {
        filename = "/piskeldata/" + id + ".json";
      }
      else
      {
        int filenumber = findNextFileNumber();
        //DBG_OUTPUT_PORT.println("filenumber:" + filenumber);
        filename = "/piskeldata/" + String(filenumber) + ".json";
      }
      //DBG_OUTPUT_PORT.println("filename [" + filename + "]");
      File file = SPIFFS.open(filename, "w");

      //String result = String("window.pskl.appEnginePiskelData_ = {\r\n \"piskel\" :");
      String framesheet = request->getParam("framesheet", true)->value();
      //DBG_OUTPUT_PORT.println("framesheet:" + framesheet);
      String result = String("{\r\n \"piskel\" :");
      //DBG_OUTPUT_PORT.println("result1:" + result);
      result +=  framesheet;
      //DBG_OUTPUT_PORT.println("result2:" + result);
      result += ",\r\n";
      result += "\"isLoggedIn\": \"true\",";
      //DBG_OUTPUT_PORT.println("result3:" + result);
      result += "\"fps\":" +  request->getParam("fps", true)->value() + ",\r\n";
      result += "\"descriptor\" : {";
      result += "\"name\": \"" + request->getParam("name", true)->value() + "\",";
      result += "\"description\": \"" + request->getParam("description", true)->value() + "\",";

      result += "\"first_frame_as_png\": \"" + request->getParam("first_frame_as_png", true)->value() + "\",";
      result += "\"framesheet_as_png\": \"" + request->getParam("framesheet_as_png", true)->value() + "\",";


      result += "\"isPublic\": \"false\"";
      result += "}\r\n}";
      //DBG_OUTPUT_PORT.println("result:");
      //DBG_OUTPUT_PORT.println(result);
      //DBG_OUTPUT_PORT.println(result.length());
      
      int r=file.write((const uint8_t *)result.c_str(), result.length());
      //DBG_OUTPUT_PORT.println(r);

      if (file)
        file.close();

      request->send(200, "text/html", "");

    }


    void handlePiskelLoad(AsyncWebServerRequest * request, String id)
    {
       DBG_OUTPUT_PORT.println("handlePiskelLoad");

      String filename = "/piskeldata/" + id + ".json";
       DBG_OUTPUT_PORT.println(filename);

      File  file = SPIFFS.open(filename, "r");




      //open the file here

      AsyncResponseStream *response = request->beginResponseStream("application/javascript", 2048);
      response->print(  "window.pskl = window.pskl || {};"
                        "window.pskl.appEngineToken_ = false;"
                        "window.pskl.appEnginePiskelData_ =");

      if (!file || file.size()==0)
      {
        // write an empty file

        response->print(
          "{"
          "piskel : null,"
          "isLoggedIn : true,"
          "fps : 4,"
          "descriptor : {"
          "name : \"New piskel\","
          "description : \"\","
          "isPublic : true"
          "}"
          "}"
        );

      }
      else
      {

        char *buf = (char*)malloc(file.size() + 1);
        if (buf) {
          file.read((uint8_t *)buf, file.size());
          buf[file.size()] = '\0';
        }
        os_printf("file size: %d\n", file.size());
        file.close();
        response->print(buf);
        free(buf);



      }

      response->print(
        ";"
      );

      request->send(response);


    }

    void handleFilePiskelJSONIndex(AsyncWebServerRequest * request)
    {      
      _theDisplayHandler->stop();

      int len = 0;
      File tempFile = SPIFFS.open("/piskelindex.json", "w");

      #ifdef ESP8266
      Dir dir = SPIFFS.openDir("/piskeldata/");


      tempFile.print("[");
      int first = 1;
      while (dir.next()) {
        if (first)
          first = 0;
        else
          tempFile.print( ",");


        String number = numberFromPath(dir.fileName());
         os_printf("file name: %s \n", dir.fileName().c_str());

        File file = SPIFFS.open(dir.fileName(), "r");
        if (!file) os_printf("file open failed\n");
 
        tempFile.print( "{\"" + number + "\":");
        //  response->print(dir.fileName());
 os_printf("file size: %d \n", file.size());

        char *buf = (char*)malloc(file.size() + 1);
        if (buf) {
      os_printf("file position: %d \n", file.position());
     
        buf[0]=0;
        Serial.println("allocated");
          int r=file.read((uint8_t *)buf, file.size());
          os_printf("file read: %d \n", r);
         len += file.size();
          buf[file.size()] = '\0';
        }
        tempFile.print(buf);
        file.close();
        free(buf);
        tempFile.print("}");

      }
      tempFile.print("]");
      os_printf("larger than: %d (4096)\n", len);





      #else
   {     

      File dir = SPIFFS.open("/piskeldata");
       os_printf("inside loop dir: %x\n",dir);

    if(!dir.isDirectory()){
        Serial.println(" - not a directory");
    }

      tempFile.print("[");
      int first = 1;
      os_printf("before openNextFile\n");
      if ( dir && dir.isDirectory()) 
      {
        File entry = dir.openNextFile();
        os_printf("before while loop entry: %x %d\n",entry,entry.size() );

        while (entry) {
        os_printf("inside while loop entry: %x %d\n",entry,entry.size());
        if (first)
          first = 0;
        else
          tempFile.print( ",");


        String number = numberFromPath(entry.name());
        

        tempFile.print( "{\"" + number + "\":");
        //  response->print(dir.fileName());
        char *buf = (char*)malloc(entry.size() + 1);
        if (buf) {
          entry.read((uint8_t *)buf, entry.size());
          len += entry.size();
          buf[entry.size()] = '\0';
        }
                os_printf("printing buf: %s\n",buf);

        tempFile.print(buf);
        entry.close();
        free(buf);
        tempFile.print("}");
        entry = dir.openNextFile();
      }
      }
      tempFile.print("]");
      os_printf("larger than: %d (4096)\n", len);
      
      #endif
      
      tempFile.close();

      request->send(SPIFFS, "/piskelindex.json", "text/json");


    }

    /*
        The piskel code expects url's to look like:

        /piskel/<ID>/index.html

        we are going to pull out the ID and then call the PO routine to handle the actual files

    */


    void handleFileReadPiskel(AsyncWebServerRequest * request, String path)
    {

      if (path == "/piskel/")
      {
        // we should print out our gallery here
        request->send(SPIFFS, "/gallery.html");

        return ;
      }
      else if (path == "/piskel/json")
      {
        handleFilePiskelJSONIndex(request);
        return ;
      }


      /* move forward /piskel/ characters, and find the next / */
      int piskelLen = 8;
      int pos = path.indexOf("/", piskelLen);
      String filenumberS = path.substring(piskelLen, pos);
      //DBG_OUTPUT_PORT.println(filenumberS);

      String newpath = path.substring(pos + 1);
      newpath = String("/p/") + newpath;
      //DBG_OUTPUT_PORT.println(newpath);

      // if we have the data js file, then we return that, otherwise we return the static files
      if (newpath == "/p/load")
      {
        handlePiskelLoad(request, filenumberS);
        return ;
      }
      else if (newpath == "/p/save")
      {
        handlePiskelSave(request, filenumberS);
        return ;
      }
      else if (newpath == "/p/upload")
      {
        handlePiskelUpload(request, filenumberS);
        return ;

      }
      else if (newpath == "/p/delete")
      {
        handlePiskelDelete(request, filenumberS);
        return ;
      }
      else
        _thePOHandler->handleRequest(request, newpath);


      return ;
    }


    void handleRequest(AsyncWebServerRequest * request) {

      if (request->url().startsWith("/piskel/"))
        handleFileReadPiskel(request, request->url());
    }


};



#if 1
// WEB HANDLER IMPLEMENTATION
class OurSPIFFSEditor: public AsyncWebHandler {
  private:
      fs::FS _fs;

    String _username;
    String _password;
    bool _uploadAuthenticated;
  public:
      virtual bool isRequestHandlerTrivial() override final {return false;}

    OurSPIFFSEditor(String username = String(), String password = String(),const fs::FS& fs=SPIFFS): _username(username), _password(password), _uploadAuthenticated(false),_fs(fs) {}
    bool canHandle(AsyncWebServerRequest *request) {
      if (request->method() == HTTP_GET && request->url() == "/edit" && (SPIFFS.exists("/edit.htm") || SPIFFS.exists("/edit.htm.gz")))
        return true;
      else if (request->method() == HTTP_GET && request->url() == "/list")
        return true;
      else if (request->method() == HTTP_GET && (request->url().endsWith("/") || SPIFFS.exists(request->url()) || (!request->hasParam("download") && SPIFFS.exists(request->url() + ".gz"))))
        return true;
      else if (request->method() == HTTP_POST && request->url() == "/edit")
        return true;
      else if (request->method() == HTTP_DELETE && request->url() == "/edit")
        return true;
      else if (request->method() == HTTP_PUT && request->url() == "/edit")
        return true;
      return false;
    }

void handleRequest(AsyncWebServerRequest *request){
  if(_username.length() && _password.length() && !request->authenticate(_username.c_str(), _password.c_str()))
    return request->requestAuthentication();
    
        int params = request->params();
        for (int i = 0; i < params; i++) {
          AsyncWebParameter* p = request->getParam(i);
          if (p->isFile()) {
            os_printf("_FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
          } else if (p->isPost()) {
            os_printf("_POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
          } else {
            os_printf("_GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
          }
        }


  if(request->method() == HTTP_GET){
    if(request->hasParam("list")){
      String path = request->getParam("list")->value();
#ifdef ESP32
      File dir = _fs.open(path);
#else
      Dir dir = _fs.openDir(path);
#endif
      path = String();
      String output = "[";
#ifdef ESP32
      File entry = dir.openNextFile();
      while(entry){
#else
      while(dir.next()){
        fs::File entry = dir.openFile("r");
#endif
        if (false /*isExcluded(_fs, entry.name())*/) {
#ifdef ESP32
            entry = dir.openNextFile();
#endif
            continue;
        }
        if (output != "[") output += ',';
        output += "{\"type\":\"";
        output += "file";
        output += "\",\"name\":\"";
        output += String(entry.name());
        output += "\",\"size\":";
        output += String(entry.size());
        output += "}";
#ifdef ESP32
        entry = dir.openNextFile();
#else
        entry.close();
#endif
      }
#ifdef ESP32
      dir.close();
#endif
      output += "]";
      request->send(200, "application/json", output);
      output = String();
    }
    else if(request->hasParam("edit") || request->hasParam("download")){
      request->send(request->_tempFile, request->_tempFile.name(), String(), request->hasParam("download"));
    }
    else {
       request->send(SPIFFS, "/edit.htm");

      /*
      const char * buildTime = __DATE__ " " __TIME__ " GMT";
      if (request->header("If-Modified-Since").equals(buildTime)) {
        request->send(304);
      } else {
        AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", edit_htm_gz, edit_htm_gz_len);
        response->addHeader("Content-Encoding", "gzip");
        response->addHeader("Last-Modified", buildTime);
        request->send(response);
      }
      */
    }
  } else if(request->method() == HTTP_DELETE){
    if(request->hasParam("path", true)){
        _fs.remove(request->getParam("path", true)->value());
      request->send(200, "", "DELETE: "+request->getParam("path", true)->value());
    } else
      request->send(404);
  } else if(request->method() == HTTP_POST){
    if(request->hasParam("data", true, true) && _fs.exists(request->getParam("data", true, true)->value()))
      request->send(200, "", "UPLOADED: "+request->getParam("data", true, true)->value());
    else
      request->send(500);
  } else if(request->method() == HTTP_PUT){
    if(request->hasParam("path", true)){
      String filename = request->getParam("path", true)->value();
      if(_fs.exists(filename)){
        request->send(200);
      } else {
        fs::File f = _fs.open(filename, "w");
        if(f){
          f.write((uint8_t)0x00);
          f.close();
          request->send(200, "", "CREATE: "+filename);
        } else {
          request->send(500);
        }
      }
    } else
      request->send(400);
  }
}
    void handleRequest2(AsyncWebServerRequest *request) {
      if (_username.length() && (request->method() != HTTP_GET || request->url() == "/edit" || request->url() == "/list") && !request->authenticate(_username.c_str(), _password.c_str()))
        return request->requestAuthentication();

      if (request->method() == HTTP_GET && request->url() == "/edit") {
        request->send(SPIFFS, "/edit.htm");
      } else if (request->method() == HTTP_GET && request->url() == "/list") {
        os_printf("LIST\n");
        int params = request->params();
        for (int i = 0; i < params; i++) {
          AsyncWebParameter* p = request->getParam(i);
          if (p->isFile()) {
            os_printf("_FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
          } else if (p->isPost()) {
            os_printf("_POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
          } else {
            os_printf("_GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
          }
        }

        if (request->hasParam("dir")) {
          String path = request->getParam("dir")->value();
          os_printf("LIST path:[%s]\n", path.c_str());


          #ifdef ESP8266
          
          Dir dir = SPIFFS.openDir(path);
          path = String();


          AsyncResponseStream *response = request->beginResponseStream("application/javascript", 4096);
          int start = 1;
          response->print( "[");
          while (dir.next()) {

            File entry = dir.openFile("r");
            if (!start) response->print( ","); else start = 0;

            bool isDir = false;
            response->print("{\"type\":\"");
            response->print((isDir) ? "dir" : "file");
            response->print( "\",\"name\":\"");
            response->print( String(entry.name()).substring(1));
            response->print( "\"}");
            entry.close();
          }
          
          #else
          File dir = SPIFFS.open(path);
          path = String();


          AsyncResponseStream *response = request->beginResponseStream("application/javascript", 4096);
          int start = 1;
          response->print( "[");
          while (File entry = dir.openNextFile()) {

            //File entry = dir.openFile("r");
            if (!start) response->print( ","); else start = 0;

            bool isDir = false;
            response->print("{\"type\":\"");
            response->print((isDir) ? "dir" : "file");
            response->print( "\",\"name\":\"");
            response->print( String(entry.name()).substring(1));
            response->print( "\"}");
            entry.close();
          }
          dir.close();
          #endif
          
          response->print("]");
          request->send(response);


        }
        else
          request->send(400);
      } else if (request->method() == HTTP_GET) {
        String path = request->url();
        if (path.endsWith("/"))
          path += "index.htm";

        request->send(SPIFFS, path, String(), request->hasParam("download"));
      } else if (request->method() == HTTP_DELETE) {
        if (request->hasParam("path", true)) {
          #if defined(ESP8266)
          ESP.wdtDisable(); 
          #endif
          SPIFFS.remove(request->getParam("path", true)->value()); 
          #if defined(ESP8266)
          ESP.wdtEnable(10);
          #endif
          request->send(200, "", "DELETE: " + request->getParam("path", true)->value());
        } else
          request->send(404);
      } else if (request->method() == HTTP_POST) {
     Serial.println("POST ");
            int params = request->params();
        for (int i = 0; i < params; i++) {
          AsyncWebParameter* p = request->getParam(i);
          if (p->isFile()) {
            os_printf("_FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
          } else if (p->isPost()) {
            os_printf("_POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
          } else {
            os_printf("_GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
          }
        }
        Serial.println(request->getParam("data", true, true)->value());
        if (request->hasParam("data", true, true) && SPIFFS.exists(request->getParam("data", true, true)->value()))
        {
          request->send(200, "", "UPLOADED: " + request->getParam("data", true, true)->value());
        }
        else
        {
     Serial.println("500 error here ");
 request->send(500);
        }
         
        
      } else if (request->method() == HTTP_PUT) {
        if (request->hasParam("path", true)) {
          String filename = request->getParam("path", true)->value();
          if (SPIFFS.exists(filename)) {
            request->send(200);
          } else {
            File f = SPIFFS.open(filename, "w");
            if (f) {
              f.write(0x00);
              f.close();
              request->send(200, "", "CREATE: " + filename);
            } else {
              request->send(500);
            }
          }
        } else
          request->send(400);
      }
    }

    void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
      
      Serial.println("handleUpload");
      if (!index) {
        if (!_username.length() || request->authenticate(_username.c_str(), _password.c_str()))
          _uploadAuthenticated = true;
        request->_tempFile = SPIFFS.open(filename, "w");
      }
      if (_uploadAuthenticated && request->_tempFile && len) {
        #if defined(ESP8266)
        ESP.wdtDisable(); 
        #endif
        request->_tempFile.write(data, len); 
        #if defined(ESP8266)
        ESP.wdtEnable(10);
        #endif
}
      if (_uploadAuthenticated && final)
        if (request->_tempFile) request->_tempFile.close();
    }
};
#endif


void initSerial(void);


// SKETCH BEGIN

AsyncWebServer server(80);
DNSServer dns;
AsyncWiFiManager wifiManager(&server, &dns);

const char* http_username = "admin";
const char* http_password = "admin";


#ifdef ESP8266
 String softap_new_ssid = "WebPixelFrame" + String("-") + String(ESP.getChipId());
#else
 String softap_new_ssid = "WebPixelFrame" + String("-") + String((uint16_t)(ESP.getEfuseMac()>>32));
#endif

DisplayHandler *theDisplay = NULL;
//callback notifying us of the need to save config




void saveConfigCallback () {

  String ip = WiFi.localIP().toString();
  theDisplay->setScrollText(ip.c_str());


  Serial.println("save config");
  // Setup MDNS responder


  if (!MDNS.begin(softap_new_ssid.c_str()/*myHostname*/)) {
    Serial.println("Error setting up MDNS responder!");
  } else {
    Serial.println("mDNS responder started");
    // Add service to MDNS-SD
    MDNS.addService("http", "tcp", 80);
  }
}


void SPIFFSInfo(){
  FSInfo fs_info;
SPIFFS.info(fs_info);

float fileTotalKB = (float)fs_info.totalBytes / 1024.0;
float fileUsedKB = (float)fs_info.usedBytes / 1024.0;

float flashChipSize = (float)ESP.getFlashChipSize() / 1024.0 / 1024.0;
float realFlashChipSize = (float)ESP.getFlashChipRealSize() / 1024.0 / 1024.0;
float flashFreq = (float)ESP.getFlashChipSpeed() / 1000.0 / 1000.0;
FlashMode_t ideMode = ESP.getFlashChipMode();

Serial.println("==========================================================");
Serial.println("Firmware: ");
Serial.printf(" Chip Id: %08X\n", ESP.getChipId());
Serial.print(" Core version: "); Serial.println(ESP.getCoreVersion());
Serial.print(" SDK version: "); Serial.println(ESP.getSdkVersion());
Serial.print(" Boot version: "); Serial.println(ESP.getBootVersion());
Serial.print(" Boot mode: "); Serial.println(ESP.getBootMode());
Serial.printf("__________________________\n\n");

Serial.println("Flash chip information: ");
Serial.printf(" Flash chip Id: %08X (for example: Id=001640E0 Manuf=E0, Device=4016 (swap bytes))\n", ESP.getFlashChipId());
Serial.printf(" Sketch thinks Flash RAM is size: "); Serial.print(flashChipSize); Serial.println(" MB");
Serial.print(" Actual size based on chip Id: "); 
Serial.print(realFlashChipSize); 
Serial.println(" MB ... given by (2^( \"Device\" - 1) / 8 / 1024");
Serial.print(" Flash frequency: "); Serial.print(flashFreq); Serial.println(" MHz");
Serial.printf(" Flash write mode: %s\n", (ideMode == FM_QIO ? "QIO" : ideMode == FM_QOUT ? "QOUT" : ideMode == FM_DIO ? "DIO" : ideMode == FM_DOUT ? "DOUT" : "UNKNOWN"));
Serial.printf(" CPU frequency: %u MHz\n\n", ESP.getCpuFreqMHz());
Serial.printf("__________________________\n\n");

Serial.println("File system (SPIFFS): ");
Serial.print(" Total KB: "); Serial.print(fileTotalKB); Serial.println(" KB");
Serial.print(" Used KB: "); Serial.print(fileUsedKB); Serial.println(" KB");
Serial.printf(" Block size: %lu\n", fs_info.blockSize);
Serial.printf(" Page size: %lu\n", fs_info.pageSize);
Serial.printf(" Maximum open files: %lu\n", fs_info.maxOpenFiles);
Serial.printf(" Maximum path length: %lu\n\n", fs_info.maxPathLength);
Serial.printf("__________________________\n\n");


}



void setup() {
  
  initSerial();

#if 1
  SPIFFS.begin();
  SPIFFSInfo();
  theDisplay = new DisplayHandler();

  //WiFi.disconnect(true);
  //MDNS?




  // modal:
  //wifiManager.autoConnect(softap_new_ssid.c_str());
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  // modeless:
  wifiManager.startConfigPortalModeless(softap_new_ssid.c_str(), "");
  saveConfigCallback(); // this seems broken..
  // MDNS will only work if we have the wifi in STA mode.. should we do that?

  ArduinoOTA.setPassword((const char *)"");
  ArduinoOTA.begin();
  //Serial.printf("format start\n"); SPIFFS.format();  Serial.printf("format end\n");
  os_printf("setup called - 3\n");

  POHandler *thePOHandler = new POHandler();
  PiskelHandler *thePiskelHandler = new PiskelHandler(thePOHandler, theDisplay);
  server.on("/heap", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });


  server.addHandler(thePOHandler).setFilter(ON_STA_FILTER);
  server.addHandler(thePiskelHandler).setFilter(ON_STA_FILTER);
  server.addHandler(theDisplay).setFilter(ON_STA_FILTER);


  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.htm").setFilter(ON_STA_FILTER);
  #ifdef ESP32
    server.addHandler(new SPIFFSEditor(SPIFFS,http_username,http_password)).setFilter(ON_STA_FILTER);
  #else
   //server.addHandler(new OurSPIFFSEditor(http_username,http_password)).setFilter(ON_STA_FILTER);
   server.addHandler(new SPIFFSEditor(http_username,http_password)).setFilter(ON_STA_FILTER);
   // server.addHandler(new SPIFFSEditor(http_username,http_password));
  #endif
    //SPIFFSEditor(const fs::FS& fs, const String& username=String(), const String& password=String());
//  server.addHandler(new SPIFFSEditor(http_username, http_password)).setFilter(ON_STA_FILTER);

  // how do we filter the notfound correctly?
  // this is picked up by the AsyncWiFiManager - if we set it here, we will lose the captive portal for all the 404 urls
#if 0
  server.onNotFound([](AsyncWebServerRequest * request) {
    os_printf("NOT_FOUND: ");
    if (request->method() == HTTP_GET)
      os_printf("GET");
    else if (request->method() == HTTP_POST)
      os_printf("POST");
    else if (request->method() == HTTP_DELETE)
      os_printf("DELETE");
    else if (request->method() == HTTP_PUT)
      os_printf("PUT");
    else if (request->method() == HTTP_PATCH)
      os_printf("PATCH");
    else if (request->method() == HTTP_HEAD)
      os_printf("HEAD");
    else if (request->method() == HTTP_OPTIONS)
      os_printf("OPTIONS");
    else
      os_printf("UNKNOWN");
    os_printf(" http://%s%s\n", request->host().c_str(), request->url().c_str());

    if (request->contentLength()) {
      os_printf("_CONTENT_TYPE: %s\n", request->contentType().c_str());
      os_printf("_CONTENT_LENGTH: %u\n", request->contentLength());
    }

    int headers = request->headers();
    int i;
    for (i = 0; i < headers; i++) {
      AsyncWebHeader* h = request->getHeader(i);
      os_printf("_HEADER[%s]: %s\n", h->name().c_str(), h->value().c_str());
    }

    int params = request->params();
    for (i = 0; i < params; i++) {
      AsyncWebParameter* p = request->getParam(i);
      if (p->isFile()) {
        os_printf("_FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
      } else if (p->isPost()) {
        os_printf("_POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
      } else {
        os_printf("_GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
      }
    }

    request->send(404);
  });
#endif
  server.onFileUpload([](AsyncWebServerRequest * request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    os_printf("onFileUpload\n");
    if (!index)
      os_printf("UploadStart: %s\n", filename.c_str());
    os_printf("%s", (const char*)data);
    if (final)
      os_printf("UploadEnd: %s (%u)\n", filename.c_str(), index + len);
  });
  server.onRequestBody([](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total) {
    if (!index)
      os_printf("BodyStart: %u\n", total);
    os_printf("%s", (const char*)data);
    if (index + len == total)
      os_printf("BodyEnd: %u\n", total);
  });



  //get heap status, analog input value and all GPIO statuses in one json call
  server.on("/all", HTTP_GET, [](AsyncWebServerRequest * request) {


    String json = "{";
    json += "\"heap\":" + String(ESP.getFreeHeap());
    #ifdef ESP8266
    json += ", \"analog\":" + String(analogRead(A0));
    json += ", \"gpio\":" + String((uint32_t)(((GPI | GPO) & 0xFFFF) | ((GP16I & 0x01) << 16)));
    #endif
    json += "}";
    request->send(200, "text/json", json);
    json = String();
  }).setFilter(ON_STA_FILTER);


  // this is being done in the AsyncWiFiManager
  //server.begin();


#endif

}

void loop() {
  wifiManager.loop();
  dns.processNextRequest();
   
  #ifdef ESP8266
  MDNS.update();
  #endif
//  ArduinoOTA.handle();
  if (theDisplay) theDisplay->ourloop();
}

#ifdef ESP32
void initSerial() {
  Serial.begin(115200);
}
#endif


#ifdef ESP8266
#ifdef __cplusplus
extern "C" {
#endif
void system_set_os_print(uint8 onoff);
void ets_install_putc1(void* routine);
#ifdef __cplusplus
}
#endif

void initSerial() {
  Serial.begin(115200);
  #ifndef ESP32

  ets_install_putc1((void *) &_u0_putc);
    system_set_os_print(1);

 #endif
}

#endif
