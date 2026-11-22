#ifndef _STORAGE_H_
#define _STORAGE_H_

#include <Arduino.h>

// config files

// default settings
#ifndef HAN
#define DEFAULT_SSID		"3dbrint-Miner"
#endif
#define DEFAULT_WIFIPW		"MineYourCoins"
#define DEFAULT_POOLURL		"pool.nerdminer.io"
#define DEFAULT_POOLPASS	"x"
#define DEFAULT_WALLETID	""
#define DEFAULT_POOLPORT	3333
#define DEFAULT_TIMEZONE	3
#define DEFAULT_SAVESTATS	false
#define DEFAULT_INVERTCOLORS	false
#define DEFAULT_BRIGHTNESS	250

// JSON config files
#define JSON_CONFIG_FILE	"/config.json"

// JSON config file SD card (for user interaction, readme.md)
#define JSON_KEY_SSID		"SSID"
#define JSON_KEY_PASW		"WifiPW"
#define JSON_KEY_POOLURL	"PoolUrl"
#define JSON_KEY_POOLPASS	"PoolPassword"
#define JSON_KEY_WALLETID	"BtcWallet"
#define JSON_KEY_POOLPORT	"PoolPort"
#define JSON_KEY_TIMEZONE	"Timezone"
#define JSON_KEY_STATS2NV	"SaveStats"
#define JSON_KEY_INVCOLOR	"invertColors"
#define JSON_KEY_BRIGHTNESS	"Brightness"

// JSON config file SPIFFS (different for backward compatibility with existing devices)
#define JSON_SPIFFS_KEY_POOLURL		"poolString"
#define JSON_SPIFFS_KEY_POOLPORT	"portNumber"
#define JSON_SPIFFS_KEY_POOLPASS	"poolPassword"
#define JSON_SPIFFS_KEY_WALLETID	"btcString"
#define JSON_SPIFFS_KEY_TIMEZONE	"gmtZone"
#define JSON_SPIFFS_KEY_STATS2NV	"saveStatsToNVS"
#define JSON_SPIFFS_KEY_INVCOLOR	"invertColors"
#define JSON_SPIFFS_KEY_BRIGHTNESS	"Brightness"
#define JSON_SPIFFS_KEY_DEVNAME		"deviceName"
#define JSON_SPIFFS_KEY_AUTOJOIN	"farmAutoJoin"
#define JSON_SPIFFS_KEY_CFGVER		"farmConfigVer"
#define JSON_SPIFFS_KEY_WIFISSID2	"wifiSSID2"
#define JSON_SPIFFS_KEY_WIFIPASS2	"wifiPass2"
#define JSON_SPIFFS_KEY_MAXTEMP		"maxTemp"

// settings
struct TSettings
{
	String WifiSSID{ DEFAULT_SSID };
	String WifiPW{ DEFAULT_WIFIPW };
	String WifiSSID2{ "" };
	String WifiPW2{ "" };
	int MaxTemp{ 75 };
	String PoolAddress{ DEFAULT_POOLURL };
	char BtcWallet[80]{ DEFAULT_WALLETID };
	char PoolPassword[80]{ DEFAULT_POOLPASS };
	int PoolPort{ DEFAULT_POOLPORT };
	int Timezone{ DEFAULT_TIMEZONE };
	bool saveStats{ DEFAULT_SAVESTATS };
	bool invertColors{ DEFAULT_INVERTCOLORS };
	int Brightness{ DEFAULT_BRIGHTNESS };
	char DeviceName[24]{ "" };
	bool farmAutoJoin{ true };
	uint32_t farmConfigVer{ 1 };
};

#endif // _STORAGE_H_