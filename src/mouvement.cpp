#include "mouvement.h"

// **********************

MvtToutDroit::MvtToutDroit(float x, float v , float a){
    _l = x;
    _v = v;
    _a = a;
};

void MvtToutDroit::go(){
  unsigned long deb = micros();
  stepperG.setMaxSpeed(_v);
  stepperG.setAcceleration(_a);
  stepperD.setMaxSpeed(_v);
  stepperD.setAcceleration(_a);
  stepperG.setCurrentPosition(0);
  stepperD.setCurrentPosition(0);
  _df = ppmm * _l;
  stepperG.moveTo(_df);
  stepperD.moveTo(- _df);
  unsigned long elapsed = micros()-deb;
  if (traceMouvement){
    Serial.print("temps init allerToutDroit "); Serial.println(elapsed);
    Serial.print("nombre de pulses à faire ");Serial.println(_df);
  }
};

void MvtToutDroit::setAvoidance(){

};

void MvtToutDroit::updatePosition(){
  float teta = positionCourante._teta;
  // longueur parcourue effective
  float lp = (arretSurAlarme ? _l/ _df * stepperG.currentPosition() : _l);
  positionCourante._x += lp * cos(teta /180*PI); // teta est en degré
  positionCourante._y += lp * sin(teta /180*PI);
};

// **********************

Rotation::Rotation(float d, float v, float a){
  _v = v;
  _a = a;
  _d = d;
};

void Rotation::go(){
  unsigned long deb = micros();
  stepperG.setMaxSpeed(_v);
  stepperG.setAcceleration(_a);
  stepperD.setMaxSpeed(_v);
  stepperD.setAcceleration(_a);
  stepperG.setCurrentPosition(0);
  stepperD.setCurrentPosition(0);
  // distance à parcourir par chaque roue (en pulse)
  _p = DEMI_ECART_ROUE * PI * _d / 180.0 * ppmm;
  stepperG.moveTo(- _p);
  stepperD.moveTo(- _p);
  unsigned long elapsed = micros()-deb;
  if (traceMouvement){
   Serial.print("temps init rotation "); Serial.println(elapsed);
   Serial.print("nombre de pulses à faire ");Serial.println(_p);
  }
};

void Rotation::setAvoidance(){

};

void Rotation::updatePosition(){
  // rotation effective en degré
  float ap = (arretSurAlarme ? - _d / _p * stepperG.currentPosition() : _d);
  positionCourante._teta += ap;
};

Virage::Virage(float r, float d, float v, float a ){
  _v = v;
  _a = a;
  _error = false;
  _r = r;
  _d = d;
  if (r<DEMI_ECART_ROUE) _error = true;
}

void Virage::go(){
  unsigned long deb = micros();
  if (_error) {
    if (traceMouvement){
      Serial.println("Virage::go : erreur sur paramètre rayon de courbure !");
    }
    return;
  }
  // distance à parcourir pour chaque roue
  int16_t dp = abs(_d); // angle en positif
  // NOTA d peut être supérieur en valeur absolue à 180°, avec 360° on a fera un cercle
  // roue extérieure : elle fait le chemin le plus long
  float exf = (float) (_r + DEMI_ECART_ROUE) * PI * dp / 180.0 * ppmm;
  long ex = (long) exf;
  float ev = _v;
  float ea = _a;
  // roue intérieure
  float ratio = (float) (_r - DEMI_ECART_ROUE)/(_r + DEMI_ECART_ROUE);
  long ix = exf * ratio;
  float iv = _v * ratio ;
  float ia = _a * ratio ;
  // virage à gauche ou à droite
  AccelStepper * stepperExt = & stepperG;
  AccelStepper * stepperInt = & stepperD;
  if (_d<0){ // sens trigo 
    // virage à droite
    ix = -ix; // le moteur droit doit avoir une valeur négative pour avancer
    // mémorise les targets en pulses
    _dx = ix;
    _gx = ex;
  }else{
    // virage à gauche
    stepperExt = & stepperD;
    stepperInt = & stepperG;
    ex = -ex;
    // mémorise les targets en pulses
    _dx = ex;
    _gx = ix;
  }
  // extérieur
  stepperExt->setMaxSpeed(ev);
  stepperExt->setAcceleration(ea);
  stepperExt->setCurrentPosition(0);
  stepperExt->moveTo(ex);
  // intérieur
  stepperInt->setMaxSpeed(iv);
  stepperInt->setAcceleration(ia);
  stepperInt->setCurrentPosition(0);
  stepperInt->moveTo(ix);
  // temps écoulé
  unsigned long elapsed = micros()-deb;
  if (traceMouvement){
    Serial.print("temps init virage "); Serial.println(elapsed);
    Serial.print("nombre de pulses à faire extérieur ");Serial.print(ex);
    Serial.print(" et intérieur ");Serial.println(ix);
  }
}

void Virage::setAvoidance(){

}

void Virage::updatePosition(){
  float d = _d; // angle du virage
  if(arretSurAlarme){
    // virage partiel
    if (d<0){
      // virage à droite, roue gauche extérieure
      d = d * stepperG.currentPosition() / _gx;
    }else {
      // virage à gauche : roue droite extérieure
      d = d * stepperD.currentPosition() / _dx;
    }
  }
  // d repésente l'angle effectivement parcouru (sens trigo toujours !)
  float teta = positionCourante._teta; // orientation en début de virage
  float vTeta = teta + (d<0 ? 90.0 : -90.0); // angle au centre du virage
  // position du centre du virage 
  float cx = positionCourante._x - _r *cos(vTeta*PI/180);
  float cy = positionCourante._y - _r *sin(vTeta*PI/180);
  // position du point d'arrivée 
  float nx = cx + _r * cos((vTeta+d)*PI/180);
  float ny = cy + _r * sin((vTeta+d)*PI/180);
  positionCourante._x = nx;
  positionCourante._y = ny;
  // nouvelle orientation
  positionCourante.setTeta(teta+d);
}