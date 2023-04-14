#ifndef _DISPLAY_ILI9341_H
#define _DISPLAY_ILI9341_H

#include <Adafruit_GFX.h>     // Core graphics library
#include <Adafruit_ILI9341.h> // Hardware-specific library for ILI9341
#include <SPI.h>
#include "Font.h"

#define TFT_WIDTH                             320
#define TFT_HEIGHT                            240

#define TOTAL_FONT_HEIGHT                     20

#define KOREAN_WIDTH                          20
#define KOREAN_HEIGHT                         TOTAL_FONT_HEIGHT
#define KOREAN                                true    
#define NON_KOREAN                            false

#define MAX_LINE                              14

#define NOTI_MSG_TITLE_Y_POS                  60
#define NOTI_MSG_CONTENT_Y_POS                120   

#define TOP_TEXT_Y_POS                        40

#define BACKGROUND_COLOR                      0x0000

#define LINE_1    0
#define LINE_2    1
#define LINE_3    2
#define LINE_4    3
#define LINE_5    4
#define LINE_6    5
#define LINE_7    6
#define LINE_8    7
#define LINE_9    8
#define LINE_10   9
#define LINE_11   10
#define LINE_12   11
#define LINE_13   12
#define LINE_14   13

#define DEFAULT_ALIGN   0
#define CENTER_ALIGN    1

#define X_START_POS_IN_ALIGN_CENTER           20
#define X_END_POS_IN_ALIGN_CENTER             X_START_POS_IN_ALIGN_CENTER

#define GAP_UNICODE_FONTFILE                  32
#define X_SPACE_TEXT_PIXEL                    1
#define Y_SPACE_TEXT_PIXEL                    5
#define CONSONANTS_VOWELS_INDEX_IN_FONT       95
#define COMPLETE_TYPE_TEXT_INDEX_IN_FONT      189

#define CONSONANTS_VOWELS_INDEX_IN_UNICODE    12593
#define COMPLETE_TYPE_TEXT_INDEX_IN_UNICODE   44032

#define UTF_8_BYTE_1   1
#define UTF_8_BYTE_2   2  
#define UTF_8_BYTE_3   3

#define CALI_VALUE_OF_CV_INDEX (CONSONANTS_VOWELS_INDEX_IN_UNICODE  - CONSONANTS_VOWELS_INDEX_IN_FONT)
#define CALI_VALUE_OF_CT_INDEX (COMPLETE_TYPE_TEXT_INDEX_IN_UNICODE - COMPLETE_TYPE_TEXT_INDEX_IN_FONT)

// SPI 핀 정의 ESP32S3
#define SCLK        12
#define MOSI        11
#define MISO        13
#define CS          10
#define DC          9
#define RST         6
#define BACK_LIGHT  7

#define TOUCH_CLK   36
#define TOUCH_CS    39
#define TOUCH_DIN   35
#define TOUCH_DO    37
#define TOUCH_IRQ   8

// 색상 정의
#define BLACK       0x0000
#define WHITE       0xFFFF
#define RED         0xF800
#define GREEN       0x07E0
#define BLUE        0x001F
#define CYAN        0x07FF
#define MAGENTA     0xF81F
#define YELLOW      0xFFE0
#define ORANGE      0xFD20

#define BACKGROUND  BLACK
#define FONT_COLOR  WHITE

#define FONT_WIDTH  10
#define FONT_HEIGHT 16

#define TOUCH_X_MAX 4095
#define TOUCH_Y_MAX 4095

#define TOTAL_WORD_COUNT 10

typedef struct 
{
  uint16_t start_X_Pos;
  uint16_t end_X_Pos;
  uint8_t numOfLine;
} startEndPos;

extern startEndPos textStartEndPos;

extern uint8_t monthNum, dayNum, hourNum, minNum;

extern bool isStatusScreen;

void setDisplay();

void clearRec(uint8_t xPos, uint8_t yPos, int width, int height);
void clearRecAll();

bool isKoreanUnicode(unsigned int unicodeNum);
bool isKoreanText(char *character, uint8_t startIndex);
bool isASCIIText(char text);

uint8_t checkUTF8_Byte(char *text, uint8_t index);

unsigned int convertUnicodeNumToCode(unsigned int unicode);
unsigned int convertKoreanToUnicodeNum(char *text, uint8_t startIndex);

void printText(uint16_t x, uint16_t y, char *text, uint8_t align, uint16_t color);
void drawText(bool isKorean, uint8_t align, uint16_t *x, uint16_t *y, char *text, uint16_t startIndex, uint8_t start_X_Pos, uint16_t end_X_Pos, uint16_t color);

void calCenterPosOfText(char *text);
uint8_t changeCharToInt(char c);
int calWidthOfText(char *text);

void printTextAlignCenter(uint16_t y, char *text, uint16_t color);


#endif
