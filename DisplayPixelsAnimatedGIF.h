#pragma once

#include <NeoPixelBus.h>

extern void updateScreenCallbackS(void);

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
      setDrawPixelCallback(gif_pixel_callback);
      setUpdateScreenCallback(updateScreenCallback);
    }

    virtual void UpdateAnimation(void)
    {
      ESP.wdtFeed();
      int result = processGIFFile(_file.c_str());
      ESP.wdtFeed();
    }

    void SetGIF(String file)
    {
      _file = file;
    }
  private:
    String _file;

};




