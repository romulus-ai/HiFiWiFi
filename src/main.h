void wifiSetup();
void setup();
void loop();
void callback(char* topic, byte* payload, unsigned int length);
void reconnect();
void handleHTTP();
void requestHandler(String strTopic, String strRequest);
String uint64ToString(uint64_t input);
void haMQTTDiscovery();
