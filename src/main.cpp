/* Serveur WEB pour actionner le PAMI via internet
 * sur le réseau wifi local
 *
 * Le serveur écoute les connexion entrante uniquement quand le Pami
 * est inactif (à l'arrêt).
 * En effet le traitement des request HTTP ne permet pas de garantir
 * le timing des impulsions sur les drivers moteur
 *
 * Les actions télécommandables pour cette V1
 *  aller tout droit
 *  rotation sur place
 *  virage
 * 
 * Possiblilté de réupérr les informations de position 
 * et des informations diverses via 3 groupes (A, B et C)
 */

#include <Arduino.h>
#include "webServer.h"

// -------------------------
// pour les drivers moteur
#include <AccelStepper.h>
#include "mouvement.h"
#include "position.h"
#include "pami.h"
// pour affichage sur OLED
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
// pour PCF8575 (16bits IO expander) - notre version corrigée de la bibliothèque
#include <PCF8575.h>
// pour VLO53L0X TOF
#include <VL53L0X.h>
 
// caractéristiques de l'afficheur OLED
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define OLED_I2C_CLK 600000UL // 600kHz
#define DEFAULT_I2C_CLK 400000UL // vitesse du bus i2c après utilisation pour OLED
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET, OLED_I2C_CLK, DEFAULT_I2C_CLK);

// driver 
AccelStepper stepperG(AccelStepper::DRIVER,STEP_PIN_MG,DIR_PIN_MG);
AccelStepper stepperD(AccelStepper::DRIVER,STEP_PIN_MD,DIR_PIN_MD);

// pour PCF8575 (16bits IO expander)
#define PCF8575_ADDRESS 0x20 // Adresse I2C du PCF8575
PCF8575 pcf(PCF8575_ADDRESS);

// pour VLO53L0X TOF
#define TIME_BETWEEN_MEASUREMENT 40000ul //µs
#define TIME_BETWEEN_TOF 5000UL //µs
#define I2C_CLK_FOR_TOF 1000000UL // vitesse du bus I2C pour lire les VL53L0X (400k d'après datacheet)
// pins de l'ESP32 connectées aux XSHUT des VL53L0X
// 20/02/2025 : changement de l'ordre des TOF (droit, gauche, profondeur)
uint8_t xshutPins[3] ={P00, P02, P01};

// adresse par défaut d'un VL53L0X est 0x52 (8bits adresse) 0x29 (7 bits adresse)
// adresses 7 bits, car setAddress attend une adresse 7 bits
byte tofAdresses [3] = {0x2A, 0x2B, 0x2C};
// appelle le constructeur par défaut pour chaque élément du tableau
VL53L0X tof[3];  // objets de la bibliothèque Pololu
uint16_t measureMM[3]; // mesure en mm, 0 pas de mesure valide
unsigned long measureTimeMicros [3]; // time of the last measure en microseconde
unsigned long tofTime[3]; // moment où se fera la lecture du tof, 0 pas de lecture à faire
bool withProfondeur = true; // faire ou non mesure de profondeur

// structure pour les informations de détection d'obstacle pour un TOF
typedef struct  {
  bool actif;
  uint16_t seuilMM;
  bool sens; // true alarme si < seuil, false alarme si > seuil
  unsigned long alarmTimeMillis;  // time de la dernière alarme, 0 sinon
  bool alarm; // alarme active ou non
} detectObstacle; // structure pour la gestion des détection d'obstacle
detectObstacle detect[3];

// gestion du mouvement
float ppmm;  // pulse par mm de trajet roue
bool arretSurAlarme = false; // indicateur d'arrêt sur alarme
bool traceMouvement = true; // trace via Serial des infos mouvements
Mouvement * mvtEnCours = nullptr;
Position positionCourante (0.0, 0.0, 0.0); // 

// FORWARD
// 3 fonctions pour avoir des informations diverses
// à modifier et déplacer dans les fichiers sources en fonction des besoins
String getInfoA(){
  return (String)"Info A";
};
String getInfoB(){
  return (String)"Info B";
};
String getInfoC(){
  return (String)"Info C";
};
// END forward 
 
// fonctions d'attente asynchrone
unsigned long endMillisWaiting = 0; // time de la fin de l'état attente, 0 si pas d'attente
/** Attendre un délai en ms
 * Cette fonction initialise l'attente uniquement, return immédiat. 
 * Pour savoir si le délai est fini utiliser isWaiting()
 * @param delai en ms
 */
void initWait(unsigned long delai){
  endMillisWaiting = millis() + delai;
}
 
/** Vérifie si le délai initialisé par initWait est écoulé
 * @return true si attente, false si délai écoulé
 */
bool isWaiting(){
  if (endMillisWaiting > millis()) return true;
  endMillisWaiting = 0;
  return false;
}
// END fonctions d'attente asynchrone
 
void initMotoDrivers(){
  // moteur Gauche
  // drv8825 : enable if low, step on pulse rising, 
  // il faut donc inverser Enable
  stepperG.setPinsInverted(false,false,true);
  stepperG.setMaxSpeed(MAX_PULSE_SPEED);
  stepperG.setAcceleration(MAX_PULSE_ACCELL);
  stepperG.setMinPulseWidth(5); // minimum pour drv8825 2µs
  // moteur Droit
  stepperD.setPinsInverted(false,false,true); // pas d'effet car on ne féfinit pas l'Enable de ce moteur
  stepperD.setMaxSpeed(MAX_PULSE_SPEED);
  stepperD.setAcceleration(MAX_PULSE_ACCELL);
  stepperD.setMinPulseWidth(5); // minimum pour drv8825 2µs
  // l'enable pin est commun au 2 moteurs et n'est défini que pour le moteur Gauche
  // nota l'enable ne peut être supprimé que de manière explicite par un call à disableOutputs();
  stepperG.setEnablePin(EN_PIN);
}

void desableMotorDrivers(){
// loes 2 enable sont connectés ensemble => action sur une seule pin
stepperG.disableOutputs();
}

void initDisplay(){
// affichage
if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
  Serial.println(F("SSD1306 allocation failed"));
  for(;;); // Don't proceed, loop forever
}
display.clearDisplay();
display.setTextSize(1);             // Normal 1:1 pixel scale
display.setTextColor(WHITE);        // Draw white text
display.setCursor(0,0);             // Start at top-left corner
display.println(F("PAMI"));
display.display();
}

/** Affiche une ligne sur OLED
 * la zone rectangulaire x=0, y , SCREEN_WIDTH, ey est effacée
 * le texte est mis en O,y 
 * et on force le display
 * Cette fonction permet de ne changer qu'une partie de l'information sur OLED
 * On pourra par exemple faire 3 zones haut, milieu et bas pour y placer des infos différentes
 * @param y coordonnée verticale en pixel de la zone
 * @param ey étenddue de la zone
 * @param text à afficher (si le teste est long => passe à la ligne - il faut alors tenir compte de la police en cours)
 */
void afficheLigne(int16_t y, int16_t ey, String text){
  display.fillRect(0,y,SCREEN_WIDTH,ey,SSD1306_BLACK);
  display.setCursor(0,y);  
  display.print(text);
  display.display();
}

void initPcf8575(){
  // PCF8575
  uint8_t inputPins []={P10, P11, P12, P13, P14, P15, P16, P17}; // pour les switch de paramétrage
  for (auto pin : inputPins ){
    pcf.pinMode(pin, INPUT);
  }
  uint8_t outputPins []={P00, P01, P02};  // pour les XSHUT des TOF
  for (auto pin : outputPins ){
    pcf.pinMode(pin, OUTPUT, LOW); // état LOW à l'initialisation
  }
  if (!pcf.begin()) {
    Serial.println("Initialisation PCF8575 failed");
    while(1); // Don't proceed, loop forever
  }
}

// Gestion des alarmes TOF (obstacle ou trou dans le sol)

/** Initialisation des paramètres d'alarme pour les TOF
 * @param [in] tofIndice numéro de TOF 0 à 2
 * @param [in] actif true si détection active
 * @param [in] seuilMM seuil de détection en mm
 * @param [in] sens true alarme si < seuil, false alarme si > seuil
 */
void initDetection(int tofIndice, bool actif, uint16_t seuilMM,bool sens ){
  if (tofIndice<0 || tofIndice>2) return;
  auto d = & detect[tofIndice];
  d->actif = actif;
  d->seuilMM = seuilMM;
  d->sens = sens;
  d->alarm = false;
  d->alarmTimeMillis = 0;
}

/** Initialise les 3 détections en non actif
 */
void resetDetection(){
  initDetection(0,false,0,true);
  initDetection(1,false,0,true);
  initDetection(2,false,0,false);
}

/** Suite à une mesure, interprétation alarme ou non
 * ne déclenche aucune action
 * @param [in] tofIndice numéro de TOF 0 à 2
 */
void testDetect(uint16_t tofIndice){
  if (tofIndice<0 || tofIndice>2) return;
  auto d = & detect[tofIndice];
  if (! d->actif) return;
  bool al = false;
  if (d->sens){
    al = (d->seuilMM > measureMM[tofIndice]);
  } else {
    al = (d->seuilMM < measureMM[tofIndice]);
  }
  if (al){
    d->alarm = true;
    d->alarmTimeMillis = millis();
  }
}

/** Détermine si une alarme a été triggée */
bool isThereAlarm(){
  for (uint16_t i = 0; i<3;i++){
    auto d = & detect[i];
    if (d->actif && d->alarm) return true;
  }
  return false;
}
/** initialise les TOF (VL53L0X) et le cycle de mesures
 * Les lectures de chaque TOF sont espacées d'au moins 40ms (25 lectures par secondes max)
 * Les lectures des 3 tofs sont espacées d'au moins 5 ms 
 * @param [in] doProfondeur si false la lecture du tof 2 (orienté vers le sol) ne sera pas faite pendant le cycle
 */

void initTof(bool doProfondeur = true){
  // VL053L0X
  // reset des VL53L0X fait par initilisation du PCF
  delay(10);  
  // initialisation avec changement d'adresse
  for (int i =0; i<3; i++){
    pcf.digitalWrite(xshutPins[i], HIGH); // enable the chip
    delay(10);
    // init 
    if (!tof[i].init()) {
      Serial.print(F("Failed to boot VL53L0X ")); Serial.println(i);
      while(1); // Don't proceed, loop forever
    }
    // changement d'adresse
    tof[i].setAddress(tofAdresses[i]);
    delay(10);
  }

  // début des mesures de type continue sur les 3 VL53L0X
  for (int i =0; i<3; i++){
    tof[i].startContinuous();
    // par défaut timing budget ~33 ms
    tof[i].setMeasurementTimingBudget(2000);
    // mesures mises à 0
    measureMM[i]=0;
    measureTimeMicros[i]=0;
  }
  // timing pourles lectures 
  withProfondeur = doProfondeur;
  unsigned long now = micros();
  tofTime[0] = now + TIME_BETWEEN_MEASUREMENT;
  tofTime[1] = now + TIME_BETWEEN_MEASUREMENT + TIME_BETWEEN_TOF;
  tofTime[2] = (withProfondeur ? now + TIME_BETWEEN_MEASUREMENT + 2 * TIME_BETWEEN_TOF : 0);
}

/** Lit les valeurs de distance
 * A chaque appel vérifie si c'est le moment de faire une lecture et fait au plus 1 seul TOF
 * Une lecture coute 580 µs avec I2C à 400kbit/s (380 µs avec I2C à 1 Mbit/s, mais non conforme datasheet)
 * En cas d'erreur timeout : lecture nonprise en compte et le TOF passe son tour.
 * @param [in] withProfondeur si false ne fait pas la lecture du tof 2 (orienté vers le sol)
 * @return -1 si aucun TOF lu, sinon indice du TOF lu
 */
int16_t readTofs(){
  static uint16_t tour = 0; // désigne le TOF qui sera lu (0,1 ou 2 si withProfondeur)
  auto now = micros();
  int16_t result = -1;
  if (tofTime[tour] < now ){
    // lecture mesure
    Wire.setClock(I2C_CLK_FOR_TOF);  // pour avoir une lecture très rapide
    auto lecture = tof[tour].readRangeContinuousMillimeters();
    Wire.setClock(DEFAULT_I2C_CLK);
    // si la lecture a échoué, on passe son tour
    if (! tof[tour].timeoutOccurred()){
      measureMM[tour] = lecture;
      measureTimeMicros[tour] = now;
      testDetect(tour);
      result = tour;
    }
    // timing pour prochaine mesure
    tofTime[tour] = now + TIME_BETWEEN_MEASUREMENT;
    // prochain TOF à lire ?
    tour++;
    if (tour>2) tour=0;
    if(!withProfondeur && tour==2) tour=0;
    // respect du time between
    if (tofTime[tour] < now + TIME_BETWEEN_TOF) tofTime[tour] = now + TIME_BETWEEN_TOF;
  }
  return result;
}


void initDiversMvt(){
ppmm = (float)(STEPS_BY_TOUR)/(WHEEL_DIAMETER * PI);
Serial.print("Nombre de pulse par mm "); Serial.println(ppmm);
} 

/** Affiche la tension de la Batterie
* @param [in] position verticale sur OLED (en pixel)
* @return true if battery >5.6V false if not
*/
bool verifieTensionBatterie(){
  uint32_t mv = 3.5 * analogReadMilliVolts(34) ;
  afficheLigne(50, 14,"Batterie "+ String(mv) +" mv");
  if (mv <5600U) {
    afficheLigne(0, 16,"pami BAD BATTERY");
    return false;  
  }
  return true;
}
//----------------------------------------

void setup() {
  Serial.begin(115200);
  initMotoDrivers();
initDisplay();
initPcf8575();
// initialiser et désactiver les alarmes sur TOF

// initialiser les TOF et début des mesures en continue
initTof();
initDiversMvt();
// vérification et affichage tension batterie
if (! verifieTensionBatterie()){
while(1);   // ne rien faire si batterie insuffisante
};

// ajustement de la position initiale
positionCourante.setTeta(0.0);

// Connect to Wi-Fi network 
connectWIFI();

}

void loop() {
  if (!stepperG.isRunning() && ! stepperD.isRunning() && ! isWaiting()){
    // un mouvemetn ou une attente vient de finir ... ou première fois
    if (mvtEnCours != nullptr) {
      mvtEnCours->updatePosition();
      delete mvtEnCours;
      afficheLigne(0,16,"Fin mvt");
      mvtEnCours = nullptr; // car delete ne met pas le pointeur à nullptr
      if (traceMouvement) {
        // pas évident : problème d'allocation / désallocation de mémoire
        Serial.println(positionCourante.toString().c_str());
      }
    }
    // on ne regarde les requêtes sur Web Serveur que si pas en mouvement
    // pour des raisons de timing (peut durer plus de 300ms)
    execIncomingRequest();
  }

  // détection d'obstacle
  if (readTofs()>=0){   // lit 0 ou un seul TOF, pour limiter à 500µs de temps écoulé dans cet appel
    // action en cas d'alarme 
    if (!arretSurAlarme && isThereAlarm()){ // ne pas dupliquer les "stop()"
      arretSurAlarme = true;
      stepperD.stop();
      stepperG.stop();
    }
  }
  // générer des pulse si nécessaire et si on time
  stepperG.run();
  stepperD.run();
}

