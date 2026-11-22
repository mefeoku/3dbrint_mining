#ifndef DISPLAY_H
#define DISPLAY_H

#include "displayDriver.h"

extern DisplayDriver *currentDisplayDriver;

void initDisplay();
void alternateScreenState();
void alternateScreenRotation();
void switchToNextScreen();
void switchToPrevScreen();
void resetToFirstScreen();
void drawLoadingScreen();
void showLoadingProgress(const char* statusText, int progressPercent);
void drawSetupScreen();
void drawStatusScreen(const char* title, const char* line1, const char* line2);
void drawCurrentScreen(unsigned long mElapsed);
void animateCurrentScreen(unsigned long frame);
void doLedStuff(unsigned long frame);
bool getDisplayPowerState();

#endif // DISPLAY_H
