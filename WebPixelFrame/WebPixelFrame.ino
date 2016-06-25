/*
  upload the contents of the data folder with MkSPIFFS Tool ("ESP8266 Sketch Data Upload" in Tools menu in Arduino IDE)
  or you can upload the contents of a folder if you CD in that folder and run the following command:
  for file in `ls -A1`; do curl -F "file=@$PWD/$file" esp8266fs.local/edit; done

  access the sample web page at http://esp8266fs.local
  edit the page by going to http://esp8266fs.local/edit


  Design thoughts:
    SPIFFS vs SD Card - why can't we have the file classes be similar, and work together?  maybe a wrapper? or a subclass

*/



#include <ESP8266WiFi.h>   //https://github.com/esp8266/Arduino

#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <NeoPixelBus.h>   // https://github.com/Makuna/NeoPixelBus
//#include <NeoPixelAnimator.h>
#include <NTPClient.h>   // https://github.com/arduino-libraries/NTPClient
//#include <ArduinoJson.h> 
#include <DNSServer.h>
#include "NewBitmap.h"
#include "DisplayPixelsLive.h"
#include "DisplayPixelsText.h"
#include "DisplayPixelsAnimatedGIF.h"

//#include "WiFiManager.h"          //https://github.com/tzapu/WiFiManager

#include "CaptivePortalAdvanced.h"

#define DBG_OUTPUT_PORT Serial

WiFiUDP ntpUDP;

// By default 'time.nist.gov' is used with 60 seconds update interval and
// no offset
NTPClient timeClient(ntpUDP,-28800+(60*60/*DST*/));

#define useNTP 1

NeoPixelBus<MyPixelColorFeature, Neo800KbpsMethod> *strip = new NeoPixelBus<MyPixelColorFeature, Neo800KbpsMethod> (PixelCount, 2);

DisplayPixelsText *pixelText = new DisplayPixelsText();
DisplayPixelsAnimatedGIF *pixelGIF = new DisplayPixelsAnimatedGIF();
DisplayPixelsLive * pixelLive = new DisplayPixelsLive();
DisplayPixels *curPixel = pixelText;
DisplayClock *pixelClock = new DisplayClock(&timeClient);


uint16_t ourLayoutMapCallback(int16_t x, int16_t y)
{

  return mosaic.Map(x, y);

}



bool handleFileRead(String);

ESP8266WebServer server(80);

/* called from animate gif code */
void updateScreenCallbackS()
{
  server.handleClient();
  loopCaptive(); 
 
  MDNS.update();
  
}

//holds the current upload
File fsUploadFile;

//format bytes
String formatBytes(size_t bytes) {
  if (bytes < 1024) {
    return String(bytes) + "B";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + "KB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + "MB";
  } else {
    return String(bytes / 1024.0 / 1024.0 / 1024.0) + "GB";
  }
}

String getContentType(String filename) {
  if (server.hasArg("download")) return "application/octet-stream";
  else if (filename.endsWith(".htm")) return "text/html";
  else if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".png")) return "image/png";
  else if (filename.endsWith(".gif")) return "image/gif";
  else if (filename.endsWith(".jpg")) return "image/jpeg";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".xml")) return "text/xml";
  else if (filename.endsWith(".pdf")) return "application/x-pdf";
  else if (filename.endsWith(".zip")) return "application/x-zip";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  else if (filename.endsWith(".bmp")) return "image/x-windows-bmp";
  return "text/plain";
}


bool handleClearScreen()
{
  pixelLive->Clear();
  curPixel = pixelLive;
  
  server.send(200, "text/plain", "");
  return true;
}

// convert a single hex digit character to its integer value (from https://code.google.com/p/avr-netino/)
unsigned char h2int(char c)
{
  if (c >= '0' && c <= '9') {
    return((unsigned char)c - '0');
  }
  if (c >= 'a' && c <= 'f') {
    return((unsigned char)c - 'a' + 10);
  }
  if (c >= 'A' && c <= 'F') {
    return((unsigned char)c - 'A' + 10);
  }
  return(0);
}

unsigned long hex2int(char *a, unsigned int len)
{
   int i;
   unsigned long val = 0;

   for(i=0;i<len;i++)
      val += h2int(a[i]) *(1<<(4*(len-1-i)));

   return val;
}


bool handleSetPixels()
{
 /*
  int numArgs = server.args();
  for (int i=0;i<numArgs;i++)
  {
     DBG_OUTPUT_PORT.println("arg: " + server.argName(i) + " val:" +server.arg(i));
  }
*/
  String pixels = server.arg("pixels");
  //pixels.toUpperCase();
  int pos=0;
  // let's pull the hex values out
  for (int y=0;y<8;y++)
  {
    for (int x=0;x<8;x++)
    {
       char hr[4];
       hr[0]=pixels[pos++];
       hr[1]=pixels[pos++];
       hr[2]=0;
       int r = hex2int(hr,2);
       char hg[4];       
       hg[0]=pixels[pos++];
       hg[1]=pixels[pos++];
       hg[2]=0;
       int g = hex2int(hg,2);
       char hb[4];
       hb[0]=pixels[pos++];
       hb[1]=pixels[pos++];
       hb[2]=0;
       int b = hex2int(hb,2);
     
       
       pixelLive->SetPixel(x,y,r,g,b);
    }
  }
  

  curPixel = pixelLive;
  
  server.send(200, "text/plain", "");
  return true;  
}

bool handleSetPixel()
{
   if (!server.hasArg("x") || !server.hasArg("y") || !server.hasArg("r") || !server.hasArg("g") ||!server.hasArg("b")) {
    server.send(500, "text/plain", "BAD ARGS");
    return false;
  }
  int x = server.arg("x").toInt();
  int y = server.arg("y").toInt();
  int r = server.arg("r").toInt();
  int g = server.arg("g").toInt();
  int b = server.arg("b").toInt();
  pixelLive->SetPixel(x,y,r,g,b);
  curPixel = pixelLive;
  
  server.send(200, "text/plain", "");
  return true;
}

bool handleShowGIF(String path)
{
  DBG_OUTPUT_PORT.println("handleShowGIF: " + path);


  pixelGIF->SetGIF(path);
  curPixel = pixelGIF;

  server.send(200, "text/plain", "");
  return true;
}

bool handleShow(String path) {
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
//  if (!image.Begin(bitmapFile))
  {
    Serial.println("File format fail, not a supported bitmap");
    // don't do anything more:
    return false;
  }
  server.send(200, "text/plain", "");
  return true;

}


/*
 * If we are autosaving, then we want to increment the next javascript number
 * 
 */

String numberFromPath(String path)
{
  int pos = path.lastIndexOf(".json");
  int slashpos = path.lastIndexOf("/");
  String filenumberS = path.substring(slashpos+1,pos);

  return filenumberS;
}
 
int findNextFileNumber()
{
  int largestnumber=0;
  Dir dir = SPIFFS.openDir("/piskeldata/");
  while (dir.next()) {
    String filenumberS = numberFromPath(dir.fileName());
    
    DBG_OUTPUT_PORT.println(filenumberS);
    int filenumber = filenumberS.toInt();
    filenumber++;
    if (filenumber>largestnumber) largestnumber=filenumber;
  }
  DBG_OUTPUT_PORT.print("largestnumber: ");
 DBG_OUTPUT_PORT.println(largestnumber);
  return largestnumber;
}


void handlePiskelSave(String id) {
  DBG_OUTPUT_PORT.println("handlePiskelSave");

  int numArgs = server.args();
  for (int i=0;i<numArgs;i++)
  {
     DBG_OUTPUT_PORT.println("arg: " + server.argName(i) + " val:" +server.arg(i));
  }

  //String result;
  String filename="";
  if (id)
  {
   filename = "/piskeldata/"+ id+".json";    
  }
  else
  {
  int filenumber = findNextFileNumber();
  DBG_OUTPUT_PORT.println("filenumber:"+filenumber);
   filename = "/piskeldata/"+ String(filenumber)+".json";
  }
  DBG_OUTPUT_PORT.println("filename ["+filename+"]");
  File file = SPIFFS.open(filename,"w");

  //String result = String("window.pskl.appEnginePiskelData_ = {\r\n \"piskel\" :");
  String result = String("{\r\n \"piskel\" :");
  result += server.arg("framesheet");
  result += ",\r\n";
  result +="\"isLoggedIn\": \"true\",";
  result +="\"fps\":"+server.arg("fps")+",\r\n";
  result +="\"descriptor\" : {";
  result +="\"name\": \""+server.arg("name")+"\",";
  result +="\"description\": \""+server.arg("description")+"\",";

  result +="\"first_frame_as_png\": \""+server.arg("first_frame_as_png")+"\",";
  result +="\"framesheet_as_png\": \""+server.arg("framesheet_as_png")+"\",";
  
  
  result +="\"isPublic\": \"false\"";
  result +="}\r\n}";
   DBG_OUTPUT_PORT.println(result);
  file.write((const uint8_t *)result.c_str(),result.length());
  if (file)
    file.close();

  
 server.send(200, "text/plain", "");
}


void handlePiskelLoad(String id)
{
  DBG_OUTPUT_PORT.println("handlePiskelLoad");
 
  String filename = "/piskeldata/"+ id+".json";
  DBG_OUTPUT_PORT.println(filename);
  File  file = SPIFFS.open(filename,"r");


  
  String l_line = "";
//open the file here
 server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
  server.sendContent(
    "window.pskl = window.pskl || {};"
     "window.pskl.appEngineToken_ = false;"
     "window.pskl.appEnginePiskelData_ ="
  );

 if (!file)
 {
  // write an empty file
         
        server.sendContent(
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
  while (file.available() != 0) {  
         
    l_line = file.readStringUntil('\n'); 
    server.sendContent(l_line);
  }
 }
  if (file) file.close();
  server.sendContent(
    ";"
  );
  server.client().stop(); 
}




/*
 * The SPIFFS system has a limit to the number of characters in a file name
 * 
 * We rewrite the files with a shell script to be a number, or number.gz and 
 * then use an index file to map them.
 */

bool handleFileReadPO(String path) {
String short_name = "";
int found=0;

 if (path.endsWith("/")) path += "index.html";
 
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
    String long_name = l_line.substring(pipePos+1);
    long_name="/"+long_name;
    short_name = l_line.substring(0, pipePos);
    
   //parse l_line here
   //DBG_OUTPUT_PORT.println("got line: "+l_line);
   //DBG_OUTPUT_PORT.println("["+long_name+"]["+short_name+"]");

   if (path == long_name)
   {
      DBG_OUTPUT_PORT.println("***** FOUND IT ***");
      DBG_OUTPUT_PORT.println("["+long_name+"]["+short_name+"]");
      found = 1;
      short_name="/po/"+short_name;
      break;
   }
}
file.close();

if (found==0) return false;


  String contentType = getContentType(path);
  String pathWithGz = short_name + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(short_name)) {
    if (SPIFFS.exists(pathWithGz))
      short_name += ".gz";
    File file = SPIFFS.open(short_name, "r");
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
   


}


void handleFilePiskelJSONIndex()
{
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/json", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
  Dir dir = SPIFFS.openDir("/piskeldata/");
  server.sendContent("[");
  int first=1;
  while (dir.next()) {
    if (first)
      first=0;
    else 
      server.sendContent(",");
     
    File file = SPIFFS.open(dir.fileName(), "r");
    String number = numberFromPath(dir.fileName());

    
    server.sendContent("{\""+number+"\":");
    
    server.client().write(file, HTTP_DOWNLOAD_UNIT_SIZE);
    server.sendContent("}");
    file.close();
      
  }
  server.sendContent("]");
  server.client().stop(); // Stop is needed because we sent no content length

}
/*
 *  The piskel code expects url's to look like:
 *  
 *  /piskel/<ID>/index.html
 *  
 *  we are going to pull out the ID and then call the PO routine to handle the actual files
 *  
 */

 
bool handleFileReadPiskel(String path)
{

  if (path=="/piskel/")
  {
     // we should print out our gallery here
     handleFileRead("/gallery.html");
     return true;
  }
  else if (path=="/piskel/json")
  {
     handleFilePiskelJSONIndex();
     return true;
  }

  
   /* move forward /piskel/ characters, and find the next / */
   int piskelLen = 8;
   int pos = path.indexOf("/",piskelLen);
   String filenumberS = path.substring(piskelLen,pos);
   DBG_OUTPUT_PORT.println(filenumberS);

   String newpath = path.substring(pos+1);
   newpath = String("/p/")+newpath;
   DBG_OUTPUT_PORT.println(newpath);

   // if we have the data js file, then we return that, otherwise we return the static files
   if (newpath=="/p/load")
   {
      handlePiskelLoad(filenumberS);
      return true;
   }
   else if (newpath=="/p/save")
   {
      handlePiskelSave(filenumberS);
       return true;
   }
   else
       return handleFileReadPO(newpath);
   
   return true;
}

/*
 * All URLs that haven't been defined in the setup end up going through here
 * 
 * return true if we took care of it, or false, and in setup there is some code to send a 404
 * 
 * We need a path handler in here, to redirect chunks of paths like the /p/ 
 * 
 */

bool handleFileRead(String path) {
  DBG_OUTPUT_PORT.println("handleFileRead: " + path);

  /* Handle the p subdirectory - which looks up files by an index file */
  if (path.startsWith("/p/")) return handleFileReadPO(path);

  if (path.startsWith("/piskel/")) return handleFileReadPiskel(path);
  
  if (path.endsWith("/")) path += "index.htm";
  String contentType = getContentType(path);
  if (contentType == "image/x-windows-bmp")
    return handleShow(path);
  else if (contentType == "image/gif")
    return handleShowGIF(path);
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) {
    if (SPIFFS.exists(pathWithGz))
      path += ".gz";
    File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}

void handleFileUpload() {
  if (server.uri() != "/edit") return;
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    if (!filename.startsWith("/")) filename = "/" + filename;
    DBG_OUTPUT_PORT.print("handleFileUpload Name: "); DBG_OUTPUT_PORT.println(filename);
    fsUploadFile = SPIFFS.open(filename, "w");
    filename = String();
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    //DBG_OUTPUT_PORT.print("handleFileUpload Data: "); DBG_OUTPUT_PORT.println(upload.currentSize);
    if (fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    if (fsUploadFile)
      fsUploadFile.close();
    DBG_OUTPUT_PORT.print("handleFileUpload Size: "); DBG_OUTPUT_PORT.println(upload.totalSize);
  }
}
void handlePiskelFileUpload() {
  DBG_OUTPUT_PORT.println("Piskel Upload");
HTTPUpload& upload = server.upload();
DBG_OUTPUT_PORT.println(upload.status);
DBG_OUTPUT_PORT.println(upload.filename);
DBG_OUTPUT_PORT.println(upload.type);
DBG_OUTPUT_PORT.println(upload.totalSize);
DBG_OUTPUT_PORT.println(upload.currentSize);

 
  int numArgs = server.args();
  for (int i=0;i<numArgs;i++)
  {
     DBG_OUTPUT_PORT.println("arg: " + server.argName(i) + " val:" +server.arg(i));
  }
  
 server.send(200, "text/plain", "");
}


void handleFileDelete() {
  if (server.args() == 0) return server.send(500, "text/plain", "BAD ARGS");
  String path = server.arg(0);
  DBG_OUTPUT_PORT.println("handleFileDelete: " + path);
  if (path == "/")
    return server.send(500, "text/plain", "BAD PATH");
  if (!SPIFFS.exists(path))
    return server.send(404, "text/plain", "FileNotFound");
  SPIFFS.remove(path);
  server.send(200, "text/plain", "");
  path = String();
}

void handleFileCreate() {
  if (server.args() == 0)
    return server.send(500, "text/plain", "BAD ARGS");
  String path = server.arg(0);
  DBG_OUTPUT_PORT.println("handleFileCreate: " + path);
  if (path == "/")
    return server.send(500, "text/plain", "BAD PATH");
  if (SPIFFS.exists(path))
    return server.send(500, "text/plain", "FILE EXISTS");
  File file = SPIFFS.open(path, "w");
  if (file)
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
  while (dir.next()) {
    File entry = dir.openFile("r");

    String contentType = getContentType(entry.name());
    if (contentType == "image/x-windows-bmp")
    {
      output += String("<a href=\"") + String(entry.name()) + "\">" + String(entry.name()) + String("</a><br/>");
    }
    entry.close();
  }
  server.send(200, "text/html", output);
}

void setScrollText()
{
  if (!server.hasArg("text")) {
    server.send(500, "text/plain", "BAD ARGS");
    return;
  }
  String colorS = server.arg("color");
  if (colorS)
  {
    int pos=0;
    char hr[4];
       hr[0]=colorS[pos++];
       hr[1]=colorS[pos++];
       hr[2]=0;
       int r = hex2int(hr,2);
       char hg[4];       
       hg[0]=colorS[pos++];
       hg[1]=colorS[pos++];
       hg[2]=0;
       int g = hex2int(hg,2);
       char hb[4];
       hb[0]=colorS[pos++];
       hb[1]=colorS[pos++];
       hb[2]=0;
       int b = hex2int(hb,2);
    pixelText->SetColor(RgbColor(r,g,b));
  }

  pixelText->SetText(server.arg("text"));

  String output = "Setting scroll to: ";
  output += server.arg("text");

  curPixel = pixelText;

  server.send(200, "text/html", output);

}

void displayClock()
{
  curPixel = pixelClock;
  pixelClock->UpdateAnimation();
  server.send(200, "text/html", "clock started");

}


void handleFileList() {
  if (!server.hasArg("dir")) {
    server.send(500, "text/plain", "BAD ARGS");
    return;
  }

  String path = server.arg("dir");
  DBG_OUTPUT_PORT.println("handleFileList: " + path);
  Dir dir = SPIFFS.openDir(path);
  path = String();

  String output = "[";
  while (dir.next()) {
    File entry = dir.openFile("r");
    if (output != "[") output += ',';
    bool isDir = false;
    output += "{\"type\":\"";
    output += (isDir) ? "dir" : "file";
    output += "\",\"name\":\"";
    output += String(entry.name()).substring(1);
    output += "\"}";
    entry.close();
  }

  output += "]";
  server.send(200, "text/json", output);
}

/*
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
}
*/

void setup(void) {

  DBG_OUTPUT_PORT.begin(115200);
  DBG_OUTPUT_PORT.print("\n");
  DBG_OUTPUT_PORT.setDebugOutput(true);

  WiFi.disconnect();
  
  strip->Begin();
  strip->Show();

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

/*
  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  //reset settings - for testing
  //wifiManager.resetSettings();

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if(!wifiManager.autoConnect()) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  } 

*/

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");



  //SERVER INIT
  setupCaptive(&server);

  server.on("/setpixels",HTTP_POST,handleSetPixels);
  server.on("/displayclock",HTTP_GET,displayClock);
  server.on("/clearscreen",HTTP_GET,handleClearScreen);
  server.on("/setpixel", HTTP_GET, handleSetPixel);
  server.on("/scroll", HTTP_GET, setScrollText);
  server.on("/image", HTTP_GET, showImageList);
  //list directory
  server.on("/list", HTTP_GET, handleFileList);
  //load editor
  server.on("/edit", HTTP_GET, []() {
    if (!handleFileRead("/edit.htm")) server.send(404, "text/plain", "FileNotFound");
  });
  //create file
  server.on("/edit", HTTP_PUT, handleFileCreate);
  //delete file
  server.on("/edit", HTTP_DELETE, handleFileDelete);
  //first callback is called after the request has ended with all parsed arguments
  //second callback handles file uploads at that location
  server.on("/edit", HTTP_POST, []() {
    server.send(200, "text/plain", "");
  }, handleFileUpload);

  server.on("/piskelupload",HTTP_POST,  handlePiskelFileUpload, handlePiskelFileUpload);
 
  //called when the url is not defined here
  //use it to load content from SPIFFS
  server.onNotFound([]() {
     if (captivePortal()) { // If caprive portal redirect instead of displaying the page.
    return;
  }
  
    if (!handleFileRead(server.uri()))
      server.send(404, "text/plain", "FileNotFound");
  });

  //get heap status, analog input value and all GPIO statuses in one json call
  server.on("/all", HTTP_GET, []() {
    String json = "{";
    json += "\"heap\":" + String(ESP.getFreeHeap());
    json += ", \"analog\":" + String(analogRead(A0));
    json += ", \"gpio\":" + String((uint32_t)(((GPI | GPO) & 0xFFFF) | ((GP16I & 0x01) << 16)));
    json += "}";
    server.send(200, "text/json", json);
    json = String();
  });
  server.begin();
  DBG_OUTPUT_PORT.println("HTTP server started");
    timeClient.begin();
 
}

int oldheap=0;

void loop(void) {
  loopCaptive(); 
  server.handleClient();
  MDNS.update();
  timeClient.update();
 
  int heap = ESP.getFreeHeap();
  if (heap!=oldheap)
  {
    
    DBG_OUTPUT_PORT.println("Heap change:");
        DBG_OUTPUT_PORT.print(oldheap);
    DBG_OUTPUT_PORT.print (" ");
    DBG_OUTPUT_PORT.println(heap);
    oldheap=heap;

  }

  if (curPixel) curPixel->UpdateAnimation();
  //image.Blt(*strip,0,0,0,0,8,8,ourLayoutMapCallback);


  //strip.SetPixelColor(mosaic.Map(0, 0), red);
  //strip.Show();
  strip->Show();

}



