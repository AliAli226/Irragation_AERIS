#include <WiFi.h>        // Librairie pour gérer le Wi-Fi sur ESP32
#include <WebServer.h>   // Librairie pour créer un serveur web simple

// --- Définition des broches ---
#define SOIL_MOISTURE_PIN 36   // Entrée analogique simulant le capteur d'humidité (via un potentiomètre)
#define LED_PIN 4              // Sortie numérique simulant la pompe/relais (via une LED)

// --- Paramètres de contrôle ---
int seuilCritique = 35;        // Seuil d'humidité (%) en dessous duquel la pompe doit s'activer
int dureePompe = 30;           // Durée maximale d'activation de la pompe (en secondes)

// --- Configuration du point d'accès Wi-Fi ---
const char* ssid = "ESP32_Ali";       // Nom du réseau Wi-Fi créé par l'ESP32
const char* password = "12345678";    // Mot de passe du réseau

// --- Création du serveur web sur le port 80 ---
WebServer server(80);

// --- Variables de gestion du mode ---
bool modeManuel = false;   // Si vrai, l'utilisateur contrôle la LED via la page web
bool ledState = false;     // État actuel de la LED (ON/OFF)

// --- Page principale du serveur web ---
void handleRoot() {
  // Construction d'une page HTML avec boutons ON/OFF/AUTO
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Contrôle LED</title></head><body>";
  html += "<h2>Contrôle LED sur ESP32</h2>";
  html += "<p>Mode manuel : " + String(modeManuel ? "ACTIF" : "INACTIF") + "</p>";
  html += "<p>LED actuelle : " + String(ledState ? "ON" : "OFF") + "</p>";
  html += "<p><a href='/on'><button style='font-size:20px;'>ON</button></a></p>";
  html += "<p><a href='/off'><button style='font-size:20px;'>OFF</button></a></p>";
  html += "<p><a href='/auto'><button style='font-size:20px;'>Retour en mode AUTO</button></a></p>";
  html += "</body></html>";
  server.send(200, "text/html", html); // Envoi de la page au client
}

// --- Gestion du bouton ON ---
void handleOn() {
  modeManuel = true;              // Passage en mode manuel
  ledState = true;                // LED activée
  digitalWrite(LED_PIN, HIGH);    // Allume la LED
  server.send(200, "text/html", "<h2>LED activée manuellement ✅</h2><a href='/'>Retour</a>");
}

// --- Gestion du bouton OFF ---
void handleOff() {
  modeManuel = true;              // Passage en mode manuel
  ledState = false;               // LED désactivée
  digitalWrite(LED_PIN, LOW);     // Éteint la LED
  server.send(200, "text/html", "<h2>LED éteinte manuellement ❌</h2><a href='/'>Retour</a>");
}

// --- Gestion du bouton AUTO ---
void handleAuto() {
  modeManuel = false;             // Retour au mode automatique
  server.send(200, "text/html", "<h2>Retour en mode automatique 🔄</h2><a href='/'>Retour</a>");
}

// --- Initialisation ---
void setup() {
  Serial.begin(115200);           // Démarrage du moniteur série
  pinMode(LED_PIN, OUTPUT);       // Configuration de la LED comme sortie

  // Création du point d'accès Wi-Fi
  WiFi.softAP(ssid, password);
  Serial.println("Point d'accès démarré !");
  Serial.print("SSID : "); Serial.println(ssid);
  Serial.print("IP : "); Serial.println(WiFi.softAPIP());

  // Définition des routes du serveur web
  server.on("/", handleRoot);
  server.on("/on", handleOn);
  server.on("/off", handleOff);
  server.on("/auto", handleAuto);

  server.begin();                 // Lancement du serveur web
  Serial.println("Serveur web lancé !");
}

// --- Boucle principale ---
void loop() {
  server.handleClient(); // Vérifie si un client web envoie une requête

  // Mode automatique (si pas en manuel)
  if (!modeManuel) {
    // Lecture brute du potentiomètre (0 à 4095)
    int soilValue = analogRead(SOIL_MOISTURE_PIN);

    // Conversion en pourcentage (0% = sec, 100% = saturé)
    int soilMoisture = map(soilValue, 0, 4095, 0, 100);

    // Affichage dans le moniteur série
    Serial.print("Humidité du sol : ");
    Serial.print(soilMoisture);
    Serial.println(" %");

    // Si humidité < seuil critique → activer pompe
    if (soilMoisture < seuilCritique) {
      digitalWrite(LED_PIN, HIGH);   // Pompe ON
      Serial.println("Sol sec ! Pompe activée...");

      // Décompte avec surveillance continue
      for (int i = dureePompe; i > 0; i--) {
        int soilValueNow = analogRead(SOIL_MOISTURE_PIN);
        int soilMoistureNow = map(soilValueNow, 0, 4095, 0, 100);

        Serial.print("Pompe active - arrêt dans ");
        Serial.print(i);
        Serial.println(" seconde(s)");

        // Si humidité remonte au-dessus du seuil → arrêt immédiat
        if (soilMoistureNow >= seuilCritique) {
          Serial.println("Humidité remontée au-dessus du seuil, arrêt immédiat !");
          break;
        }

        delay(1000); // Attente d'une seconde
      }

      digitalWrite(LED_PIN, LOW);    // Pompe OFF
      Serial.println("Pompe arrêtée.");
    } else {
      digitalWrite(LED_PIN, LOW);    // Pompe OFF
      Serial.println("Sol suffisamment humide, pas d'arrosage.");
    }
  }
}
