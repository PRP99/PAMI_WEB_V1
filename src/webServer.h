#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <WiFi.h>
#include "mouvement.h"
#include "position.h"

// input your local WIFi data
#define MY_WIFI_SSID "Local_SSID"
#define MY_WIFI_PASSWORD "Local_password"

enum  Mvt {MVT_NONE, MVT_TOUT_DROIT, MVT_ROTATION, MVT_VIRAGE};
enum Sens {SENS_BAD, SENS_DROIT, SENS_GAUCHE};

void connectWIFI();
void execIncomingRequest();



#endif
