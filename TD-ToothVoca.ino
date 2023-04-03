#include <SPI.h>
#include <Wire.h>
#include <XPT2046_Touchscreen.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Ticker.h>
#include <Preferences.h>

#include "esp_sleep.h"
#include "display_ILI9341.h"

#define WORD_Y    70
#define MEANING_Y 120

#define TOTAL_TIME_TO_BURSH 180
#define WORD_NUM_FOR_TIME_T 6
#define TIME_TO_CHANGE_WORD (TOTAL_TIME_TO_BURSH / WORD_NUM_FOR_TIME_T)

#define DEBUG

const char *ssid          = "your network SSID";
const char *password      = "your network password";

String notion_api_key     = "your notion api key";
String open_ai_api_key    = "your open ai api key";

const char *server_notion = "api.notion.com"; // Server URL
const char *server_openAI = "api.openai.com"; // Server URL

const char *test_root_ca =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDdzCCAl+gAwIBAgIEAgAAuTANBgkqhkiG9w0BAQUFADBaMQswCQYDVQQGEwJJ\n"
    "RTESMBAGA1UEChMJQmFsdGltb3JlMRMwEQYDVQQLEwpDeWJlclRydXN0MSIwIAYD\n"
    "VQQDExlCYWx0aW1vcmUgQ3liZXJUcnVzdCBSb290MB4XDTAwMDUxMjE4NDYwMFoX\n"
    "DTI1MDUxMjIzNTkwMFowWjELMAkGA1UEBhMCSUUxEjAQBgNVBAoTCUJhbHRpbW9y\n"
    "ZTETMBEGA1UECxMKQ3liZXJUcnVzdDEiMCAGA1UEAxMZQmFsdGltb3JlIEN5YmVy\n"
    "VHJ1c3QgUm9vdDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAKMEuyKr\n"
    "mD1X6CZymrV51Cni4eiVgLGw41uOKymaZN+hXe2wCQVt2yguzmKiYv60iNoS6zjr\n"
    "IZ3AQSsBUnuId9Mcj8e6uYi1agnnc+gRQKfRzMpijS3ljwumUNKoUMMo6vWrJYeK\n"
    "mpYcqWe4PwzV9/lSEy/CG9VwcPCPwBLKBsua4dnKM3p31vjsufFoREJIE9LAwqSu\n"
    "XmD+tqYF/LTdB1kC1FkYmGP1pWPgkAx9XbIGevOF6uvUA65ehD5f/xXtabz5OTZy\n"
    "dc93Uk3zyZAsuT3lySNTPx8kmCFcB5kpvcY67Oduhjprl3RjM71oGDHweI12v/ye\n"
    "jl0qhqdNkNwnGjkCAwEAAaNFMEMwHQYDVR0OBBYEFOWdWTCCR1jMrPoIVDaGezq1\n"
    "BE3wMBIGA1UdEwEB/wQIMAYBAf8CAQMwDgYDVR0PAQH/BAQDAgEGMA0GCSqGSIb3\n"
    "DQEBBQUAA4IBAQCFDF2O5G9RaEIFoN27TyclhAO992T9Ldcw46QQF+vaKSm2eT92\n"
    "9hkTI7gQCvlYpNRhcL0EYWoSihfVCr3FvDB81ukMJY2GQE/szKN+OMY3EU/t3Wgx\n"
    "jkzSswF07r51XgdIGn9w/xZchMB5hbgF/X++ZRGjD8ACtPhSNzkE1akxehi/oCr0\n"
    "Epn3o0WC4zxe9Z2etciefC7IpJ5OCBRLbf1wbWsaY71k5h+3zvDyny67G7fyUIhz\n"
    "ksLi4xaNmjICq44Y3ekQEe5+NauQrz4wlHrQMz2nZQ/1/I6eYs9HRCwBXbsdtTLS\n"
    "R9I4LtD+gdwyah617jzV/OeBHRnDJELqYzmp\n"
    "-----END CERTIFICATE-----\n";

typedef struct
{
  int totalCountWord;
  int curIndexToDisplay;
  String enWord[TOTAL_WORD_COUNT];
  String meaning[TOTAL_WORD_COUNT];
} wordInNotion;

wordInNotion wordData;

Preferences preferences;

Ticker timer;
Ticker timer_for_word;

WiFiClientSecure client;

XPT2046_Touchscreen touch(TOUCH_CS);

int scale_x, scale_y;
int pre_x = 1000, pre_y = 0;

unsigned long time_ms = 0;
int time_min = 0;
int time_sec = 0;
bool isChangeTime = false;
bool isChangeWord = true;

bool isBeginOpenAI = false;

char timeStr[10];
char nvs_key[5];

 // 인터럽트 처리 함수
 void IRAM_ATTR touch_isr() {
  Serial.println("pushed");
   //..
 }

void ICACHE_RAM_ATTR onTimer()
{
  time_sec++;
  Serial.println(time_sec);

  if (time_sec == 60)
  {
    time_sec = 0;
    time_min++;

    if (time_min == 3)
    {
      Serial.print("3min");
      preferences.end();
      attachInterrupt(digitalPinToInterrupt(TOUCH_IRQ), touch_isr, RISING);
      esp_deep_sleep_start();
    }
  }

  isChangeTime = true;
}

void ICACHE_RAM_ATTR displayWord()
{
  wordData.curIndexToDisplay++;
  if (wordData.curIndexToDisplay == wordData.totalCountWord)
    wordData.curIndexToDisplay = 0;
  isChangeWord = true;
}

char *changeToCharArrFromString(String str)
{
  int length = str.length();

  char *char_array = new char[length + 1]; // 널 문자를 고려한 크기 할당
  str.toCharArray(char_array, length + 1); // 문자열 복사

  return char_array;
}

void connect_Wifi()
{
  Serial.print("Attempting to connect to SSID: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  // attempt to connect to Wifi network:
  while (WiFi.status() != WL_CONNECTED)
  {
    // wait 1 second for re-trying
    loop();
    delay(100);
  }
}

void request_api_notion()
{
  client.setCACert(test_root_ca);

  Serial.println("\nStarting connection to server_notion...");
  client.setInsecure(); // skip verification
  if (!client.connect(server_notion, 443))
    Serial.println("Connection failed!");
  else
  {
    Serial.println("Connected to server_notion!");
    // Make a HTTP request:
    client.println("POST https://api.notion.com/v1/databases/e614fff21e5d479aa696d56b3a2779fa/query HTTP/1.1");
    client.println("Host: api.notion.com");
    client.println(String("Authorization: Bearer ") + notion_api_key);
    client.println("accept: application/json");
    client.println("Content-Type: application/json");
    client.println("Notion-Version: 2022-06-28");
    client.println("Notion-API-Version: 2022-06-28");
    client.println("Connection: close");
    client.println();

    while (client.connected())
    {
      String line = client.readStringUntil('\n');
      if (line == "\r")
      {
        Serial.println("headers received");
        break;
      }
      loop();
    }

    String response;

    // if there are incoming bytes available
    // from the server, read them and print them:
    while (client.available())
    {
      response = client.readString();
#ifdef DEBUG
      Serial.print("response1 : ");
      Serial.println(response);
#endif
      response.remove(0, 5);
#ifdef DEBUG
      Serial.print("length : ");
      Serial.print(response.length() - 6);
      Serial.print(", length_Va : ");
      Serial.println(response[response.length() - 5]);
#endif
      response.remove(response.length() - 6, 6);
#ifdef DEBUG
      Serial.print("response2 : ");
      Serial.println(response);
#endif
      loop();
    }

    DynamicJsonDocument doc(response.length());
    deserializeJson(doc, response);

    JsonObject obj = doc.as<JsonObject>();
    JsonArray results = obj["results"].as<JsonArray>();

    wordData.totalCountWord = results.size();

    for (int i = 0; i < wordData.totalCountWord; i++)
    {
      wordData.enWord[i] = obj["results"][i]["properties"]["enWord"]["title"][0]["plain_text"].as<String>();
      wordData.meaning[i] = obj["results"][i]["properties"]["Meaning"]["rich_text"][0]["plain_text"].as<String>();
#ifdef DEBUG
      Serial.print(", enWord : ");
      Serial.print(wordData.enWord[i]);
      Serial.print(", meaning : ");
      Serial.println(wordData.meaning[i]);
#endif
    }

    uint8_t random_index_arr[6] = {0};

    for (int i = 0; i < WORD_NUM_FOR_TIME_T; i++)
    {
      int randomNum = random(0, wordData.totalCountWord);
      if (i == 0)
      {
        random_index_arr[i] = randomNum;
        continue;
      }

      for (int j = 0; j < i; j++)
      {
        if (random_index_arr[j] == randomNum)
        {
          i--;
          break;
        }

        if (j == (i - 1))
          random_index_arr[i] = randomNum;
      }
    }

    for (int i = 0; i < WORD_NUM_FOR_TIME_T; i++)
    {
      sprintf(nvs_key, "W%d", i);
      preferences.putString(nvs_key, wordData.enWord[random_index_arr[i]]);

      sprintf(nvs_key, "M%d", i);
      preferences.putString(nvs_key, wordData.meaning[random_index_arr[i]]);
    }

    client.stop();
  }
}

void request_api_openAI(char *word)
{
  isBeginOpenAI = true;
  String prompt = "Tell me a short sentence with '" + String(word) + "'.";

  Serial.println(prompt);

  String requestBody = "{\"model\": \"text-davinci-003\",\"prompt\": \"What is the capital of France?\", \"max_tokens\": 10, \"temperature\": 0}";

  Serial.println(requestBody);

  client.setCACert(test_root_ca);

  Serial.println("\nStarting connection to server_openai...");
  client.setInsecure(); // skip verification
  if (!client.connect(server_openAI, 443))
    Serial.println("Connection failed!");
  else
  {
    Serial.println("Connected to server_openAI!");
    // Make a HTTP request:
//    client.println("POST https://api.openai.com/v1/engines/davinci-codex/completions HTTP/1.1");
    client.println("POST https://api.openai.com/v1/completions HTTP/1.1");
    client.println("Host: api.openai.com");
    client.println(String("Authorization: Bearer ") + open_ai_api_key);
//    client.println("accept: application/json");
    client.println("Content-Type: application/json");
    client.println("Content-Length: " + requestBody.length());
    client.println();
    client.print(requestBody);
  }

  Serial.println("Here2");

  while (client.connected())
  {
    String line = client.readStringUntil('\n');
    if (line == "\r")
    {
      Serial.println("headers received");
      break;
    }
    loop();
  }

  String response;
  Serial.println("Here3");

  while (client.available())
  {
    response = client.readString();

    Serial.print("response3 : ");
    Serial.println(response);
    loop();
  }

//  client.stop();
}

void setup()
{
  // 시작 시리얼 통신
  Serial.begin(115200);

  wordData.curIndexToDisplay = 0;

  setDisplay();
  clearRecAll();

  pinMode(BACK_LIGHT, OUTPUT);
  digitalWrite(BACK_LIGHT, HIGH);

  timer.attach(1, onTimer); // 1초마다 onTimer 함수를 호출
  timer_for_word.attach(TIME_TO_CHANGE_WORD, displayWord);

  preferences.begin("word_space", false);

#ifdef DEBUG
  for (int i = 0; i < 6; i++)
  {
    sprintf(nvs_key, "W%d", i);
    Serial.print("W");
    Serial.print(i);
    Serial.print(" : ");
    Serial.println(preferences.getString(nvs_key, ""));
  }
#endif

  connect_Wifi();
  request_api_notion();
  request_api_openAI("allow");

  SPI.begin(TOUCH_CLK, TOUCH_DO, TOUCH_DIN, TOUCH_CS);
  touch.begin();
  touch.setRotation(3);

  pinMode(TOUCH_IRQ, INPUT_PULLUP);

  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT1)
  {
    Serial.println("Woke up due to touch interrupt");
  }

  // 슬립 모드에서 깨어날 때 사용할 인터럽트 설정
   esp_sleep_enable_ext1_wakeup(1ULL << TOUCH_IRQ, ESP_EXT1_WAKEUP_ALL_LOW);
}

void loop()
{
  if (isChangeTime)
  {
    sprintf(timeStr, "%02d:%02d", time_min, time_sec);
    clearRec(130, 200, KOREAN_WIDTH * 5, TOTAL_FONT_HEIGHT);
    printTextAlignCenter(200, timeStr, FONT_COLOR);

    isChangeTime = false;
  }

  if (isChangeWord)
  {
    char* charArr;
    
    clearRec(0, WORD_Y, TFT_WIDTH, TOTAL_FONT_HEIGHT * 2 + MEANING_Y);

    sprintf(nvs_key, "W%d", wordData.curIndexToDisplay);
    charArr = changeToCharArrFromString(preferences.getString(nvs_key, ""));
    printTextAlignCenter(WORD_Y, charArr, FONT_COLOR);
    free(charArr);

    sprintf(nvs_key, "M%d", wordData.curIndexToDisplay);
    charArr = changeToCharArrFromString(preferences.getString(nvs_key, ""));
    printTextAlignCenter(MEANING_Y, charArr, FONT_COLOR);
    free(charArr);

    isChangeWord = false;
  }

  if (client.available() && isBeginOpenAI)
  {
     Serial.println("Here4");     
  }
  
}
