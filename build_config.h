#ifndef _BUILD_CONFIG_H_
#define _BUILD_CONFIG_H_

#define BUILD_PROFILE_OLED      1
#define BUILD_PROFILE_HEADLESS  2
#define BUILD_PROFILE_BUTONLU   3

// Otomatik derleme ve senkronizasyon araci (sync_github.py) tarafindan ayarlanir:
#ifndef CURRENT_BUILD_PROFILE
#define CURRENT_BUILD_PROFILE   BUILD_PROFILE_OLED
#endif

#endif // _BUILD_CONFIG_H_
