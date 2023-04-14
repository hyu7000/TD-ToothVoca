#include "display_ILI9341.h"

startEndPos textStartEndPos;

uint8_t monthNum=0, dayNum=0, hourNum=0, minNum=0;

bool isStatusScreen = true;

Adafruit_ILI9341 tft = Adafruit_ILI9341(CS, DC, MOSI, SCLK, RST, MISO);

void setDisplay()
{
  tft.begin();
  tft.setRotation(3);      
}

void clearRec(uint8_t xPos, uint8_t yPos, int width, int height)
{
  tft.fillRect(xPos, yPos, width, height, BACKGROUND_COLOR);
}

void clearRecAll()
{
  tft.fillRect(0, 0, TFT_WIDTH, TFT_HEIGHT, BACKGROUND_COLOR);
}

bool isInKoreanUnicode(unsigned int unicodeNum)
{
  if(unicodeNum < 0xD7AF && unicodeNum > 0xAC00) return true;

  return false;
}

bool isKoreanText(char *character, uint8_t startIndex)
{
  if(character[startIndex] & 0b11100000 && !(character[startIndex] & 0b00010000))
  {
    if(character[startIndex+1] & 0b10000000 && !(character[startIndex+1] & 0b01000000))
    {
       if(character[startIndex+2] & 0b10000000&& !(character[startIndex+2] & 0b01000000))
       {
        return true;
       }       
    }
  }  
  
  return false;
}

bool isASCIIText(char text)
{
  if(text < 127) return true;

  return false;
}

uint8_t checkUTF8_Byte(char *text, uint8_t index)
{
  if((byte)text[index] < 0b10000000)
  {
    return UTF_8_BYTE_1;
  }
  else if((byte)text[index] < 0b11100000)
  {
    return UTF_8_BYTE_2;
  }
  else if((byte)text[index] < 0b11110000)
  {
    return UTF_8_BYTE_3;
  }
}

unsigned int convertUnicodeNumToCode(unsigned int unicode)
{
  unsigned int code;
  
  if(unicode >= COMPLETE_TYPE_TEXT_INDEX_IN_UNICODE)
  {
    code = unicode - CALI_VALUE_OF_CT_INDEX; //유니코드와 Font.h 파일간의 배열 인덱스 차이 계산
  }
  else
  {
    code = unicode - CALI_VALUE_OF_CV_INDEX; //유니코드와 Font.h 파일간의 배열 인덱스 차이 계산
  }

  return code;
}

unsigned int convertKoreanToUnicodeNum(char *text, uint8_t startIndex)
{ 
  unsigned int num1 = 0, num2 = 0, num3 = 0;

  num1 = text[startIndex] - 0b11100000;
  num1 = num1 << 12;

  num2 = text[startIndex + 1] - 0b10000000;
  num2 = num2 << 6;

  num3 = text[startIndex + 2] - 0b10000000;

  return num1 + num2 + num3;
}

void printText(uint16_t x, uint16_t y, char *text, uint8_t align, uint16_t color)
{
  uint16_t index = 0;
  uint16_t xIndex = x;
  uint16_t yIndex = y;

  uint8_t text_len = strlen(text);

  while(index < text_len)
  {
    if(isASCIIText(text[index]))
    {
      drawText(NON_KOREAN, align, &xIndex, &yIndex, &text[index], 0,     x, TFT_WIDTH, color);
      index++;
    }
    else if(isKoreanText(text, index))
    {
      drawText(KOREAN,     align, &xIndex, &yIndex, text,         index, x, TFT_WIDTH, color);
      index += 3;
    }
    else
    {
      drawText(NON_KOREAN, align, &xIndex, &yIndex, "?",          0,     x, TFT_WIDTH, color);
      index++;
    }
  }
}

void drawText(bool isKorean, uint8_t align, uint16_t *x, uint16_t *y, char *text, uint16_t startIndex, uint8_t start_X_Pos, uint16_t end_X_Pos, uint16_t color)
{
  unsigned int unicodeNum, fontIndex;
  static uint8_t lines = 0;

  if(isKorean)
  {
    unicodeNum = convertKoreanToUnicodeNum(text, startIndex);
    if(isInKoreanUnicode(unicodeNum))
      fontIndex = convertUnicodeNumToCode(unicodeNum);
    else
      fontIndex = 0;
  }
  else
  {
    fontIndex = *text - GAP_UNICODE_FONTFILE;
  }

  if(*x + Font.chars[fontIndex].image->width > end_X_Pos)
  {
    lines++;
    
    if(align == CENTER_ALIGN)
    {
      if(lines < (textStartEndPos.numOfLine))
      {
        *x = X_START_POS_IN_ALIGN_CENTER;
      }
      else
      {
        *x = textStartEndPos.start_X_Pos;
      }      
    }
    else
    {
      *x = start_X_Pos;
    }

    *y += Font.chars[fontIndex].image->height + Y_SPACE_TEXT_PIXEL;    
  }

  tft.drawBitmap(*x,*y,Font.chars[fontIndex].image->data,Font.chars[fontIndex].image->width, Font.chars[fontIndex].image->height, color);

  *x += Font.chars[fontIndex].image->width + X_SPACE_TEXT_PIXEL;  
}

void calCenterPosOfText(char *text)
{
  int widthOfAllText = calWidthOfText(text);

  if(widthOfAllText < TFT_WIDTH)
  {
    textStartEndPos.start_X_Pos = (TFT_WIDTH - widthOfAllText)/2;
    textStartEndPos.numOfLine   = 0;
  }
  else if(widthOfAllText > TFT_WIDTH)
  {
    uint8_t lines = widthOfAllText / TFT_WIDTH;
     
    textStartEndPos.start_X_Pos = (TFT_WIDTH - (widthOfAllText - (TFT_WIDTH - X_START_POS_IN_ALIGN_CENTER * 2) * lines))/2;
    textStartEndPos.numOfLine   = lines;
  }
}

int calWidthOfText(char *text)
{
  uint8_t index = 0;
  uint8_t text_len = strlen(text);
  int xTotal = 0;

  while(index < text_len)
  {
    if(isASCIIText(text[index]))
    {
      xTotal += Font.chars[text[index]-GAP_UNICODE_FONTFILE].image->width + X_SPACE_TEXT_PIXEL;
      index++;
    }
    else if(isKoreanText(text, index))
    {
      xTotal += KOREAN_WIDTH + X_SPACE_TEXT_PIXEL;
      index += 3;
    }
  }
  
  return xTotal;
}

uint8_t changeCharToInt(char c)
{
  return c - '0';
}

void printTextAlignCenter(uint16_t y, char *text, uint16_t color)
{
  uint8_t x;
  
  calCenterPosOfText(text);

  if(textStartEndPos.numOfLine == 0)
  {
    x = textStartEndPos.start_X_Pos;
  }
  else
  {
    x = X_START_POS_IN_ALIGN_CENTER;
  }

  printText(x, y, text, CENTER_ALIGN, color);
}
