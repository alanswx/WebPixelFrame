
#include "DisplayPixelsText.h"


void DisplayPixelsText::UpdateAnimation()
{
  /*
    Serial.print("UpdateAnimation Called ");
    Serial.print(textStartX);
    Serial.println(_text);
  */

  //Serial.println("about to create red pixel");
  RgbColor red(128, 0, 0);
  //  strip->SetPixelColor(mosaic.Map(0, 0), red);
  //  strip->Show();
  // delay(1000);
  //   Serial.println("after red pixel");


  drawString(textStartX, 0, _text, red);
  int now = millis();
  if (now -   setupStartMillis  > 100000 / 1000 )
  {
    setupStartMillis = now;
    textStartX--;
    int end = _text.length() * fontWidth * -1;
    if (textStartX <  end) textStartX = 8;
  }

}
/** \brief Draw character with color and mode.
  Draw character c using color and draw mode at x,y.
*/
int  DisplayPixelsText::drawChar(uint8_t x, uint8_t y, uint8_t c, RgbColor color, uint8_t mode) {
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

void DisplayPixelsText::pixel(uint8_t x, uint8_t y,  RgbColor color, uint8_t mode)
{
  // uint8_t i = (y * ledsPerStrip + x);
  if (x >= PanelWidth)
  {
    /* Serial.print("overflow:");
      Serial.print(x);
      Serial.print(",");
      Serial.println(y);
    */
    return;
  } else if (x < 0)
  {
    Serial.println("less than 0");
    return;
  }
  strip->SetPixelColor(mosaic.Map(x, y), color);
  /*
    Serial.print("setPixel:");
    Serial.print(x);
    Serial.print(",");
    Serial.println(y);
  */

}

uint8_t DisplayPixelsText::setFontType(uint8_t type) {
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


int DisplayPixelsText::drawString(uint8_t x, uint8_t y, String text, RgbColor color)
{
  // start at x, and call drawChar for each.. increment x, and return it..
  int length = text.length();
  int last = x;
  int totalWidth = 0;
  for (int i = 0; i < length; i++)
  {
    int width = drawChar(last, 0, text.charAt(i), color, 0);
    last += width;
    totalWidth += width;
  }
  return totalWidth;
}



