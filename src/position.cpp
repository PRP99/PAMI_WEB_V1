#include "position.h"

std::string Position::toString(){
    return "Position courante X= "+  std::to_string(_x) +" Y = "+ std::to_string(_y)+ " orientation "+std::to_string(_teta);
}

std::string Position::toStringShort(){
    return "X= "+  std::to_string((int)_x) +" Y = "+ std::to_string((int)_y)+ " Teta = "+std::to_string((int)_teta);
}

float Position::setTeta(float teta){
    bool negatif =  (teta < 0);
    if (negatif) teta = -teta;
    int i = (int)((teta+180.0)/360.0);
    float t = teta - 360.0 * i;
    if (negatif) t=-t;
    _teta = t;
    return t;
}

void Position::setPosition(float x, float y, float teta){
    _x=x; _y=y; // en mm
    setTeta(teta);
  }