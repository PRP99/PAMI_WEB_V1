# PAMI Web Server

Ce logiciel apporte à notre PAMI un Web Server qui peut se connecter sur un réseau WIFI.

Cela permet d'afficher sur un browser une page de télécommande du PAMI.

On peut demander un mouvement :
- aller tout droit d'une certaine distance
- tourner sur lui-même d'un certain angle, à droite ou à gauche
- faire un virage avec un rayon de courbure, un angle, à droite ou à gauche

On peut gérer la position (x, y, teta)
- reset de l'origine
- get de la position calculée par l'odométrie

On peut enfin demander des informations internes (3 groupes qui peuvent être programmés à des fins de debugging)

Le logiciel est développé sous VSCode et PlatformIO. Il utilise les librairies :
- adafruit/Adafruit SSD1306
- pololu/VL53L0X@^1.3.1
- waspinator/AccelStepper@^1.64
- PCF8575 de  Renzo Mischianti (voir répertoire lib : choix d'option + correction de bug)

Le choix du réseau WIFi doit être fait dans le fichier /src/webServer.h
