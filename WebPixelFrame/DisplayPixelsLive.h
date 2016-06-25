#pragma once

#include <NeoPixelBus.h>
#include "DisplayPixels.h"

class DisplayPixelsLive : public DisplayPixels {
  public:
    DisplayPixelsLive() {
      black.R = black.G = black.B = 0;
    }

    virtual void UpdateAnimation(void)
    {
      ESP.wdtFeed();
     
    }

    void SetPixel(int x, int y, HtmlColor htmlcolor)
    {
      RgbColor color(htmlcolor);
       strip->SetPixelColor(mosaic.Map(x, y), color);
    }


    void SetPixel(int x, int y, int red, int green, int blue)
    {
        RgbColor color(red, green, blue);
        strip->SetPixelColor(mosaic.Map(x, y), color);

    }
     void Clear()
    {
      
        strip->ClearTo(black);

    }
  private:
    RgbColor black;
};




