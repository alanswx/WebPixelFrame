#pragma once

#include <NeoPixelBus.h>

extern void updateScreenCallbackS(void);

void drawPixel(uint16_t x, uint16_t y, uint8_t red, uint8_t green, uint8_t blue)
{
  ESP.wdtFeed();
  RgbColor color(red, green, blue);
  strip->SetPixelColor(mosaic.Map(x, y), color);
}



void fillScreen( uint8_t red, uint8_t green, uint8_t blue)
{
  ESP.wdtFeed();
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
      ESP.wdtFeed();
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
        ESP.wdtFeed();
        lastFrame = millis();
      }
    }

    void SetGIF(String file)
    {
      if (gifPlayer == NULL)
        gifPlayer = new GifPlayer();
      _file = file;
      // close old file?
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
      Dir dir = SPIFFS.openDir(_path);
      while (dir.next()) {

        File entry = dir.openFile("r");
        if (_file.length() == 0)
        {
          SetGIF(entry.name());
          entry.close();
          
        }
        else
        {
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
    }

  private:
    String _file;
    String _path;
    File theFile;
    GifPlayer *gifPlayer;
    int lastFrame = -1;
};





