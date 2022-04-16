#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <RCSwitch.h>
#include <ArduinoJson.h>

//WIFI-Verbindungsdaten
#define WIFI_SSID   "XXX"
#define WIFI_PASS   "XXX"
#define mqtt_server "XXX"
#define mqtt_user   "XXX"
#define mqtt_pass   "XXX"

#define STATUS_LED 2 //internal LED give some feedback
#define IR_PIN 5 // GPIO PIN 5 (D1) is for sending IR signals
#define RC_PIN 12    // RC Switch at GPIO 12 which is D6

//IR-Signale koennen nach dem Einbinden der Bibliotheken ueber passende Beispielprogramme 
//(Datei -> Beispiele -> IRremoteESP8266 -> IRrecvDumpV2) eingelesen/decodiert werden.
//IR-Codes (in Hex) des Ventilators:
#define IR_MAJ_VOLMINUS 0x807F10EF // Protocol NEC
#define IR_MAJ_VOLPLUS  0x807FC03F // Protocol NEC
#define IR_MAJ_PAIR     0x807F906F // Protocol NEC
#define IR_MAJ_MUTE     0x807FCC33 // Protocol NEC
#define IR_MAJ_POWER    0x807F807F // Protocol NEC

#define IR_MAJ_VOLMINUS_STR "volminus"
#define IR_MAJ_VOLPLUS_STR  "volplus"
#define IR_MAJ_PAIR_STR     "pair"
#define IR_MAJ_MUTE_STR     "mute"
#define IR_MAJ_POWER_STR    "power"


#define IR_BEAM_POWER   0x10C8E11E // Protocol NEC
#define IR_BEAM_SOURCE  0x10C831CE // Protocol NEC
#define IR_BEAM_MODE    0x10C801FE // Protocol NEC
#define IR_BEAM_ENTER   0x10C843BC // Protocol NEC
#define IR_BEAM_UP      0x10C841BE // Protocol EPSON (Repeat)
#define IR_BEAM_RIGHT   0x10C8817E // Protocol NEC
#define IR_BEAM_DOWN    0x10C8A15E // Protocol EPSON (Repeat)
#define IR_BEAM_LEFT    0x10C8C13E // Protocol NEC
#define IR_BEAM_BACK    0x10C8837C // Protocol NEC
#define IR_BEAM_MENU    0x10C821DE // Protocol NEC

#define IR_BEAM_POWER_STR  "power"
#define IR_BEAM_SOURCE_STR "source"
#define IR_BEAM_MODE_STR   "mode"
#define IR_BEAM_ENTER_STR  "enter"
#define IR_BEAM_UP_STR     "up"
#define IR_BEAM_RIGHT_STR  "right"
#define IR_BEAM_DOWN_STR   "down"
#define IR_BEAM_LEFT_STR   "left"
#define IR_BEAM_BACK_STR   "back"
#define IR_BEAM_MENU_STR   "menu"

#define IR_BITS 32 //Anzahl Bits
#define IR_WDH 3  //Anzahl Wiederholungen

//Delay in ms zwischen dem Senden von mehrfachen IR-Befehlen (bei ...plus X / ...minus X).
#define IR_DELAY_MS 100

// RC Commands
#define RC_ESMART_UP   7994226
#define RC_ESMART_STOP 7994232
#define RC_ESMART_DOWN 7994228

#define RC_ESMART_UP_STR    "up"
#define RC_ESMART_STOP_STR  "stop"
#define RC_ESMART_DOWN_STR  "down"

//RC Switch Bitlength
#define RC_BITS 24

#define RC_PULSELENGTH 184 // RC Pulselength
#define RC_REPS 3 // RC Repeats

#define CLIENT_ID      "hifi_wifi"
#define MAJORITY_TOPIC "ir_majority"
#define BEAMER_TOPIC   "ir_beamer"
#define ESMART_TOPIC   "rc_esmart" 

WiFiClient espClient;
PubSubClient client(espClient);

// Set web server port number to 80
WiFiServer server(80);

IRsend irsend(IR_PIN);

// RC Switch to controll things via 433MHZ
RCSwitch mySwitch = RCSwitch();

#define SIZE_MAJ_REQ_ARR 5
#define SIZE_BEAM_REQ_ARR 10
#define SIZE_ESMART_REQ_ARR 3

String maj_requests[SIZE_MAJ_REQ_ARR] = { String(IR_MAJ_VOLMINUS_STR), String(IR_MAJ_VOLPLUS_STR), String( IR_MAJ_PAIR_STR), String( IR_MAJ_MUTE_STR), String( IR_MAJ_POWER_STR) };
String beamer_requests[SIZE_BEAM_REQ_ARR] = { String(IR_BEAM_POWER_STR), String( IR_BEAM_SOURCE_STR), String( IR_BEAM_MODE_STR), String( IR_BEAM_ENTER_STR), String( IR_BEAM_UP_STR), String( IR_BEAM_RIGHT_STR), String( IR_BEAM_DOWN_STR), String( IR_BEAM_LEFT_STR), String( IR_BEAM_BACK_STR), String( IR_BEAM_MENU_STR) };
String esmart_requests[SIZE_ESMART_REQ_ARR] = { String(RC_ESMART_UP_STR), String( RC_ESMART_STOP_STR), String( RC_ESMART_DOWN_STR) };

void wifiSetup() {
  //Setze WIFI-Module auf STA mode
  WiFi.mode(WIFI_STA);
 
  //Verbinden
  Serial.println();
  Serial.println("[WIFI] Verbinden mit " + String(WIFI_SSID) );
  WiFi.begin(WIFI_SSID, WIFI_PASS);
 
  //Warten bis Verbindung hergestellt wurde
  while (WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
      delay(500);
  }
  Serial.println();
  
  // Connected, blick 3 times and print ifo to serial
  digitalWrite(STATUS_LED, HIGH); // Turn the STATUS_LED off by making the voltage HIGH
  delay(200); // Wait for a second
  digitalWrite(STATUS_LED, LOW); // Turn the STATUS_LED on (Note that LOW is the voltage level)
  delay(200); // Wait for a second
  digitalWrite(STATUS_LED, HIGH); // Turn the STATUS_LED off by making the voltage HIGH
  delay(200); // Wait for a second
  digitalWrite(STATUS_LED, LOW); // Turn the STATUS_LED on (Note that LOW is the voltage level)
  delay(200); // Wait for a second
  digitalWrite(STATUS_LED, HIGH); // Turn the STATUS_LED off by making the voltage HIGH
  delay(200); // Wait for a second
  digitalWrite(STATUS_LED, LOW); // Turn the STATUS_LED on (Note that LOW is the voltage level)
  
  Serial.println("[WIFI] STATION Mode connected, SSID: " + WiFi.SSID() + ", IP-Adresse: " + WiFi.localIP().toString());

}

void setup() {
  delay(1500);
 
  Serial.begin(115200);
  pinMode(STATUS_LED, OUTPUT); // Initialize the LED pin as an output
  digitalWrite(STATUS_LED, LOW); // Let the LED shine

  //WIFI
  wifiSetup();

  // MQTT
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  Serial.printf("[MQTT] Configured, Server: %s, Port: 1883\n", mqtt_server);

  // IR LED
  irsend.begin();
  Serial.println("[IR] Initialized");

  
  mySwitch.enableTransmit(RC_PIN);
  mySwitch.setPulseLength(RC_PULSELENGTH);
  mySwitch.setRepeatTransmit(RC_REPS);
  Serial.println("[RC] Initialized and configured");

  // start webserver
  server.begin();
  Serial.println("[WEBSRV] Started");

  // connect to mqtt and publish discovery
  reconnect();
  haMQTTDiscovery();
}

// the loop function runs over and over again forever
void loop() {
  yield();
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  yield();

  handleHTTP();
}

// This is executed if a package is received
void callback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';
  String strTopic = String(topic);
  String strPayload = String((char * ) payload);

  Serial.printf("[MQTT] On Topic %s I fetched payload %s\n", strTopic.c_str(), strPayload.c_str());

  if (strPayload != "handled" && strPayload != "") {
    requestHandler(strTopic, strPayload);
    String reply = "handled";
    client.publish(strTopic.c_str(), "" , true);
    client.publish(strTopic.c_str(), reply.c_str() , false);
  }
}

// reconnecting to MQTT if connection is lost
void reconnect() {
  while (!client.connected()){
    Serial.printf("[MQTT] connecting...\n");
    if (client.connect(CLIENT_ID,mqtt_user,mqtt_pass)) {
     Serial.printf("[MQTT] Connected, ClientID: %s, User: %s, Password: %s\n", CLIENT_ID, mqtt_user, mqtt_pass);
     client.subscribe(MAJORITY_TOPIC);
     Serial.printf("[MQTT] subscribed topic %s\n", MAJORITY_TOPIC);
     client.subscribe(BEAMER_TOPIC);
     Serial.printf("[MQTT] subscribed topic %s\n", BEAMER_TOPIC);
     client.subscribe(ESMART_TOPIC);
     Serial.printf("[MQTT] subscribed topic %s\n", ESMART_TOPIC);
    } else {
      Serial.printf("[MQTT] connecting failed, retry after a few seconds...\n");
      delay(6000);
    }
  }
}

void handleHTTP() {
  static int timeout_busy = 0;
  WiFiClient client = server.available(); // Listen for incoming clients


  if (!client) {
    return;
  }
  Serial.println("[WEBSRV] New Client connected");
  
  while ( (!client.available()) && (timeout_busy++ < 5000) ){
    delay(1);
  }
  
  if (timeout_busy >= 5000) {
    Serial.println("[WEBSRV] timeout, disconnecting client");
    client.flush();
    client.stop();
    Serial.println("[WEBSRV] Client disconnected.");
    timeout_busy = 0;
    return;
  }

  
  Serial.println("[WEBSRV] connection made");
  
  String payload = client.readStringUntil('\r');
  client.flush();
  
  Serial.print("[WEBSRV] Recv http: ");  
  Serial.println(payload);
  delay(100);


  // Request is: GET /?topic=TOPIC&request=REQUEST
  if (payload.indexOf("GET /?topic=") >= 0 ) {
    int begin_topic = payload.indexOf("topic=") + 6;
    int end_topic = payload.indexOf("&");
    int begin_request = payload.indexOf("request=") + 8;
    int end_request = payload.indexOf("HTTP");

    String strTopic = payload.substring(begin_topic, end_topic);
    String strRequest = payload.substring(begin_request, end_request);
    Serial.printf("[WEBSRV] On Topic %s I fetched request %s\n", strTopic.c_str(), strRequest.c_str());

    requestHandler(strTopic, strRequest);
  }

  String strMajorityTopic = "?topic=" + String(MAJORITY_TOPIC) + "&request=";
  String strBeamerTopic = "?topic=" + String(BEAMER_TOPIC) + "&request=";
  String strEsmartTopic = "?topic=" + String(ESMART_TOPIC) + "&request=";

  // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
  // and a content-type so the client knows what's coming, then a blank line:
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println("Connection: close");
  client.println();

  // Display the HTML web page
  client.println("<!DOCTYPE html><html>");
  client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
  client.println("<link rel=\"icon\" href=\"data:,\">");
  // CSS to style the on/off buttons 
  // Feel free to change the background-color and font-size attributes to fit your preferences
  client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
  client.println(".button { background-color: #195B6A; border: none; color: white; padding: 16px 40px;");
  client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
  client.println("table, th, td { border: thin solid; border-collapse: collapse; }");
  client.println(".button2 {background-color: #77878A;}</style></head>");
  
  // Web Page Heading
  client.println("<body><h1>HiFi Wifi Web</h1>");

  client.println("<table>");
  client.println("<tr>");
  client.println("<td><p><a href=\"/?topic=mixed&request=watching\"><button class=\"button\">Watching</button></a></td>");
  client.println("<td><p><a href=\"/?topic=mixed&request=stopwatching\"><button class=\"button\">Stop Watching</button></a></td>");
  client.println("</tr>");
  client.println("</table>");
  
  client.println("<table>");
  client.println("<tr>");
  client.println("<td><h2>Soundbar Controls</h2></td>");
  client.println("<td><h2>Beamer Controls</h2></td>");
  client.println("<td><h2>Screen Controls</h2></td>");
  client.println("</tr>");

  client.println("<tr>");
  client.println("<td>");
  
  client.println("<p><a href=\"/" + strMajorityTopic + IR_MAJ_POWER_STR + "\"><button class=\"button\">Power</button></a></p>");
  client.println("<p><a href=\"/" + strMajorityTopic + IR_MAJ_VOLPLUS_STR + "\"><button class=\"button\">VOL+</button></a></p>");
  client.println("<p><a href=\"/" + strMajorityTopic + IR_MAJ_VOLMINUS_STR + "\"><button class=\"button\">VOL-</button></a></p>");
  client.println("<p><a href=\"/" + strMajorityTopic + IR_MAJ_MUTE_STR + "\"><button class=\"button\">Mute</button></a></p>");
  client.println("<p><a href=\"/" + strMajorityTopic + IR_MAJ_PAIR_STR + "\"><button class=\"button\">Bluetooth Pair</button></a></p>");
  client.println("</td>");

  client.println("<td>");
  client.println("<p><a href=\"/" + strBeamerTopic + IR_BEAM_POWER_STR + "\"><button class=\"button\">Power</button></a></p>");
  client.println("<p><a href=\"/" + strBeamerTopic + IR_BEAM_SOURCE_STR + "\"><button class=\"button\">Source</button></a></p>");
  client.println("<p><a href=\"/" + strBeamerTopic + IR_BEAM_MODE_STR + "\"><button class=\"button\">Mode</button></a></p>");
  client.println("<p><a href=\"/" + strBeamerTopic + IR_BEAM_ENTER_STR + "\"><button class=\"button\">Enter</button></a></p>");
  client.println("<p><a href=\"/" + strBeamerTopic + IR_BEAM_UP_STR + "\"><button class=\"button\">UP</button></a></p>");
  client.println("<p><a href=\"/" + strBeamerTopic + IR_BEAM_RIGHT_STR + "\"><button class=\"button\">RIGHT</button></a></p>");
  client.println("<p><a href=\"/" + strBeamerTopic + IR_BEAM_DOWN_STR + "\"><button class=\"button\">DOWN</button></a></p>");
  client.println("<p><a href=\"/" + strBeamerTopic + IR_BEAM_LEFT_STR + "\"><button class=\"button\">LEFT</button></a></p>");
  client.println("<p><a href=\"/" + strBeamerTopic + IR_BEAM_BACK_STR + "\"><button class=\"button\">Back</button></a></p>");
  client.println("<p><a href=\"/" + strBeamerTopic + IR_BEAM_MENU_STR + "\"><button class=\"button\">Menu</button></a></p>");
  client.println("</td>");

  client.println("<td>");
  client.println("<p><a href=\"/" + strEsmartTopic + RC_ESMART_UP_STR + "\"><button class=\"button\">UP</button></a></p>");
  client.println("<p><a href=\"/" + strEsmartTopic + RC_ESMART_DOWN_STR + "\"><button class=\"button\">DOWN</button></a></p>");
  client.println("<p><a href=\"/" + strEsmartTopic + RC_ESMART_STOP_STR + "\"><button class=\"button\">STOP</button></a></p>");
  client.println("</td>");
  client.println("</tr>");
  
  client.println("</table></body></html>");
  // The HTTP response ends with another blank line
  client.println();

  // Close the connection
  client.flush();
  client.stop();
  Serial.println("[WEBSRV] Client disconnected.");
  Serial.println("");
  yield();

}

void requestHandler(String strTopic, String strRequest) {

  strTopic.trim();
  strRequest.trim();
  Serial.println("[HANDLER] sending signal for " + strTopic + "/" + strRequest);

  digitalWrite(STATUS_LED, HIGH); // Turn the STATUS_LED off by making the voltage HIGH
  delay(200); // Wait for a second
  digitalWrite(STATUS_LED, LOW); // Turn the STATUS_LED on (Note that LOW is the voltage level)
  delay(200); // Wait for a second

  if(strTopic == MAJORITY_TOPIC) {
    Serial.println("[IR HANDLER] topic: " + String(MAJORITY_TOPIC));
    if(strRequest == IR_MAJ_VOLPLUS_STR) {
      Serial.println("[IR HANDLER] sending signal " + String(IR_MAJ_VOLPLUS_STR) + " HEX: " + uint64ToString(IR_MAJ_VOLPLUS));
      irsend.sendNEC(IR_MAJ_VOLPLUS, IR_BITS, IR_WDH); 
    } 
    else if (strRequest == IR_MAJ_VOLMINUS_STR) {
      Serial.println("[IR HANDLER] sending signal " + String(IR_MAJ_VOLMINUS_STR) + " HEX: " + uint64ToString(IR_MAJ_VOLMINUS));
      irsend.sendNEC(IR_MAJ_VOLMINUS, IR_BITS, IR_WDH);
    }
    else if (strRequest == IR_MAJ_PAIR_STR) {
      Serial.println("[IR HANDLER] sending signal " + String(IR_MAJ_PAIR_STR) + " HEX: " + uint64ToString(IR_MAJ_PAIR));
      irsend.sendNEC(IR_MAJ_PAIR, IR_BITS, IR_WDH);
    }
    else if (strRequest == IR_MAJ_MUTE_STR) {
      Serial.println("[IR HANDLER] sending signal " + String(IR_MAJ_MUTE_STR) + " HEX: " + uint64ToString(IR_MAJ_MUTE));
      irsend.sendNEC(IR_MAJ_MUTE, IR_BITS, IR_WDH);
    }
    else if (strRequest == IR_MAJ_POWER_STR) {
      Serial.println("[IR HANDLER] sending signal " + String(IR_MAJ_POWER_STR) + " HEX: " + uint64ToString(IR_MAJ_POWER));
      irsend.sendNEC(IR_MAJ_POWER, IR_BITS, IR_WDH);
    }
  }
  
  else if(strTopic == BEAMER_TOPIC) {
    Serial.println("[IR HANDLER] topic: " + String(BEAMER_TOPIC));
    if(strRequest == IR_BEAM_POWER_STR) {
      Serial.println("[IR HANDLER] sending signal " + String(IR_BEAM_POWER_STR) + " HEX: " + uint64ToString(IR_BEAM_POWER));
      irsend.sendNEC(IR_BEAM_POWER, IR_BITS, IR_WDH); 
    } 
    else if (strRequest == IR_BEAM_SOURCE_STR) {
      Serial.println("[IR HANDLER] sending signal " + String(IR_BEAM_SOURCE_STR) + " HEX: " + uint64ToString(IR_BEAM_SOURCE));
      irsend.sendNEC(IR_BEAM_SOURCE, IR_BITS, IR_WDH);
    }
    else if (strRequest == IR_BEAM_MODE_STR) {
      Serial.println("[IR HANDLER] sending signal " + String(IR_BEAM_MODE_STR) + " HEX: " + uint64ToString(IR_BEAM_MODE));
      irsend.sendNEC(IR_BEAM_MODE, IR_BITS, IR_WDH);
    }
    else if (strRequest == IR_BEAM_ENTER_STR) {
      Serial.println("[IR HANDLER] sending signal " + String(IR_BEAM_ENTER_STR) + " HEX: " + uint64ToString(IR_BEAM_ENTER));
      irsend.sendNEC(IR_BEAM_ENTER, IR_BITS, IR_WDH);
    }
    else if (strRequest == IR_BEAM_UP_STR) {
      Serial.println("[IR HANDLER] sending signal " + String(IR_BEAM_UP_STR) + " HEX: " + uint64ToString(IR_BEAM_UP));
      irsend.sendEpson(IR_BEAM_UP, IR_BITS, IR_WDH);
    }
    else if (strRequest == IR_BEAM_RIGHT_STR) {
      Serial.println("[IR HANDLER] sending signal " + String(IR_BEAM_RIGHT_STR) + " HEX: " + uint64ToString(IR_BEAM_RIGHT));
      irsend.sendNEC(IR_BEAM_RIGHT, IR_BITS, IR_WDH);
    }
    else if (strRequest == IR_BEAM_DOWN_STR) {
      Serial.println("[IR HANDLER] sending signal " + String(IR_BEAM_DOWN_STR) + " HEX: " + uint64ToString(IR_BEAM_DOWN));
      irsend.sendEpson(IR_BEAM_DOWN, IR_BITS, IR_WDH);
    }
    else if (strRequest == IR_BEAM_LEFT_STR) {
      Serial.println("[IR HANDLER] sending signal " + String(IR_BEAM_LEFT_STR) + " HEX: " + uint64ToString(IR_BEAM_LEFT));
      irsend.sendNEC(IR_BEAM_LEFT, IR_BITS, IR_WDH);
    }
    else if (strRequest == IR_BEAM_BACK_STR) {
      Serial.println("[IR HANDLER] sending signal " + String(IR_BEAM_BACK_STR) + " HEX: " + uint64ToString(IR_BEAM_BACK));
      irsend.sendNEC(IR_BEAM_BACK, IR_BITS, IR_WDH);
    }
    else if (strRequest == IR_BEAM_MENU_STR) {
      Serial.println("[IR HANDLER] sending signal " + String(IR_BEAM_MENU_STR) + " HEX: " + uint64ToString(IR_BEAM_MENU));
      irsend.sendNEC(IR_BEAM_MENU, IR_BITS, IR_WDH);
    }
  }

  else if(strTopic == ESMART_TOPIC) {
    Serial.println("[IR HANDLER] topic: " + String(ESMART_TOPIC));
    if (strRequest == RC_ESMART_UP_STR) {
      Serial.println("[RC HANDLER] sending signal " + String(RC_ESMART_UP_STR) + " DEC: " + String(RC_ESMART_UP));
      mySwitch.send(RC_ESMART_UP, RC_BITS);
    }
    else if (strRequest == RC_ESMART_STOP_STR) {
      Serial.println("[RC HANDLER] sending signal " + String(RC_ESMART_STOP_STR) + " DEC: " + String(RC_ESMART_STOP));
      mySwitch.send(RC_ESMART_STOP, RC_BITS);
    }
    else if (strRequest == RC_ESMART_DOWN_STR) {
      Serial.println("[RC HANDLER] sending signal " + String(RC_ESMART_DOWN_STR) + " DEC: " + String(RC_ESMART_DOWN));
      mySwitch.send(RC_ESMART_DOWN, RC_BITS);
    }
  }

  else if (strTopic == "mixed") {
    Serial.println("[MXD HANDLER] topic: mixed");
    if (strRequest == "watching") {
      Serial.println("[MXD HANDLER] triggering watching mode, screen down, beamer power, soundbar power, soundbar volume 3 times down");
      mySwitch.send(RC_ESMART_DOWN, RC_BITS);
      irsend.sendNEC(IR_BEAM_POWER, IR_BITS, IR_WDH);
      delay(1000);
      irsend.sendNEC(IR_MAJ_POWER, IR_BITS, IR_WDH);
      irsend.sendNEC(IR_MAJ_VOLMINUS, IR_BITS, IR_WDH);
      irsend.sendNEC(IR_MAJ_VOLMINUS, IR_BITS, IR_WDH);
      irsend.sendNEC(IR_MAJ_VOLMINUS, IR_BITS, IR_WDH);
    } 
    
    else if (strRequest == "stopwatching") {
      Serial.println("[MXD HANDLER] triggering stop watching mode, screen up, beamer power off, soundbar power off");
      mySwitch.send(RC_ESMART_UP, RC_BITS);
      irsend.sendNEC(IR_BEAM_POWER, IR_BITS, IR_WDH);
      delay(2000);
      irsend.sendNEC(IR_BEAM_POWER, IR_BITS, IR_WDH);
      delay(1000);
      irsend.sendNEC(IR_MAJ_POWER, IR_BITS, IR_WDH);
    }
  }

  digitalWrite(STATUS_LED, HIGH); // Turn the STATUS_LED off by making the voltage HIGH
  delay(200); // Wait for a second
  digitalWrite(STATUS_LED, LOW);
}

String uint64ToString(uint64_t input) {
  String result = "";
  uint8_t base = 10;

  do {
    char c = input % base;
    input /= base;

    if (c < 10)
      c +='0';
    else
      c += 'A' - 10;
    result = c + result;
  } while (input);
  return result;
}

// see https://www.home-assistant.io/docs/mqtt/discovery/
void haMQTTDiscovery() {
  Serial.println("[DISCOVERY] Publishing devices to MQTT");
  for (int i = 0; i < SIZE_MAJ_REQ_ARR; i++) {
    DynamicJsonDocument doc(1024);
    String unique_id = "soundbar_" + maj_requests[i] + "_button";
    String topic = "homeassistant/button/" + unique_id + "/config";
    
    doc["unique_id"] = unique_id;
    doc["name"]   = maj_requests[i] + "soundbar";
    doc["command_topic"] = MAJORITY_TOPIC;
    doc["payload_press"] = maj_requests[i];
    doc["entity_category"] = "config";

    String jsonDoc = "";

    serializeJson(doc, jsonDoc);

    Serial.println("[DISCOVERY] Publishing " + topic + ": " + jsonDoc);
    client.publish(topic.c_str(), jsonDoc.c_str() , true);
  }

  for (int i = 0; i < SIZE_BEAM_REQ_ARR; i++) {
    DynamicJsonDocument doc(1024);
    String unique_id = "beamer_" + beamer_requests[i] + "_button";
    String topic = "homeassistant/button/" + unique_id + "/config";
    
    doc["unique_id"] = unique_id;
    doc["name"]   = beamer_requests[i] + "beamer";
    doc["command_topic"] = BEAMER_TOPIC;
    doc["payload_press"] = beamer_requests[i];
    doc["entity_category"] = "config";

    String jsonDoc = "";

    serializeJson(doc, jsonDoc);

    Serial.println("[DISCOVERY] Publishing " + topic + ": " + jsonDoc);
    client.publish(topic.c_str(), jsonDoc.c_str() , true);
  }

  for (int i = 0; i < SIZE_ESMART_REQ_ARR; i++) {
    DynamicJsonDocument doc(1024);
    String unique_id = "screen_" + esmart_requests[i] + "_button";
    String topic = "homeassistant/button/" + unique_id + "/config";
    
    doc["unique_id"] = unique_id;
    doc["name"]   = esmart_requests[i] + "screen";
    doc["command_topic"] = ESMART_TOPIC;
    doc["payload_press"] = esmart_requests[i];
    doc["entity_category"] = "config";

    String jsonDoc = "";

    serializeJson(doc, jsonDoc);

    Serial.println("[DISCOVERY] Publishing " + topic + ": " + jsonDoc);
    client.publish(topic.c_str(), jsonDoc.c_str() , true);
  }
  Serial.println("[DISCOVERY] Finished publishing devices to MQTT");
}
