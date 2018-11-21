#pragma once
//#include <ESP8266WiFi.h>
#include <NTPClient.h>

#include "DisplayPixels.h"

#include "C64FontUpper.h"
#include "C64FontLower.h"
#include "font5x7.h"

#define TOTALFONTS    3
#define FONTHEADERSIZE    6
static const unsigned char *fontsPointer[TOTALFONTS]  = {
  font5x7
  , C64FontUpper
  , C64FontLower
};



class DisplayPixelsText : public DisplayPixels {
  public:
    DisplayPixelsText() {
      setFontType(2);
      textColor.R= 128; textColor.G=textColor.B=0;
      
      black.R = black.G = black.B = 0;
      setupStartMillis = millis();
      textStartX = 8;
    }
    void SetText(const char *text) {
      //_text = text;
      strcpy(_text,text);
      textStartX = 8;
    }
    void SetColor(RgbColor newColor)
    {
      textColor = newColor;
      
    }
    virtual void UpdateAnimation(void);
    virtual void NewAnimation(void) {}

  private:
    int drawChar(uint8_t x, uint8_t y, uint8_t c, RgbColor color, uint8_t mode);
    int drawString(uint8_t x, uint8_t y, const char * text, RgbColor color);
    uint8_t setFontType(uint8_t type);
    void pixel(uint8_t x, uint8_t y,  RgbColor color, uint8_t mode);

    char _text[256];
//    String _text;

    RgbColor black;
    RgbColor textColor;
    long setupStartMillis;
    uint8_t foreColor, drawMode, fontWidth, fontHeight, fontType, fontStartChar, fontTotalChar, cursorX, cursorY;
    uint16_t fontMapWidth;
    int textStartX;
};


class DisplayClock : public DisplayPixelsText
{
   public:
      DisplayClock(NTPClient *pTimeClient) : DisplayPixelsText()
      {
        timeClient=pTimeClient;
         NewAnimation();
      }

   virtual void NewAnimation(void)
   {
      String newTime=timeClient->getFormattedTime();
      SetText(newTime.c_str());
   }

   NTPClient *timeClient;
};
