#ifndef PAMI_H
#define PAMI_H

// caractéristiques du robot 
#define WHEEL_DIAMETER 60.0 // en mm
// ce qui donne un tour de roue = pi*60 = 188.5 mm
#define DEMI_ECART_ROUE 44.0 // en mm

#define STEPS_BY_TOUR (200*8)   // on fonctionne en 1/8 de step


// les moteurs Gauche  et Droit
#define STEP_PIN_MD 18
#define DIR_PIN_MD 5
#define STEP_PIN_MG 4
#define DIR_PIN_MG 19
#define EN_PIN 23

// 4800 pulse/s = 600 step/s en 1/8 soit 3tr/s => ~60cm/s
// 800 pulse/s => ~10 cm/s
#define MAX_PULSE_SPEED 2400   // 30 cm/s, min intervalle entre pulse 416 µs
#define MAX_PULSE_ACCELL 4800  // vitesse atteinte en 0.5s
// si MAX_PULSE_SPEED == MAX_PULSE_ACCELL , la vitesse maximum sera atteinte au bout de 1s

#define DEFAULT_PULSE_SPEED 1600.0 // à 1/8 => ~20 cm/s,
#define DEFAULT_PULSE_ACCEL 4800.0 // vitesse atteinte en 0.33 s


#define MIN_PULSE_WIDTH 5     // 5 µs, le DRV8825 demande 2µs au minimum

#endif