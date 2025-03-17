#ifndef MOUVEMENT_H
#define MOUVEMENT_H
/** Classe mouvement : classe abstraite
 * Les classes dérivées correpondent au divers mouvements programmables
 *    aller tout droit
 *    rotation sur soi-même
 *    virage à gauche ou à droite
 *    ... cette liste peut-être complétée
 * Un mouvement doit implanter 
 *    constructeur qui précise les paramètres du mouvement
 *    go pour lancer le mouvement (fonction non bloquante)
 *    setAvoidance pour préciserles paramètres de détecton d'obstacle
 *    updatePosition ui doit faire la mise à jour de la position courante du robot
 *      en tenant compte d'un arrêt sur alarme potentiel
 * 
 * Les unités sont en mm, et degré (sens trigonométrique)
 * Pour mémoire les fonctions sin et cos de C++ utilisent des paramètres en radians
 */
#include <AccelStepper.h>
#include "position.h"
#include "pami.h"



extern AccelStepper stepperG, stepperD;
extern float ppmm; 
extern bool arretSurAlarme;
extern void initDetection(int tofIndice, bool actif, uint16_t seuilMM,bool sens );
extern bool traceMouvement;
extern Position positionCourante;

class Mouvement {
  public:
    virtual void go();
    virtual void setAvoidance();
    virtual void updatePosition();
  protected :
    float _v; // vitesse atteinte après accélération en pulse/s
    float _a; // accélération en pulse/s²
};

class MvtToutDroit : public Mouvement {
  public:
    /** Avancer tout droit
     * @param l distance en mm (toujours positive)
     */
    MvtToutDroit(float l, float v = DEFAULT_PULSE_SPEED, float a = DEFAULT_PULSE_ACCEL);
    void go();
    void setAvoidance();
    void updatePosition();
  private :
    float _l; // distance à parcourir en mm
    long _df; // nombre de pulses correspondant
};

class Rotation : public Mouvement {
  public:
    /** Rotation sur place (axe de rotation millieu de l'essieu)
     * @param d angle de rotation en degré (positif à gauche, négatif à droite)
     *  */  
    Rotation(float d, float v = DEFAULT_PULSE_SPEED, float a = DEFAULT_PULSE_ACCEL);
    void go();
    void setAvoidance();
    void updatePosition();
  private:
    float _d; // angle de rotation (sens trigonométrique : positif à gauche)
    long _p; // traduit en pulse pour la roue gauche
};

class Virage : public Mouvement {
  public:
    /** virage avec un rayon de courbure et un angle 
     * @param r rayon de courbure en mm (doit être > DEMI_ECART_ROUE)
     * @param d angle du virage en degré (sens trigo : négatif à droite, positif à gauche)
     */
    Virage(float r, float d, float v = DEFAULT_PULSE_SPEED, float a = DEFAULT_PULSE_ACCEL);
    void go();
    void setAvoidance();
    void updatePosition();
  private:
    float _r; // rayon de courbure
    float _d; // angle du virage
    long _dx; // nombre de pulses pour le stepper droit (est négatif ou nul)
    long _gx; // nombre de pulses pour le stepper gauche (est posqitif ou nul)
  public:
    bool _error; // indique une erreur de paramètre à la création => mouvement non utilisable
};

#endif