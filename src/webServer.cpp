#include "webServer.h"

//Accès à la position courante de l'Odométrie
extern Position positionCourante;
//Pour avoir informations groupe A,B et C
extern String getInfoA();
extern String getInfoB();
extern String getInfoC();
// pour initier un mouvement
extern Mouvement * mvtEnCours;
extern void initDetection(int tofIndice, bool actif, uint16_t seuilMM,bool sens );
// pour affichage sur OLED
extern void afficheLigne(int16_t y, int16_t ey, String text);

// My network credentials
const char* ssid = MY_WIFI_SSID;
const char* password = MY_WIFI_PASSWORD;

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// time to manage http requests
unsigned long currentTime = millis();// Current time
unsigned long previousTime = 0; // Previous time
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

// variables pour stocker la demande d'action

Mvt demMvt = MVT_NONE;
int demDist = 0;
int demAngle = 0;
int demRayon = 0;
Sens demSens = SENS_DROIT;
String mvtEnClair []={"Aucun","Tout droit","Rotation","Virage"};
/** Connexion au réseau local WIFI
 * with SSID and password
 */
void connectWIFI(){
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address 
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  // start web server
  server.begin();
}

void send404(WiFiClient * client){
  client->println("HTTP/1.1 404 NOT FOUND");
  client->println("Connection: close");
  // fin d'entête
  client->println();
}

void sendHeaderHTML(WiFiClient * client){
  client->println("HTTP/1.1 200 OK");
  client->println("Content-type:text/html");
  client->println("Connection: close");
  client->println();
}

void sendHeaderText(WiFiClient * client){
  client->println("HTTP/1.1 200 OK");
  client->println("Content-type:text/plain;charset=utf-8");
  client->println("Connection: close");
  client->println();
}

// Display the HTML web page
void sendMainPage (WiFiClient * client){
  client->println("<!DOCTYPE html><html>");
  client->println("<!DOCTYPE html><html><head><meta charset=\"utf-8\">");
  client->println("<title>CDE Robot V1.0</title>");
  client->println("<style>body,.norm {font-family: Arial,Helvetica,sans-serif;font-size: 3vw;}");
  client->println(".myb{font-size: 4vw;width:20vw;}.mybl{font-size: 3vw;width:30vw;}.myzn{font-size: 3vw;width:10vw;}.seph{display:inline-block;width:5vw;}");
  client->println(".sepv {min-height:3vw;}.myheader {font-size: 8vw;}.inl {display:inline-block;}</style>");
  client->println("</head><body><header class\"myheader\">Télé PAMI V1.0</header>");
  // zone commande de mouvements
  client->println("<div class=\"sepv\"></div>");
  client->println("<div><button class=\"myb\" id=\"tdB\">Tout droit</button><div class=\"seph\"></div><label for=\"dist\">distance en mm</label>");
  client->println("<input type=\"number\" id=\"dist\" class=\"myzn\" min=\"10\" max=\"4000\" value=\"100\"></div><div class=\"sepv\"></div>");
  client->println("<div><button class=\"myb\" id=\"roB\">Rotation</button><div class=\"seph\"></div><button class=\"myb\" id=\"viB\">Virage</button>");
  client->println("<div class=\"inl\"><input type=\"radio\" id=\"idDroit\" name=\"sens\" value=\"droit\" checked><label for=\"idDroit\">à droite</label>");
  client->println("<input type=\"radio\" id=\"idGauche\" name=\"sens\" value=\"gauche\"><label for=\"idGauche\">à gauche</label></div>");
  client->println("<div class=\"sepv\"></div><label for=\"angle\">angle</label><input type=\"number\" id=\"angle\" class=\"myzn\" min=\"0\" max=\"360\" step=\"15\" value=\"90\">");
  client->println("<label for=\"rayon\">rayon de courbure</label><input type=\"number\" id=\"rayon\" class=\"myzn\" min=\"0\" max=\"500\" step=\"10\" value=\"100\"></div>");
  // bouton et zone odométrie
  client->println("<div class=\"sepv\"></div><div><button class=\"mybl\" id=\"setOd\">Reset position</button><div class=\"seph\"></div><button class=\"mybl\" id=\"getOd\">Get position</button></div>");
  client->println("<div class=\"sepv\"></div><div><textarea id=\"idOd\" class=\"norm\" rows=\"3\" cols=\"42\" readonly>Zone position</textarea></div>");
  // boutons et zone info
  client->println("<div class=\"sepv\"></div><div><button class=\"myb\" id=\"infoA\">Info A</button><div class=\"seph\"></div><button class=\"myb\" id=\"infoB\">Info B</button>");
  client->println("<div class=\"seph\"></div><button class=\"myb\" id=\"infoC\">Info C</button></div>");
  client->println("<div class=\"sepv\"></div><div><textarea id=\"idOut\" class=\"norm\" rows=\"7\" cols=\"42\" readonly>Zone retour</textarea></div>");
  // fin du corps
  client->println("</body>");
  // script
  client->println("<script>");
  client->println("document.addEventListener('DOMContentLoaded',function(e){");
  client->println("var baseHost=document.location.origin;");
  client->println("const tdB=document.getElementById('tdB');");
  client->println("const roB=document.getElementById('roB');");
  client->println("const viR=document.getElementById('viB');");
  client->println("const dist=document.getElementById('dist');");
  client->println("const angle=document.getElementById('angle');");
  client->println("const rayon=document.getElementById('rayon');");
  client->println("const boutonsRadio=document.querySelectorAll('input[name=\"sens\"]');");
  // position
  client->println("const setOd=document.getElementById('setOd');");
  client->println("const getOd=document.getElementById('getOd');");
  client->println("const outOd = document.getElementById('idOd');");
  // infos
  client->println("const infoA=document.getElementById('infoA');");
  client->println("const infoB=document.getElementById('infoB');");
  client->println("const infoC=document.getElementById('infoC');");
  client->println("const outZ = document.getElementById('idOut');");
  // envoi d'une demande fetch avec retour d'info dans la zone info
  client->println("function myfetch(query){");
  client->println("fetch(query).then(response=>{console.log(`request to ${query} finished, status: ${response.status}`);");
  client->println("if (!response.ok){outZ.value = `request to ${query} finished, status: ${response.status}`;} ");
  client->println("else {response.text().then(re=>{outZ.value=re;});}});}");
  // une info en retour dans la zone info
  client->println("function getSens(){var sens;boutonsRadio.forEach(b=>{if(b.checked){sens=b.value;}});return sens;}");
  client->println("function sendCde(order){var sens=getSens();");
  client->println("const query=`${baseHost}/cde?type=${order}&dist=${dist.value}&angle=${angle.value}&rayon=${rayon.value}&sens=${sens}`;");
  client->println("myfetch(query);}");
  client->println("tdB.onclick=()=>{sendCde('toutDroit');};");
  client->println("roB.onclick=()=>{sendCde('rotation');};");
  client->println("viB.onclick=()=>{sendCde('virage');};");
  // envoi d'une commande de position (set ou get)
  client->println("function sendDemPos(what){const query=`${baseHost}/pos${what}`;");
  client->println("fetch(query).then(response=>{console.log(`request to ${query} finished, status: ${response.status}`);");
  client->println("if (!response.ok){outZ.value = `request to ${query} finished, status: ${response.status}`;}");
  client->println("else {response.text().then(re=>{outOd.value=re;});}});}");
  client->println("setOd.onclick=()=>{sendDemPos('Set');};"); // reset d'odométrie => /posSet
  client->println("getOd.onclick=()=>{sendDemPos('Get');};"); // obtenir position => /posGet
  // envoi d'une demande d'information
  client->println("function sendDemInfo(groupe){const query=`${baseHost}/info?groupe=${groupe}`;");
  client->println("myfetch(query);}");
  client->println("infoA.onclick=()=>{sendDemInfo('A');};");
  client->println("infoB.onclick=()=>{sendDemInfo('B');};");
  client->println("infoC.onclick=()=>{sendDemInfo('C');};");
  // fin du script
  client->println("});</script>");
  client->println("  </html>");
  // The HTTP response ends with another blank line
  client->println();
}

void traceDemande(){
  Serial.println("Décodage de la demande :");
  Serial.print("type de monvement : ");Serial.println(demMvt);
  Serial.print("distance : ");Serial.println(demDist);
  Serial.print("angle : ");Serial.println(demAngle);
  Serial.print("rayon : ");Serial.println(demRayon);
  Serial.print("sens : ");Serial.println(demSens);
}

void  execAndSendRespCde(WiFiClient * client){
  if (mvtEnCours != nullptr){
    send404(client);
    return;
  }
  // instanciation d'un mouvement 
  switch (demMvt){
    case MVT_TOUT_DROIT :
      mvtEnCours = new MvtToutDroit(demDist);
      break;
    case MVT_ROTATION :
      mvtEnCours = new Rotation((demSens==SENS_DROIT?-demAngle:demAngle));
      break;
    case MVT_VIRAGE :
      mvtEnCours = new Virage(demRayon,(demSens==SENS_DROIT?-demAngle:demAngle));
      break;
    default : 
      send404(client);
      return;
  }
  // inititialisation du mouvement et des détection
  afficheLigne(0,16,mvtEnClair[demMvt]);
  mvtEnCours->go();
  initDetection(0,true,100,true); // tof à droite
  initDetection(1,true,100,true); // tof à gauche
  // réponse HTTP
  sendHeaderText(client);
  client->print("Commande en cours d'exécution ");
  client->println(mvtEnClair[demMvt]);
  client->println(); // fin du corps HTTP
}

// décodage des champs de commande (mvt, distance, angle, rayon, sens)
void execCde(WiFiClient * client){
  Serial.println("execCde");
  bool bonneDemande = true;
  // récupère les paramètres de la commande
  // une commande est dans la première ligne de la String header
  // GET /cde?type=toutDroit&dist=100&angle=90&rayon=100&sens=droit HTTP/1.1
  int posType = header.indexOf("type=");
  int posDist = header.indexOf("dist=");
  int posAngle = header.indexOf("angle=");
  int posRayon = header.indexOf("rayon=");
  int posSens = header.indexOf("sens=");
  int posHttp = header.indexOf("HTTP");
  if (posType<0 || posDist<0 || posAngle<0 || posRayon<0 || posSens<0 || posHttp<0){
    Serial.println("bad params");
    send404(client);
    return;
  }
  String mvt = header.substring(posType+5,posDist-1);
  if (mvt=="toutDroit") demMvt = MVT_TOUT_DROIT;
  else if (mvt=="rotation") demMvt = MVT_ROTATION;
  else if (mvt=="virage") demMvt = MVT_VIRAGE;
  else {
    demMvt = MVT_NONE;
    bonneDemande = false;
  }
  String dist = header.substring(posDist+5,posAngle-1);
  demDist= dist.toInt();
  String angle = header.substring(posAngle+6,posRayon-1);
  demAngle = angle.toInt();
  String rayon = header.substring(posRayon+6,posSens-1);
  demRayon = rayon.toInt();
  String sens = header.substring(posSens+5,posHttp-1);
  if (sens=="droit") demSens = SENS_DROIT;
  else if (sens=="gauche") demSens = SENS_GAUCHE;
  else {
    demSens = SENS_BAD;
    bonneDemande=false;
  }
  traceDemande();
  if (!bonneDemande){
    send404(client);
    return;
  }
  // pour l'instant
  // TODO  utiliser bonneDemande
  execAndSendRespCde(client);
}

// envoie d'informations (3 groupes ! A, B ou C)
void execInfo(WiFiClient * client){
  sendHeaderText(client);
  // extraire groupe et réponse appropriée
  String r = "Bad group";
  int pos = header.indexOf("GET /info?groupe=")+17;
  char groupe = header[pos];
  Serial.print("info groupe"); Serial.println(groupe);
  switch (groupe) {
    case 'A' : r = getInfoA(); break;
    case 'B' : r = getInfoB(); break;
    case 'C' : r = getInfoC(); break;
  }
  client->println(r.c_str());
  client->println(""); // fin de corps
}

// send position
void sendPosition(WiFiClient * client){
  client->println(positionCourante.toStringShort().c_str());
  client->println(""); // fin de corps
}

// set origine Odométrie
void execSetOrigin(WiFiClient * client){
  sendHeaderText(client);
  // reset de l'origine
  positionCourante.setPosition(0,0,0);
  sendPosition(client);
}

// get odométrie
void execGetPosition (WiFiClient * client){
  sendHeaderText(client);
  sendPosition(client);
}
/* analyse la requête reçue et fait le traitement approprié + réponse HTTP
 * la requête est dans la variable globale header
 * doit faire une réponse complète (entête et corps)
 * ne doit pas clôre la connexion
 */
void analyseRequete(WiFiClient * client){
  // si télécommande
  if (header.indexOf("GET / HTTP") >= 0){
    // demande page principale
    sendHeaderHTML(client);
    sendMainPage(client);
  }else if (header.indexOf("GET /cde") >= 0) {
    // une commande
    execCde(client);
  }else if (header.indexOf("GET /info") >= 0) {
    execInfo(client);
  }else if (header.indexOf("GET /posSet") >= 0) {
    execSetOrigin(client);
  }else if (header.indexOf("GET /posGet") >= 0) {
    execGetPosition(client);
  }else{
    // requête non conforme (ex demande de favicon)
    send404(client);
  }
}

void execIncomingRequest(){
  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    // cette boucle d'acquisition de la request est bloquante 
    // si la requete est reçu au complet, la boucle inclut la réponse
    while (client.connected() && currentTime - previousTime <= timeoutTime) {  // loop while the client's connected
      currentTime = millis();
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            analyseRequete(&client);
            
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    } // time out ou déconnexion ou 
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
        Serial.println("");
  }
}
