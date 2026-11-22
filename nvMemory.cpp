#include "drivers/storage/nvMemory.h"

#ifdef NVMEM_SPIFFS

#include <SPIFFS.h>
#include <FS.h>
#include <ArduinoJson.h>

#include "drivers/devices/device.h"
#include "drivers/storage/storage.h"

nvMemory::nvMemory() : Initialized_(false){};

nvMemory::~nvMemory()
{
    if (Initialized_)
        SPIFFS.end();
};

/// @brief Save settings to config file on SPIFFS
/// @param TSettings* Settings to be saved.
/// @return true on success
bool nvMemory::saveConfig(TSettings* Settings)
{
    if (init())
    {
        // Save Config in JSON format
        Serial.println(F("SPIFS: Saving configuration."));

        // Create a JSON document
        JsonDocument json;
        json[JSON_SPIFFS_KEY_POOLURL] = Settings->PoolAddress;
        json[JSON_SPIFFS_KEY_POOLPORT] = Settings->PoolPort;
        json[JSON_SPIFFS_KEY_POOLPASS] = Settings->PoolPassword;
        json[JSON_SPIFFS_KEY_WALLETID] = Settings->BtcWallet;
        json[JSON_SPIFFS_KEY_TIMEZONE] = Settings->Timezone;
        json[JSON_SPIFFS_KEY_STATS2NV] = Settings->saveStats;
        json[JSON_SPIFFS_KEY_INVCOLOR] = Settings->invertColors;
        json[JSON_SPIFFS_KEY_BRIGHTNESS] = Settings->Brightness;
        json[JSON_SPIFFS_KEY_DEVNAME] = Settings->DeviceName;
        json[JSON_SPIFFS_KEY_AUTOJOIN] = Settings->farmAutoJoin;
        json[JSON_SPIFFS_KEY_CFGVER] = Settings->farmConfigVer;
        json[JSON_SPIFFS_KEY_WIFISSID2] = Settings->WifiSSID2;
        json[JSON_SPIFFS_KEY_WIFIPASS2] = Settings->WifiPW2;
        json[JSON_SPIFFS_KEY_MAXTEMP] = Settings->MaxTemp;

        // Open config file
        File configFile = SPIFFS.open(JSON_CONFIG_FILE, "w");
        if (!configFile)
        {
            // Error, file did not open
            Serial.println("SPIFS: Failed to open config file for writing");
            return false;
        }

        // Serialize JSON data to write to file
        serializeJsonPretty(json, Serial);
        Serial.print('\n');
        if (serializeJson(json, configFile) == 0)
        {
            // Error writing file
            Serial.println(F("SPIFS: Failed to write to file"));
            return false;
        }
        // Close file
        configFile.close();
        return true;
    };
    return false;
}

/// @brief Load settings from config file located in SPIFFS.
/// @param TSettings* Struct to update with new settings.
/// @return true on success
bool nvMemory::loadConfig(TSettings* Settings)
{
    // Uncomment if we need to format filesystem
    // SPIFFS.format();

    // Load existing configuration file
    // Read configuration from FS json

    if (init())
    {
        if (SPIFFS.exists(JSON_CONFIG_FILE))
        {
            // The file exists, reading and loading
            File configFile = SPIFFS.open(JSON_CONFIG_FILE, "r");
            if (configFile)
            {
                Serial.println("SPIFS: Loading config file");
                JsonDocument json;
                DeserializationError error = deserializeJson(json, configFile);
                configFile.close();
                serializeJsonPretty(json, Serial);
                Serial.print('\n');
                if (!error)
                {
                    Settings->PoolAddress = json[JSON_SPIFFS_KEY_POOLURL] | Settings->PoolAddress;
                    strlcpy(Settings->PoolPassword, json[JSON_SPIFFS_KEY_POOLPASS] | Settings->PoolPassword, sizeof(Settings->PoolPassword));
                    strlcpy(Settings->BtcWallet, json[JSON_SPIFFS_KEY_WALLETID] | Settings->BtcWallet, sizeof(Settings->BtcWallet));
                    if (json[JSON_SPIFFS_KEY_POOLPORT] != nullptr)
                        Settings->PoolPort = json[JSON_SPIFFS_KEY_POOLPORT].as<int>();
                    if (json[JSON_SPIFFS_KEY_TIMEZONE] != nullptr)
                        Settings->Timezone = json[JSON_SPIFFS_KEY_TIMEZONE].as<int>();
                    if (json[JSON_SPIFFS_KEY_STATS2NV] != nullptr)
                        Settings->saveStats = json[JSON_SPIFFS_KEY_STATS2NV].as<bool>();
                    if (json[JSON_SPIFFS_KEY_INVCOLOR] != nullptr) {
                        Settings->invertColors = json[JSON_SPIFFS_KEY_INVCOLOR].as<bool>();
                    } else {
                        Settings->invertColors = false;
                    }
                    if (json[JSON_SPIFFS_KEY_BRIGHTNESS] != nullptr) {
                        Settings->Brightness = json[JSON_SPIFFS_KEY_BRIGHTNESS].as<int>();
                    } else {
                        Settings->Brightness = 250;
                    }
                    if (json[JSON_SPIFFS_KEY_DEVNAME] != nullptr) {
                        const char* dname = json[JSON_SPIFFS_KEY_DEVNAME] | "";
                        strlcpy(Settings->DeviceName, dname, sizeof(Settings->DeviceName));
                    }
                    if (json[JSON_SPIFFS_KEY_AUTOJOIN] != nullptr) {
                        Settings->farmAutoJoin = json[JSON_SPIFFS_KEY_AUTOJOIN].as<bool>();
                    } else {
                        Settings->farmAutoJoin = true;
                    }
                    if (json[JSON_SPIFFS_KEY_CFGVER] != nullptr) {
                        Settings->farmConfigVer = json[JSON_SPIFFS_KEY_CFGVER].as<uint32_t>();
                    } else {
                        Settings->farmConfigVer = 1;
                    }
                    if (json[JSON_SPIFFS_KEY_WIFISSID2] != nullptr) {
                        Settings->WifiSSID2 = json[JSON_SPIFFS_KEY_WIFISSID2].as<String>();
                    }
                    if (json[JSON_SPIFFS_KEY_WIFIPASS2] != nullptr) {
                        Settings->WifiPW2 = json[JSON_SPIFFS_KEY_WIFIPASS2].as<String>();
                    }
                    if (json[JSON_SPIFFS_KEY_MAXTEMP] != nullptr) {
                        Settings->MaxTemp = json[JSON_SPIFFS_KEY_MAXTEMP].as<int>();
                    } else {
                        Settings->MaxTemp = 75;
                    }
                    if (Settings->PoolAddress == "public-pool.io" || Settings->PoolAddress == "pool.nerdminers.org") {
                        Serial.println("Migrating pool to pool.nerdminer.io:3333");
                        Settings->PoolAddress = "pool.nerdminer.io";
                        Settings->PoolPort = 3333;
                        saveConfig(Settings);
                    }
                    return true;
                }
                else
                {
                    // Error loading JSON data
                    Serial.println("SPIFS: Error parsing config file!");
                }
            }
            else
            {
                Serial.println("SPIFS: Error opening config file!");
            }
        }
        else
        {
            Serial.println("SPIFS: No config file available!");
        }
    }
    return false;
}

/// @brief Delete config file from SPIFFS
/// @return true on successs
bool nvMemory::deleteConfig()
{
    Serial.println("SPIFS: Erasing config file..");
    return SPIFFS.remove(JSON_CONFIG_FILE); //Borramos fichero
}

/// @brief Prepare and mount SPIFFS
/// @return true on success
bool nvMemory::init()
{
    if (!Initialized_)
    {
        Serial.println("SPIFS: Mounting File System...");
        // May need to make it begin(true) first time you are using SPIFFS
        Initialized_ = SPIFFS.begin(false) || SPIFFS.begin(true);
        Initialized_ ? Serial.println("SPIFS: Mounted") : Serial.println("SPIFS: Mounting failed.");
    }
    else
    {
        Serial.println("SPIFS: Already Mounted");
    }
    return Initialized_;
};

#else

nvMemory::nvMemory() {}
nvMemory::~nvMemory() {}
bool nvMemory::saveConfig(TSettings* Settings) { return false; }
bool nvMemory::loadConfig(TSettings* Settings) { return false; }
bool nvMemory::deleteConfig() { return false; }
bool nvMemory::init() { return false; }


#endif //NVMEM_TYPE
