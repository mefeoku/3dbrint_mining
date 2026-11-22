#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "HTTPClient.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <list>
#include "mining.h"
#include "utils.h"
#include "monitor.h"
#include "drivers/storage/storage.h"
#include "drivers/devices/device.h"

extern uint32_t templates;
extern uint32_t hashes;
extern uint32_t Mhashes;
extern uint32_t totalKHashes;
extern uint32_t elapsedKHs;
extern uint64_t upTime;

extern volatile uint32_t shares; // increase if blockhash has 32 bits of zeroes
extern volatile uint32_t valids; // increased if blockhash <= targethalfshares

extern double best_diff; // track best diff

extern monitor_data mMonitor;

//from saved config
extern TSettings Settings; 
bool invertColors = false;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);
unsigned int bitcoin_price=0;
String current_block = "793261";
global_data gData;
pool_data pData;
String poolAPIUrl;
double g_lastHashrateKHs = 0.0;


void setup_monitor(void){
    /******** TIME ZONE SETTING *****/
    configTime(3600 * Settings.Timezone, 0, "pool.ntp.org", "time.google.com");
    timeClient.begin();
    timeClient.setTimeOffset(3600 * Settings.Timezone);

    // Ilk giriste internet uzerinden canli saat bilgisini hemen cek
    if (WiFi.status() == WL_CONNECTED) {
        for (int i = 0; i < 3; i++) {
            if (timeClient.forceUpdate()) {
                time_t epoch = timeClient.getEpochTime();
                struct timeval tv = { .tv_sec = epoch, .tv_usec = 0 };
                settimeofday(&tv, NULL);
                Serial.println("NTP Saat senkronize edildi: " + timeClient.getFormattedTime());
                break;
            }
            delay(300);
        }
    }

    Serial.println("TimeClient setup done");
#ifdef SCREEN_WORKERS_ENABLE
    poolAPIUrl = getPoolAPIUrl();
    Serial.println("poolAPIUrl: " + poolAPIUrl);
#endif
}

unsigned long mGlobalUpdate = 0;

void updateGlobalData(void) {
    if ((mGlobalUpdate == 0) || (millis() - mGlobalUpdate > UPDATE_Global_min * 60 * 1000)) {
        mGlobalUpdate = millis(); // Set immediately to prevent rapid retries on failure
        if (WiFi.status() != WL_CONNECTED) return;
            
        WiFiClientSecure secureClient;
        secureClient.setInsecure();
        HTTPClient http;
        http.setTimeout(2500); // Max 2.5 seconds timeout
        try {
            if (http.begin(secureClient, getGlobalHash)) {
                int httpCode = http.GET();
                if (httpCode == HTTP_CODE_OK) {
                    String payload = http.getString();
                    StaticJsonDocument<1024> doc;
                    deserializeJson(doc, payload);
                    String temp = "";
                    if (doc.containsKey("currentHashrate")) temp = String(doc["currentHashrate"].as<float>());
                    if (temp.length() > 18 + 3) {
                        gData.globalHash = temp.substring(0, temp.length() - 18 - 3);
                    }
                    if (doc.containsKey("currentDifficulty")) temp = String(doc["currentDifficulty"].as<float>());
                    if (temp.length() > 10 + 3) {
                        temp = temp.substring(0, temp.length() - 10 - 3);
                        gData.difficulty = temp.substring(0, temp.length() - 2) + "." + temp.substring(temp.length() - 2, temp.length()) + "T";
                    }
                    doc.clear();
                }
                http.end();
            }
        } catch (...) {
            Serial.println("Global data HTTP error caught");
            http.end();
        }
    }
}

unsigned long mHeightUpdate = 0;

String getBlockHeight(void) {
    if ((mHeightUpdate == 0) || (millis() - mHeightUpdate > UPDATE_Height_min * 60 * 1000)) {
        mHeightUpdate = millis(); // Set immediately
        if (WiFi.status() != WL_CONNECTED) return current_block;
            
        WiFiClientSecure secureClient;
        secureClient.setInsecure();
        HTTPClient http;
        http.setTimeout(2500);
        try {
            if (http.begin(secureClient, getHeightAPI)) {
                int httpCode = http.GET();
                if (httpCode == HTTP_CODE_OK) {
                    String payload = http.getString();
                    payload.trim();
                    if (payload.length() > 0 && payload.length() < 12) {
                        current_block = payload;
                    }
                }
                http.end();
            }
        } catch (...) {
            Serial.println("Height HTTP error caught");
            http.end();
        }
    }
    return current_block;
}

unsigned long mBTCUpdate = 0;

String getBTCprice(void) {
    // BTC fiyati kaldirildi - gereksiz HTTP cagrisi yapilmiyor
    return String("");
}

unsigned long mTriggerUpdate = 0;
unsigned long initialMillis = millis();
unsigned long initialTime = 0;
unsigned long mPoolUpdate = 0;

void getTime(unsigned long* currentHours, unsigned long* currentMinutes, unsigned long* currentSeconds){
  time_t now;
  time(&now);
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);

  if (timeinfo.tm_year + 1900 < 2024 && WiFi.status() == WL_CONNECTED) {
    if (timeClient.update()) {
      time_t epoch = timeClient.getEpochTime();
      if (epoch > 1700000000) {
        struct timeval tv = { .tv_sec = epoch, .tv_usec = 0 };
        settimeofday(&tv, NULL);
        localtime_r(&epoch, &timeinfo);
      }
    }
  }

  *currentHours = timeinfo.tm_hour;
  *currentMinutes = timeinfo.tm_min;
  *currentSeconds = timeinfo.tm_sec;
}

String getDate(){
  time_t now;
  time(&now);
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);

  if (timeinfo.tm_year + 1900 < 2024 && WiFi.status() == WL_CONNECTED) {
    if (timeClient.update()) {
      time_t epoch = timeClient.getEpochTime();
      if (epoch > 1700000000) {
        struct timeval tv = { .tv_sec = epoch, .tv_usec = 0 };
        settimeofday(&tv, NULL);
        localtime_r(&epoch, &timeinfo);
      }
    }
  }

  char currentDate[20];
  if (timeinfo.tm_year + 1900 >= 2024) {
    sprintf(currentDate, "%02d.%02d.%04d", timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);
  } else {
    sprintf(currentDate, "--.--.----");
  }

  return String(currentDate);
}

String getTime(void){
  unsigned long currentHours, currentMinutes, currentSeconds;
  getTime(&currentHours, &currentMinutes, &currentSeconds);

  char LocalHour[10];
  sprintf(LocalHour, "%02d:%02d", currentHours, currentMinutes);
  
  String mystring(LocalHour);
  return LocalHour;
}

enum EHashRateScale
{
  HashRateScale_99KH,
  HashRateScale_999KH,
  HashRateScale_9MH
};

static EHashRateScale s_hashrate_scale = HashRateScale_99KH;
static uint32_t s_skip_first = 3;
static double s_top_hashrate = 0.0;

static std::list<double> s_hashrate_avg_list;
static double s_hashrate_summ = 0.0;
static uint8_t s_hashrate_recalc = 0;

String getCurrentHashRate(unsigned long mElapsed)
{
  double hashrate = (double)elapsedKHs * 1000.0 / (double)mElapsed;

  s_hashrate_summ += hashrate;
  s_hashrate_avg_list.push_back(hashrate);
  if (s_hashrate_avg_list.size() > 10)
  {
    s_hashrate_summ -= s_hashrate_avg_list.front();
    s_hashrate_avg_list.pop_front();
  }

  ++s_hashrate_recalc;
  if (s_hashrate_recalc == 0)
  {
    s_hashrate_summ = 0.0;
    for (auto itt = s_hashrate_avg_list.begin(); itt != s_hashrate_avg_list.end(); ++itt)
      s_hashrate_summ += *itt;
  }

  double avg_hashrate = s_hashrate_summ / (double)s_hashrate_avg_list.size();
  if (avg_hashrate < 0.0)
    avg_hashrate = 0.0;

  g_lastHashrateKHs = is_mining_paused_flag ? 0.0 : avg_hashrate;

  if (s_skip_first > 0)
  {
    s_skip_first--;
  } else
  {
    if (avg_hashrate > s_top_hashrate)
    {
      s_top_hashrate = avg_hashrate;
      if (avg_hashrate > 999.9)
        s_hashrate_scale = HashRateScale_9MH;
      else if (avg_hashrate > 99.9)
        s_hashrate_scale = HashRateScale_999KH;
    }
  }

  switch (s_hashrate_scale)
  {
    case HashRateScale_99KH:
      return String(avg_hashrate, 2);
    case HashRateScale_999KH:
      return String(avg_hashrate, 1);
    default:
      return String((int)avg_hashrate );
  }
}

mining_data getMiningData(unsigned long mElapsed)
{
  mining_data data;

  char best_diff_string[16] = {0};
  suffix_string(best_diff, best_diff_string, 16, 0);

  char timeMining[15] = {0};
  uint64_t tm = upTime;
  int secs = tm % 60;
  tm /= 60;
  int mins = tm % 60;
  tm /= 60;
  int hours = tm % 24;
  int days = tm / 24;
  sprintf(timeMining, "%01d  %02d:%02d:%02d", days, hours, mins, secs);

  data.completedShares = shares;
  data.totalMHashes = Mhashes;
  data.totalKHashes = totalKHashes;
  data.currentHashRate = getCurrentHashRate(mElapsed);
  data.templates = templates;
  data.bestDiff = best_diff_string;
  data.timeMining = timeMining;
  data.valids = valids;
  data.temp = String(temperatureRead(), 0);
  data.currentTime = getTime();

  return data;
}

clock_data getClockData(unsigned long mElapsed)
{
  clock_data data;

  data.completedShares = shares;
  data.totalKHashes = totalKHashes;
  data.currentHashRate = getCurrentHashRate(mElapsed);
  data.btcPrice = getBTCprice();
  data.blockHeight = getBlockHeight();
  data.currentTime = getTime();
  data.currentDate = getDate();

  return data;
}

clock_data_t getClockData_t(unsigned long mElapsed)
{
  clock_data_t data;

  data.valids = valids;
  data.currentHashRate = getCurrentHashRate(mElapsed);
  getTime(&data.currentHours, &data.currentMinutes, &data.currentSeconds);

  return data;
}

coin_data getCoinData(unsigned long mElapsed)
{
  coin_data data;

  updateGlobalData(); // Update gData vars asking mempool APIs

  data.completedShares = shares;
  data.totalKHashes = totalKHashes;
  data.currentHashRate = getCurrentHashRate(mElapsed);
  data.btcPrice = "";
  data.currentTime = getTime();
  data.halfHourFee = "";
  data.netwrokDifficulty = (gData.difficulty.length() > 0) ? gData.difficulty : "~105 T";
  data.globalHashRate = (gData.globalHash.length() > 0) ? gData.globalHash : "~750";
  data.blockHeight = getBlockHeight();

  unsigned long currentBlock = data.blockHeight.toInt();
  unsigned long remainingBlocks = (((currentBlock / HALVING_BLOCKS) + 1) * HALVING_BLOCKS) - currentBlock;
  data.progressPercent = (HALVING_BLOCKS - remainingBlocks) * 100 / HALVING_BLOCKS;
  data.remainingBlocks = String(remainingBlocks) + " BLOCKS";

  return data;
}

String getPoolAPIUrl(void) {
    poolAPIUrl = String(getPublicPool);
    if (Settings.PoolAddress == "pool.nerdminer.io") {
        poolAPIUrl = "https://pool.nerdminer.io/api/miner?address=";
    }
    else if (Settings.PoolAddress == "public-pool.io") {
        poolAPIUrl = "https://public-pool.io:40557/api/client/";
    } 
    else {
        if (Settings.PoolAddress == "pool.nerdminers.org") {
            poolAPIUrl = "https://pool.nerdminers.org/users/";
        }
        else {
            switch (Settings.PoolPort) {
                case 3333:
                    if (Settings.PoolAddress == "pool.sethforprivacy.com")
                        poolAPIUrl = "https://pool.sethforprivacy.com/api/client/";
                    if (Settings.PoolAddress == "pool.solomining.de")
                        poolAPIUrl = "https://pool.solomining.de/api/client/";
                    // Add more cases for other addresses with port 3333 if needed
                    break;
                case 2018:
                    // Local instance of public-pool.io on Umbrel or Start9
                    poolAPIUrl = "http://" + Settings.PoolAddress + ":2019/api/client/";
                    break;
                default:
                    poolAPIUrl = String(getPublicPool);
                    break;
            }
        }
    }
    return poolAPIUrl;
}

pool_data getPoolData(void){
    //pool_data pData;    
    if((mPoolUpdate == 0) || (millis() - mPoolUpdate > UPDATE_POOL_min * 60 * 1000)){      
        if (WiFi.status() != WL_CONNECTED) return pData;            
        //Make first API call to get global hash and current difficulty
        HTTPClient http;
        http.setTimeout(10000);        
        try {          
          String btcWallet = Settings.BtcWallet;
          // Serial.println(btcWallet);
          if (btcWallet.indexOf(".")>0) btcWallet = btcWallet.substring(0,btcWallet.indexOf("."));
#ifdef SCREEN_WORKERS_ENABLE
          Serial.println("Pool API : " + poolAPIUrl+btcWallet);
          http.begin(poolAPIUrl+btcWallet);
#else
          http.begin(String(getPublicPool)+btcWallet);
#endif
          int httpCode = http.GET();
          if (httpCode == HTTP_CODE_OK) {
              String payload = http.getString();
              // Serial.println(payload);
              StaticJsonDocument<300> filter;
              filter["bestDifficulty"] = true;
              filter["bestshare"] = true;
              filter["workersCount"] = true;
              filter["workers"] = true;
              filter["hashrate1m"] = true;
              filter["workers"][0]["sessionId"] = true;
              filter["workers"][0]["hashRate"] = true;
              StaticJsonDocument<2048> doc;
              deserializeJson(doc, payload, DeserializationOption::Filter(filter));
              //Serial.println(serializeJsonPretty(doc, Serial));
              if (doc.containsKey("workersCount")) pData.workersCount = doc["workersCount"].as<int>();
              else if (doc.containsKey("workers") && doc["workers"].is<int>()) pData.workersCount = doc["workers"].as<int>();

              if (doc.containsKey("hashrate1m")) {
                  pData.workersHash = doc["hashrate1m"].as<String>();
              } else {
                  const JsonArray& workers = doc["workers"].as<JsonArray>();
                  float totalhashs = 0;
                  for (const JsonObject& worker : workers) {
                    totalhashs += worker["hashRate"].as<double>();
                  }
                  char totalhashs_s[16] = {0};
                  suffix_string(totalhashs, totalhashs_s, 16, 0);
                  pData.workersHash = String(totalhashs_s);
              }

              double temp = 0;
              if (doc.containsKey("bestDifficulty")) {
                  temp = doc["bestDifficulty"].as<double>();            
              } else if (doc.containsKey("bestshare")) {
                  temp = doc["bestshare"].as<double>();
              }
              if (temp > 0) {
                  char best_diff_string[16] = {0};
                  suffix_string(temp, best_diff_string, 16, 0);
                  pData.bestDifficulty = String(best_diff_string);
              }
              doc.clear();
              mPoolUpdate = millis();
              Serial.println("\n####### Pool Data OK!");               
          } else {
              Serial.println("\n####### Pool Data HTTP Error!");    
              /* Serial.println(httpCode);
              String payload = http.getString();
              Serial.println(payload); */
              // mPoolUpdate = millis();
              pData.bestDifficulty = "P";
              pData.workersHash = "E";
              pData.workersCount = 0;
              http.end();
              return pData; 
          }
          http.end();
        } catch(...) {
          Serial.println("####### Pool Error!");          
          // mPoolUpdate = millis();
          pData.bestDifficulty = "P";
          pData.workersHash = "Error";
          pData.workersCount = 0;
          http.end();
          return pData;
        } 
    }
    return pData;
}
