#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <WiFi.h>
#include "mouvement.h"
#include "position.h"

enum  Mvt {MVT_NONE, MVT_TOUT_DROIT, MVT_ROTATION, MVT_VIRAGE};
enum Sens {SENS_BAD, SENS_DROIT, SENS_GAUCHE};

void connectWIFI();
void execIncomingRequest();



#endif