#ifndef POSITION_H
#define POSITION_H

#include <string>

class Position {
  public :
    float _x, _y, _teta;
    Position(float x, float y, float teta){
      _x=x; _y=y; // en mm
      _teta=teta; // en degré sens trigonométrique 
    }
    std::string toString();
    std::string toStringShort();
    /** Set angle, en normalisant l'angle entre -180 et +180
     * @param teta nouvelle orientation
     * @return l'orientation normalisée
     */
    float setTeta(float teta);
    void setPosition(float x, float y, float teta);
};

#endif
