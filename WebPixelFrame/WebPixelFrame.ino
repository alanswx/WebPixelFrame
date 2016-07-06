#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <FS.h>

#include <ESPAsyncTCP.h>   // https://github.com/me-no-dev/ESPAsyncTCP
#include <ESPAsyncWebServer.h>  // https://github.com/me-no-dev/ESPAsyncWebServer/
#include <ESPAsyncWiFiManager.h>  // https://github.com/alanswx/ESPAsyncWiFiManager

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

class DisplayHandler: public AsyncWebHandler {
    DisplayPixelsText *pixelText;
    DisplayPixelsAnimatedGIF *pixelGIF;
    DisplayPixelsLive * pixelLive;
    DisplayPixels *curPixel ;
    DisplayClock *pixelClock ;
    WiFiUDP ntpUDP;

    const int relayPin = D1;
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
      curPixel = pixelClock;
      strip->Begin();
      //  strip->Show();
      sendmorse = false;
      pinMode(relayPin, OUTPUT);

    }

    void stop()
    {
      curPixel->stop();
    }

    void loop()
    {
      //  os_printf("loop: \n");
      timeClient->update();
      if (curPixel) curPixel->UpdateAnimation();
      strip->Show();

      if (sendmorse && morsecount)
      {
        os_printf("inside loop - sendmorse\n");
        digitalWrite(relayPin, HIGH); // turn on relay with voltage HIGH
        os_printf("high\n");
        delay(interval);              // pause
        digitalWrite(relayPin, LOW);  // turn off relay with voltage LOW
        os_printf("low\n");
        delay(interval);              // pause
        
        if (morsecount==0) sendmorse=false;
        morsecount--;
      }

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
      morsecount=5;
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

    void handleSetPixels(AsyncWebServerRequest *request)
    {


      /*
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
      */
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

            //os_printf("%d %x %x%x%x\n",x,y,r,g,b);
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
          short_name = "/po/" + short_name;
          break;
        }
      }
      file.close();

      if (found == 0)
      {
        request->send(404);
        return;
      }

      os_printf("send: SPIFFS [%s] [%s] [blank] [download?]", short_name.c_str(), path.c_str());

      request->send(SPIFFS, short_name, _setContentType(path));
      //request->send(SPIFFS, short_name, String(), request->hasParam("download"));


      return ;




    }
};


class PiskelHandler: public AsyncWebHandler {

    POHandler *_thePOHandler;
    DisplayHandler *_theDisplayHandler;

  public:
    PiskelHandler(POHandler *thePOHandler, DisplayHandler *theDisplay)
    {
      _thePOHandler = thePOHandler;
      _theDisplayHandler = theDisplay;
    }



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
      Dir dir = SPIFFS.openDir("/piskeldata/");
      while (dir.next()) {
        String filenumberS = numberFromPath(dir.fileName());

        //DBG_OUTPUT_PORT.println(filenumberS);
        int filenumber = filenumberS.toInt();
        filenumber++;
        if (filenumber > largestnumber) largestnumber = filenumber;
      }
      //DBG_OUTPUT_PORT.print("largestnumber: ");
      //DBG_OUTPUT_PORT.println(largestnumber);
      return largestnumber;
    }

    void handlePiskelUpload(AsyncWebServerRequest *request, String id) {

      int params = request->params();
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
    void handlePiskelSave(AsyncWebServerRequest * request, String id) {
      //DBG_OUTPUT_PORT.println("handlePiskelSave");

      /*
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

      */
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
      //DBG_OUTPUT_PORT.println(result);
      file.write((const uint8_t *)result.c_str(), result.length());
      if (file)
        file.close();

      request->send(200, "text/html", "");

    }


    void handlePiskelLoad(AsyncWebServerRequest * request, String id)
    {
      // DBG_OUTPUT_PORT.println("handlePiskelLoad");

      String filename = "/piskeldata/" + id + ".json";
      // DBG_OUTPUT_PORT.println(filename);

      File  file = SPIFFS.open(filename, "r");




      //open the file here

      AsyncResponseStream *response = request->beginResponseStream("application/javascript", 2048);
      response->print(  "window.pskl = window.pskl || {};"
                        "window.pskl.appEngineToken_ = false;"
                        "window.pskl.appEnginePiskelData_ =");

      if (!file)
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

      Dir dir = SPIFFS.openDir("/piskeldata/");

      File tempFile = SPIFFS.open("/piskelindex.json", "w");

      tempFile.print("[");
      int first = 1;
      while (dir.next()) {
        if (first)
          first = 0;
        else
          tempFile.print( ",");


        String number = numberFromPath(dir.fileName());
        File file = SPIFFS.open(dir.fileName(), "r");

        tempFile.print( "{\"" + number + "\":");
        //  response->print(dir.fileName());
        char *buf = (char*)malloc(file.size() + 1);
        if (buf) {
          file.read((uint8_t *)buf, file.size());
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
      else
        _thePOHandler->handleRequest(request, newpath);


      return ;
    }


    void handleRequest(AsyncWebServerRequest * request) {

      if (request->url().startsWith("/piskel/"))
        handleFileReadPiskel(request, request->url());
    }


};




// WEB HANDLER IMPLEMENTATION
class SPIFFSEditor: public AsyncWebHandler {
  private:
    String _username;
    String _password;
    bool _uploadAuthenticated;
  public:
    SPIFFSEditor(String username = String(), String password = String()): _username(username), _password(password), _uploadAuthenticated(false) {}
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


    void handleRequest(AsyncWebServerRequest *request) {
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
          ESP.wdtDisable(); SPIFFS.remove(request->getParam("path", true)->value()); ESP.wdtEnable(10);
          request->send(200, "", "DELETE: " + request->getParam("path", true)->value());
        } else
          request->send(404);
      } else if (request->method() == HTTP_POST) {
        if (request->hasParam("data", true, true) && SPIFFS.exists(request->getParam("data", true, true)->value()))
          request->send(200, "", "UPLOADED: " + request->getParam("data", true, true)->value());
        else
          request->send(500);
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
      if (!index) {
        if (!_username.length() || request->authenticate(_username.c_str(), _password.c_str()))
          _uploadAuthenticated = true;
        request->_tempFile = SPIFFS.open(filename, "w");
      }
      if (_uploadAuthenticated && request->_tempFile && len) {
        ESP.wdtDisable(); request->_tempFile.write(data, len); ESP.wdtEnable(10);
      }
      if (_uploadAuthenticated && final)
        if (request->_tempFile) request->_tempFile.close();
    }
};


// SKETCH BEGIN
AsyncWebServer server(80);
DNSServer dns;
AsyncWiFiManager wifiManager(&server, &dns);

//AsyncWebSocket ws("/ws");

void onEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    os_printf("ws[%s][%u] connect\n", server->url(), client->id());
    client->printf("Hello Client %u :)", client->id());
    client->ping();
  } else if (type == WS_EVT_DISCONNECT) {
    os_printf("ws[%s][%u] disconnect: %u\n", server->url(), client->id());
  } else if (type == WS_EVT_ERROR) {
    os_printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
  } else if (type == WS_EVT_PONG) {
    os_printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len) ? (char*)data : "");
  } else if (type == WS_EVT_DATA) {
    AwsFrameInfo * info = (AwsFrameInfo*)arg;
    String msg = "";
    if (info->final && info->index == 0 && info->len == len) {
      //the whole message is in a single frame and we got all of it's data
      os_printf("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT) ? "text" : "binary", info->len);

      if (info->opcode == WS_TEXT) {
        for (size_t i = 0; i < info->len; i++) {
          msg += (char) data[i];
        }
      } else {
        char buff[3];
        for (size_t i = 0; i < info->len; i++) {
          sprintf(buff, "%02x ", (uint8_t) data[i]);
          msg += buff ;
        }
      }
      os_printf("%s\n", msg.c_str());

      if (info->opcode == WS_TEXT)
        client->text("I got your text message");
      else
        client->binary("I got your binary message");
    } else {
      //message is comprised of multiple frames or the frame is split into multiple packets
      if (info->index == 0) {
        if (info->num == 0)
          os_printf("ws[%s][%u] %s-message start\n", server->url(), client->id(), (info->message_opcode == WS_TEXT) ? "text" : "binary");
        os_printf("ws[%s][%u] frame[%u] start[%llu]\n", server->url(), client->id(), info->num, info->len);
      }

      os_printf("ws[%s][%u] frame[%u] %s[%llu - %llu]: ", server->url(), client->id(), info->num, (info->message_opcode == WS_TEXT) ? "text" : "binary", info->index, info->index + len);

      if (info->opcode == WS_TEXT) {
        for (size_t i = 0; i < info->len; i++) {
          msg += (char) data[i];
        }
      } else {
        char buff[3];
        for (size_t i = 0; i < info->len; i++) {
          sprintf(buff, "%02x ", (uint8_t) data[i]);
          msg += buff ;
        }
      }
      os_printf("%s\n", msg.c_str());

      if ((info->index + len) == info->len) {
        os_printf("ws[%s][%u] frame[%u] end[%llu]\n", server->url(), client->id(), info->num, info->len);
        if (info->final) {
          os_printf("ws[%s][%u] %s-message end\n", server->url(), client->id(), (info->message_opcode == WS_TEXT) ? "text" : "binary");
          if (info->message_opcode == WS_TEXT)
            client->text("I got your text message");
          else
            client->binary("I got your binary message");
        }
      }
    }
  }
}


const char* ssid = "Alan";
const char* password = "***REMOVED***";
const char* http_username = "admin";
const char* http_password = "admin";


extern "C" void system_set_os_print(uint8 onoff);
extern "C" void ets_install_putc1(void* routine);

//Use the internal hardware buffer
static void _u0_putc(char c) {
  while (((U0S >> USTXC) & 0x7F) == 0x7F);
  U0F = c;
}
//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("save config");
  // Setup MDNS responder
  String softap_new_ssid = "WebPixelFrame" + String("_") + String(ESP.getChipId());

  if (!MDNS.begin(softap_new_ssid.c_str()/*myHostname*/)) {
    Serial.println("Error setting up MDNS responder!");
  } else {
    Serial.println("mDNS responder started");
    // Add service to MDNS-SD
    MDNS.addService("http", "tcp", 80);
  }
}
void initSerial() {
  Serial.begin(115200);
  ets_install_putc1((void *) &_u0_putc);
  system_set_os_print(1);
}
DisplayHandler *theDisplay = NULL;



void setup() {
  initSerial();
  SPIFFS.begin();
  theDisplay = new DisplayHandler();

  //WiFi.disconnect(true);
  //MDNS?

  String softap_new_ssid = "WebPixelFrame" + String("_") + String(ESP.getChipId());


#if 0
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.printf("STA: Failed!\n");
    WiFi.disconnect(false);
    delay(1000);
    WiFi.begin(ssid, password);
  }
#endif
  //wifiManager.autoConnect(softap_new_ssid.c_str());
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.startConfigPortalModeless(softap_new_ssid.c_str(), "");
  ArduinoOTA.setPassword((const char *)"");
  ArduinoOTA.begin();
  //Serial.printf("format start\n"); SPIFFS.format();  Serial.printf("format end\n");

  //  ws.onEvent(onEvent);
  //  server.addHandler(&ws);

  POHandler *thePOHandler = new POHandler();
  PiskelHandler *thePiskelHandler = new PiskelHandler(thePOHandler, theDisplay);
  server.on("/heap", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });


  //server.addHandler(new CaptiveHandler()).setFilter(ON_AP_FILTER);
  server.addHandler(thePOHandler).setFilter(ON_STA_FILTER);
  server.addHandler(thePiskelHandler).setFilter(ON_STA_FILTER);
  server.addHandler(theDisplay).setFilter(ON_STA_FILTER);


  server.serveStatic("/fs", SPIFFS, "/").setFilter(ON_STA_FILTER);
  server.addHandler(new SPIFFSEditor(http_username, http_password)).setFilter(ON_STA_FILTER);

  // how do we filter the notfound correctly?
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
    json += ", \"analog\":" + String(analogRead(A0));
    json += ", \"gpio\":" + String((uint32_t)(((GPI | GPO) & 0xFFFF) | ((GP16I & 0x01) << 16)));
    json += "}";
    request->send(200, "text/json", json);
    json = String();
  }).setFilter(ON_STA_FILTER);



  //server.begin();



}

void loop() {
  wifiManager.loop();
  dns.processNextRequest();
  MDNS.update();
  ArduinoOTA.handle();
  if (theDisplay) theDisplay->loop();
}
