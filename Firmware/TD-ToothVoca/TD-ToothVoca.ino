#include <SPI.h>
#include <Wire.h>
#include <XPT2046_Touchscreen.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Ticker.h>
#include <Preferences.h>
#include <WebServer.h>

#include "esp_sleep.h"
#include "display_ILI9341.h"
#include "index_html.h"

#define WORD_Y    40
#define MEANING_Y 70
#define EXAMPLE_Y 120

#define TOTAL_TIME_TO_BURSH  180
#define WORD_NUM_FOR_TIME_T  6
#define TIME_TO_CHANGE_WORD  (TOTAL_TIME_TO_BURSH / WORD_NUM_FOR_TIME_T)

#define MAX_TOKENS 10

#define DEBUG

const char* ap_ssid     = "TD_ToothVoca";
const char* ap_password = "12341234";

String ssid     = "";
String password = "";

String notion_database_id  = "";
String notion_api_key      = "";
String open_ai_api_key     = "";

const char *server_notion  = "api.notion.com"; // Server URL
const char *server_openAI  = "api.openai.com"; // Server URL

//for https
const char *root_ca =
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

// notio database's structure to save and read
typedef struct
{
  int totalCountWord;
  int curIndexToDisplay;
  String enWord[TOTAL_WORD_COUNT];
  String meaning[TOTAL_WORD_COUNT];
} wordInNotion;

wordInNotion wordData;

/*
 * word_space
 * --IS_GET_DATA (verify that you received api key, ap, password and so on)
 * --SSID, PW (wifi data to connect wifi AP)
 * 
 * --N_DB (notion database)
 * --N_API (notion api key)
 * --OPEN_API (open api key)
 * 
 * --W0,W1,W2.W3,W4,W5,W6 (Word)
 * --E0,E1,E2,E3,E4,E5,E6 (Example sentence)
 * --M0,M1,M2,M3,M4,M5,M6 (Meaning of word)
 */
Preferences preferences;

Ticker timer;
Ticker timer_for_word;

WiFiClientSecure client;

WebServer server(80);

XPT2046_Touchscreen touch(TOUCH_CS);

int scale_x, scale_y;
int pre_x = 1000, pre_y = 0;

unsigned long time_ms = 0;

int time_min = 0;
int time_sec = 0;

bool isChangeTime    = false;
bool isChangeWord    = true;

int isTouchForReset = 0;

char timeStr[10];
char nvs_key[5];

String textField1, textField2;

unsigned long timeForReset = 0;

// interrupt function
void IRAM_ATTR touch_isr()
{
  Serial.println("pushed");
}

// 1 sec timer
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

// change index and val to display word, meaning, example sentence
void ICACHE_RAM_ATTR displayWord()
{
  wordData.curIndexToDisplay++;
  if (wordData.curIndexToDisplay == wordData.totalCountWord)
    wordData.curIndexToDisplay = 0;
  isChangeWord = true;
}

// server on
void handleRoot() {
   server.send(200, "text/html", index_html);
}

void handleSave1() {
  if (server.hasArg("ssid") && server.hasArg("password")) {
    ssid     = server.arg("ssid");
    password = server.arg("password");
        
#ifdef DEBUG    
    Serial.print("SSID : ");
    Serial.print(ssid);
    Serial.print(", PW : ");
    Serial.println(password);
#endif

    if(!connect_Wifi()){
      server.send(400, "text/plain", "Bad Request");
    }

    if(WiFi.status() == WL_CONNECTED){
      preferences.putString("SSID", ssid);
      preferences.putString("PW",   password);  
    }
  }
  server.send(200, "text/plain", "Data received");
}

void handleSave2() {
  if (server.hasArg("notion_db") && server.hasArg("notion_api") && server.hasArg("openai")) {
    notion_database_id = server.arg("notion_db");
    notion_api_key     = server.arg("notion_api");
    open_ai_api_key    = server.arg("openai");

    String output_S[MAX_TOKENS], output_Q[MAX_TOKENS];
    int tokens_S = 0, tokens_Q = 0;

    if(splitStringBySlash(notion_database_id, output_S, tokens_S)){
      if(splitStringByQuestion(output_S[tokens_S-1],output_Q, tokens_Q)){
        notion_database_id = output_Q[tokens_Q-1];
      }
    }

    preferences.putString("N_DB",     notion_database_id);
    preferences.putString("N_API",    notion_api_key);
    preferences.putString("OPEN_API", open_ai_api_key);

    preferences.putBool("IS_GET_DATA", true);

#ifdef DEBUG 
    Serial.print("n_db : ");
    Serial.print(notion_database_id);
    Serial.print(", n_api : ");
    Serial.print(notion_api_key);
    Serial.print(", open_ai_api : ");
    Serial.println(open_ai_api_key);
    Serial.print("pIs_get_data : ");
    Serial.print(preferences.getBool("IS_GET_DATA"));
    Serial.print(", pIs_get_data_len : ");
    Serial.println(preferences.getBytesLength("IS_GET_DATA"));
#endif
  }
  server.send(200, "text/plain", "Data received");
}

void serveOnToGetData() {
  char ipStr[20];

  WiFi.softAP(ap_ssid, ap_password);
  
#ifdef DEBUG
  Serial.println("ESP32-S3 Access Point is running");
#endif

  IPAddress IP = WiFi.softAPIP();

  clearRecAll();

  sprintf(ipStr, "IP : %d.%d.%d.%d", IP[0], IP[1], IP[2], IP[3]);
  printTextAlignCenter(WORD_Y, ipStr, FONT_COLOR);

#ifdef DEBUG
  Serial.print("AP IP address: ");
  Serial.println(IP);
#endif

  server.on("/",      HTTP_GET,  handleRoot); 
  server.on("/save",  HTTP_POST, handleSave1);
  server.on("/save2", HTTP_POST, handleSave2);

  server.begin();
#ifdef DEBUG
  Serial.println("Web server started");
  Serial.print("Please wait...");
#endif

  while(!preferences.getBool("IS_GET_DATA")){    
    server.handleClient();
    Serial.print(".");
    delay(100);
  }
}

void nvs_init_TD() {
  ssid               = preferences.getString("SSID");
  password           = preferences.getString("PW");

  notion_database_id = preferences.getString("N_DB");
  notion_api_key     = preferences.getString("N_API");
  open_ai_api_key    = preferences.getString("OPEN_API");

#ifdef DEBUG
  Serial.print("1 : ");
  Serial.print(ssid);
  Serial.print(", 2 : ");
  Serial.print(password);
  Serial.print(", 3 : ");
  Serial.print(notion_database_id);
  Serial.print(", 4 : ");
  Serial.print(notion_api_key);
  Serial.print(", 5 : ");
  Serial.println(open_ai_api_key);
#endif
}

// String -> *char
char *changeToCharArrFromString(String str)
{
  int length = str.length();

  char *char_array = new char[length + 1]; // for null
  str.toCharArray(char_array, length + 1); // copy

  return char_array;
}

bool splitStringBySlash(const String &inputStr, String outputArr[], int &numTokens){
  int startIndex = 0;
  int slashIndex = inputStr.indexOf('/');
  numTokens = 0;

  if(slashIndex != -1) return false;

  while (slashIndex != -1 && numTokens < MAX_TOKENS - 1) {
    outputArr[numTokens++] = inputStr.substring(startIndex, slashIndex);
    startIndex = slashIndex + 1;
    slashIndex = inputStr.indexOf('/', startIndex);
  }

  return true;
}

bool splitStringByQuestion(const String &inputStr, String outputArr[], int &numTokens){
  int startIndex = 0;
  int slashIndex = inputStr.indexOf('?');
  numTokens = 0;

  if(slashIndex != -1) return false;

  while (slashIndex != -1 && numTokens < MAX_TOKENS - 1) {
    outputArr[numTokens++] = inputStr.substring(startIndex, slashIndex);
    startIndex = slashIndex + 1;
    slashIndex = inputStr.indexOf('?', startIndex);
  }

  return true;
}


String notion_api_http_cmd()
{
  String notion_api_http_cmd = String("POST https://api.notion.com/v1/databases/") + notion_database_id + String("/query HTTP/1.1\r\n") +
                               String("Host: api.notion.com\r\n") +
                               String("Authorization: Bearer ") + notion_api_key + String("\r\n") +
                               String("accept: application/json\r\n") +
                               String("Content-Type: application/json\r\n") +
                               String("Notion-Version: 2022-06-28\r\n") +
                               String("Notion-API-Version: 2022-06-28\r\n") +
                               String("Connection: close\r\n\r\n");

  return notion_api_http_cmd;
}

String openAi_api_http_cmd(char *word)
{
  String prompt = "Tell me a short sentence with '" + String(word) + "'.";
  String requestBody = "{\"model\": \"text-davinci-003\",\"prompt\": \"" + prompt + "\", \"max_tokens\": 10, \"temperature\": 0}";

  String openAi_api_http_cmd = String("POST https://api.openai.com/v1/completions HTTP/1.1\r\n") +
                               String("Host: api.openai.com\r\n") +
                               String("Authorization: Bearer ") + open_ai_api_key + String("\r\n") +
                               String("Content-Type: application/json\r\n") +
                               String("Content-Length: ") + String(requestBody.length()) + String("\r\n") +
                               String("\r\n") +
                               requestBody;

  return openAi_api_http_cmd;
}

bool connect_Wifi()
{
  if(WiFi.status() == WL_CONNECTED) return false;

  uint16_t timeoutNum = 0;

  Serial.print("Attempting to connect to SSID: ");
  Serial.println(ssid);
  WiFi.begin(changeToCharArrFromString(ssid), changeToCharArrFromString(password));

  // attempt to connect to Wifi network:
  while (WiFi.status() != WL_CONNECTED)
  {
    // wait 1 second for re-trying
    loop();
    delay(100);
    timeoutNum++;

    if(timeoutNum > 600) return false;
  }

  return true;
}

bool sendRequest(String httpCmd, const char *server_addr)
{
  client.setCACert(root_ca);
  client.setInsecure();
  if (!client.connect(server_addr, 443))
  {
    Serial.print(server_addr);
    Serial.println(" Connection failed!");
    return false;
  }
  else
  {
    Serial.print("Connected to server! : ");
    Serial.println(server_addr);

    // Send a HTTP request:
    client.println(httpCmd);
  }

  return true;
}

// save response after https post/get request
String checkRespose()
{
  String response;

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

  while (client.available())
  {
    response = client.readString();
    loop();
  }

  return response;
}

void request_api_notion()
{
  String response;

  if (sendRequest(notion_api_http_cmd(), server_notion))
  {
    response = checkRespose();

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

    DynamicJsonDocument doc(response.length());
    deserializeJson(doc, response);

    JsonObject obj = doc.as<JsonObject>();
    JsonArray results = obj["results"].as<JsonArray>();

    wordData.totalCountWord = results.size();

    for (int i = 0; i < wordData.totalCountWord; i++)
    {
      wordData.enWord[i]  = obj["results"][i]["properties"]["enWord"]["title"][0]["plain_text"].as<String>();
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

String request_api_openAI(char *word)
{
  String results;

  if (sendRequest(openAi_api_http_cmd(word), server_openAI))
  {
    String response = checkRespose();

    DynamicJsonDocument doc(response.length());
    deserializeJson(doc, response);

    JsonObject obj = doc.as<JsonObject>();
    results = obj["choices"][0]["text"].as<String>();

    results.remove(0, 2);

#ifdef DEBUG
    Serial.print("response Text : ");
    Serial.println(results);
#endif

    client.stop();
  }

  return results;
}

// request openAi api to get example sentences including specific word
void generateExampleSentence()
{
  String wordStr;
  char *charArr;

  for (int i = 0; i < WORD_NUM_FOR_TIME_T; i++)
  {
    sprintf(nvs_key, "W%d", i);
    wordStr = preferences.getString(nvs_key, "");

    sprintf(nvs_key, "E%d", i);
    charArr = changeToCharArrFromString(wordStr);
    preferences.putString(nvs_key, request_api_openAI(charArr));
    free(charArr);
  }
}

void setup()
{
  // begin serial
  Serial.begin(115200);

  // init index
  wordData.curIndexToDisplay = 0;

  setDisplay();
  clearRecAll();

  pinMode(BACK_LIGHT, OUTPUT);
  digitalWrite(BACK_LIGHT, HIGH);

  // begin nvs
  preferences.begin("word_space", false);

  size_t key_length = preferences.getBytesLength("IS_GET_DATA");

  if (!preferences.getBool("IS_GET_DATA"))
    serveOnToGetData();
  else
    nvs_init_TD();

  // set timer interrupt
  timer.attach(1, onTimer); // call onTimer interrupt per 1 sec
  timer_for_word.attach(TIME_TO_CHANGE_WORD, displayWord); // call displayWord interrupt per TIME_TO_CHANGE_WORD sec
 
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
  generateExampleSentence();

  // set touch of display panel
  SPI.begin(TOUCH_CLK, TOUCH_DO, TOUCH_DIN, TOUCH_CS);
  touch.begin();
  touch.setRotation(3);

  pinMode(TOUCH_IRQ, INPUT_PULLUP);

  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT1)
  {
    Serial.println("Woke up due to touch interrupt");
  }

  // Set the interrupt to use when waking up from sleep mode
  // when wake up, set interrupt
  esp_sleep_enable_ext1_wakeup(1ULL << TOUCH_IRQ, ESP_EXT1_WAKEUP_ALL_LOW);
}

void loop()
{
  // if time is changed, draw time to display
  if (isChangeTime)
  {
    sprintf(timeStr, "%02d:%02d", time_min, time_sec);
    clearRec(130, 200, KOREAN_WIDTH * 5, TOTAL_FONT_HEIGHT);
    printTextAlignCenter(200, timeStr, FONT_COLOR);

    isChangeTime = false;
  }
  
  // if word is changed, draw word, meaning, example sentence
  if (isChangeWord)
  {
    char *charArr;

    clearRec(0, WORD_Y, TFT_WIDTH, TOTAL_FONT_HEIGHT * 2 + EXAMPLE_Y);

    sprintf(nvs_key, "W%d", wordData.curIndexToDisplay);
    charArr = changeToCharArrFromString(preferences.getString(nvs_key, ""));
    printTextAlignCenter(WORD_Y, charArr, FONT_COLOR);
    free(charArr);

    sprintf(nvs_key, "M%d", wordData.curIndexToDisplay);
    charArr = changeToCharArrFromString(preferences.getString(nvs_key, ""));
    printTextAlignCenter(MEANING_Y, charArr, FONT_COLOR);
    free(charArr);

    sprintf(nvs_key, "E%d", wordData.curIndexToDisplay);
    charArr = changeToCharArrFromString(preferences.getString(nvs_key, ""));
    printTextAlignCenter(EXAMPLE_Y, charArr, FONT_COLOR);
    free(charArr);

    isChangeWord = false;
  }


  // Reset by pressing the touchpad for more than 5 seconds
  if(digitalRead(TOUCH_IRQ) && millis() > (timeForReset + 1000))
  {
    isTouchForReset++;
    timeForReset = millis();
    
    if(isTouchForReset > 5){
      preferences.clear();
      setup();
    }
  }
}
