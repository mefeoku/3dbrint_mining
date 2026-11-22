#ifndef _GITHUB_OTA_H_
#define _GITHUB_OTA_H_

#include <Arduino.h>
#include <WebServer.h>

#define GITHUB_OTA_DEFAULT_REPO "mefeoku/3dbrint_mining"
#define GITHUB_OTA_BRANCH "main"
#define GITHUB_OTA_MANIFEST_URL "https://raw.githubusercontent.com/" GITHUB_OTA_DEFAULT_REPO "/" GITHUB_OTA_BRANCH "/version.json"

String otaGetHardwareKey();
String otaGetHardwareTitle();

bool otaCheckUpdate(String& outNewVer, String& outBinUrl, String& outNotes, bool& outHasUpdate);
bool otaPerformGithubUpdate(const String& binUrl);
void otaRegisterRoutes(WebServer* server);
void otaCheckPeriodic();

#endif // _GITHUB_OTA_H_
