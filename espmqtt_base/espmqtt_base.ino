#define MQTT_PORT 1883

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>
#include <PubSubClient.h>

// nom de la machine ayant le broker (mDNS)
const char* mqtt_server = "raspberrypiled.local";

// structure pour la configuration
struct EEconf {
  char ssid[32];
  char password[64];
  char myhostname[32];
} readconf;

// objet pour la connexion
WiFiClient espClient;
// connexion MQTT
PubSubClient client(espClient);
// pour l'intervalle
long lastMsg = 0;
// valeur à envoyer
byte val = 0;

void setup_wifi() {
  // mode station
  WiFi.mode(WIFI_STA);
  Serial.println();
  Serial.print("Connexion ");
  Serial.println(readconf.ssid);
  // connexion wifi
  WiFi.begin(readconf.ssid, readconf.password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // affichage
  Serial.println("");
  Serial.println("Connexion wifi ok");
  Serial.println("Adresse IP: ");
  Serial.println(WiFi.localIP());

  // configuration mDNS
  WiFi.hostname(readconf.myhostname);
  if (!MDNS.begin(readconf.myhostname)) {
    Serial.println("Erreur configuration mDNS!");
  } else {
    Serial.println("répondeur mDNS démarré");
    Serial.println(readconf.myhostname);
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message [");
  Serial.print(topic);
  Serial.print("] ");
  // affichage du payload
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  // le caractère '1' est-il le premier du payload ?
  if ((char)payload[0] == '1') {
    // oui led = on
    digitalWrite(BUILTIN_LED, LOW);
  } else {
    // non led = off
    digitalWrite(BUILTIN_LED, HIGH);
  }
}

void reconnect() {
  // Connecté au broker ?
  while(!client.connected()) {
    // non. On se connecte.
    if(!client.connect(readconf.myhostname)) {
      Serial.print("Erreur connexion MQTT, rc=");
      Serial.println(client.state());
      delay(5000);
      continue;
    }
    Serial.println("Connexion serveur MQTT ok");
    // connecté.
    // on s'abonne au topic "ctrlled"
    client.subscribe("ctrlled");
  }
}

void setup() {
  // configuration led
  pinMode(BUILTIN_LED, OUTPUT);
  // configuration moniteur série
  Serial.begin(115200);
  // configuration EEPROM
  EEPROM.begin(sizeof(readconf));
  // lecture configuration
  EEPROM.get(0, readconf);
  // configuration wifi
  setup_wifi();
  // configuration broker
  client.setServer(mqtt_server, MQTT_PORT);
  // configuration callback
  client.setCallback(callback);
}

void loop() {
  // array pour conversion val
  char msg[16];
  // array pour topic
  char topic[64];

  // Sommes-nous connecté ?
  if (!client.connected()) {
    // non. Connexion
    reconnect();
  }
  // gestion MQTT
  client.loop();

  // temporisation
  long now = millis();
  if (now - lastMsg > 5000) {
    // 5s de passé
    lastMsg = now;
    val++;
    // construction message
    sprintf(msg, "hello world #%hu", val);
    // construction topic
    sprintf(topic, "maison/%s/valeur", readconf.myhostname);
    // publication message sur topic
    client.publish(topic, msg);
  }
}

