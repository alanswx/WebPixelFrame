#pragma once
#ifdef ESP32
#include "SPIFFS.h"
#endif
#include <NeoPixelBus.h>

extern void updateScreenCallbackS(void);

void drawPixel(uint16_t x, uint16_t y, uint8_t red, uint8_t green, uint8_t blue)
{
  #if defined(ESP8266)
  ESP.wdtFeed();
  #endif
  RgbColor color(red, green, blue);
  strip->SetPixelColor(mosaic.Map(x, y), color);
}



void fillScreen( uint8_t red, uint8_t green, uint8_t blue)
{
  #if defined(ESP8266)
  ESP.wdtFeed();
  #endif
  RgbColor color(red, green, blue);
  strip->ClearTo( color);
}
#include "GifPlayer.h"




class DisplayPixelsAnimatedGIF : public DisplayPixels {
  public:
    DisplayPixelsAnimatedGIF() {
      gifPlayer = NULL;

      //setDrawPixelCallback(gif_pixel_callback);
      //setUpdateScreenCallback(updateScreenCallback);
    }
    virtual void stop()
    {
      Serial.println("gif stop called");
      delete gifPlayer;
      gifPlayer = NULL;
    }
    virtual void UpdateAnimation(void)
    {
      if (gifPlayer == NULL) return;
  #if defined(ESP8266)
  ESP.wdtFeed();
  #endif
      //int result = processGIFFile(_file.c_str());

      //check to see if framedelay has passed
      if ( millis() > lastFrame + gifPlayer->frameDelay * 10)
      {


        unsigned long result = gifPlayer->drawFrame();
        if (result == ERROR_FINISHED)
        {

          Serial.println("gif finished");
          next();
        }
  #if defined(ESP8266)
  ESP.wdtFeed();
  #endif
        lastFrame = millis();
      }
    }

    void SetGIF(String file)
    {
      if (gifPlayer == NULL)
        gifPlayer = new GifPlayer();
      
      _file = file;
      Serial.printf("SetGIF:new file:%s\n",file.c_str());
      // close old file?
      if (theFile) {
        Serial.printf("closing old file\n");
        theFile.close();
      }
      theFile = SPIFFS.open(file, "r");
      gifPlayer->setFile(theFile);
      gifPlayer->parseGifHeader();
      gifPlayer->parseLogicalScreenDescriptor();
      gifPlayer->parseGlobalColorTable();
      lastFrame = millis();
    }


    void SetDIR(String path)
    {
      _path = path;
      next();
    }
    void next()
    {
      if (_path.length() == 0)
      {
        theFile.seek(0, SeekSet);
        gifPlayer->parseGifHeader();
        gifPlayer->parseLogicalScreenDescriptor();
        gifPlayer->parseGlobalColorTable();
        return;
      }
 #ifdef ESP8266 

      // _file hold the last file we played
      
      Dir dir = SPIFFS.openDir(_path);
      while (dir.next()) {
        File entry = dir.openFile("r");

        // if _file is blank, we need to start at the beggining.
        if (_file.length() == 0)
        {
          SetGIF(entry.name());
          entry.close();
          
        }
        else
        {
          // walk the list of files until we find the one stored in _file (the last one)
          // then play the next one
          if (_file == entry.name())
          {
            entry.close();
            // found current file
            if (dir.next())
            {
              File entry = dir.openFile("r");
              SetGIF(entry.name());
              entry.close();
            }
            // if we run out of files, rewind and start at the beginning
            else
            {
              // back to beginning
              Dir dir = SPIFFS.openDir(_path);
              if (dir.next())
              {
                File entry = dir.openFile("r");
                SetGIF(entry.name());
                entry.close();
              }

            }
          }

        }
      }
 #else      
      File dir = SPIFFS.open(_path);

      // find the first file in the directory
      //Serial.printf("openNextFile:\n");
      File entry = dir.openNextFile();

      // if _file is blank, we need to start at the beggining.
      if (_file.length() == 0)
       {
          SetGIF(entry.name());
          entry.close();
          dir.close();
          return;
          
       }
       else
       {
        while (entry)
        {
          //Serial.printf("inside while, entry %x\n",entry);
           // walk the list of files until we find the one stored in _file (the last one)
          // then play the next one
          if (_file == entry.name())
          {
            //Serial.printf("found current file %s %s\n",_file.c_str(),entry.name());
            entry.close();
            // found current file
            if ( entry = dir.openNextFile())
            {
  //            Serial.printf("calling set gif: %x %s\n",entry,entry.name());

              SetGIF(entry.name());
              entry.close();
              dir.close();
    
              return;
            }
            // if we run out of files, rewind and start at the beginning
            else
            {
              // back to beginning
              Serial.printf("back to beginning");
              dir.rewindDirectory();

              if (entry = dir.openNextFile())
              {
                SetGIF(entry.name());
                entry.close();
                dir.close();

                return;
              }

            }
          }
           entry = dir.openNextFile();
        }
       }
       
        dir.close();

      
 
 #endif

    }

  private:
    String _file;
    String _path;
    File theFile;
    GifPlayer *gifPlayer;
    int lastFrame = -1;
};
