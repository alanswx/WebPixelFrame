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
#include "GIFDecoder.h"

static void gif_pixel_callback(int16_t x, int16_t y, uint8_t red, uint8_t green, uint8_t blue)
{
  ESP.wdtFeed();
  RgbColor color(red, green, blue);
  strip->SetPixelColor(mosaic.Map(x, y), color);
}

static void updateScreenCallback()
{
  ESP.wdtFeed();
  updateScreenCallbackS();
  ESP.wdtFeed();
  strip->Show();
}

static void idleCallback()
{
  ESP.wdtFeed();
  updateScreenCallbackS();
  ESP.wdtFeed();
}


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
          // reset the gif decoder?
          //theFile.close();
          theFile.seek(0, SeekSet);
          gifPlayer->parseGifHeader();
          gifPlayer->parseLogicalScreenDescriptor();
          gifPlayer->parseGlobalColorTable();
          //SetGIF(_file);
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
  private:
    String _file;
    File theFile;
    GifPlayer *gifPlayer;
    int lastFrame = -1;
};




