#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#define WIFI_SSID "levakuz"
#define WIFI_PASSWORD "qazwsxtgb"
#define MQTT_HOST IPAddress(192,168,1,105)
#define MQTT_PORT 1883
#include <AsyncMqttClient.h>
#include <OneWire.h>
#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN         22         // Configurable, see typical pin layout above
#define SS_PIN          21         // Configurable, see typical pin layout above

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance
AsyncMqttClient mqttClient;
TimerHandle_t mqttReconnectTimer;
TimerHandle_t wifiReconnectTimer;
// Создаем экземпляр класса «AsyncWebServer»
// под названием «server» и задаем ему номер порта «80»:
AsyncWebServer server(80);
int a;

String processor(const String& var){
  
  String ledState;
 
  Serial.println(var);
  if(var == "STATE"){
    if(a==1){
      ledState = "Connected";
    }
    else{
      ledState = "Disconnected";
    }
    Serial.print(ledState);
    return ledState;
  }
  return String();
}

void setup() {
Serial.begin(115200);
SPI.begin();      // Init SPI bus
mfrc522.PCD_Init();   // Init MFRC522
delay(4);        // Optional delay. Some board do need more time after init to be ready, see Readme
  mfrc522.PCD_DumpVersionToSerial();  // Show details of PCD - MFRC522 Card Reader details
WiFi.onEvent(WiFiEvent); //задает то. что при подключении к wi-fi будет запущена функция обратного вызова WiFiEvent(), которая напечатает данные о WiFi подключении
connectToWifi();
// Инициализируем SPIFFS:
if(!SPIFFS.begin(true)){
  Serial.println("An Error has occurred while mounting SPIFFS");
              //  "При монтировании SPIFFS произошла ошибка"
return;
  }
  


// URL для корневой страницы веб-сервера:
Serial.println(WiFi.localIP());
 server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", String(), false);
  });

  
// URL для файла «style.css»:
server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
request->send(SPIFFS, "/style.css", "text/css");
  });

    
server.on("/Connecttomqtt", HTTP_GET, [](AsyncWebServerRequest *request){
a = 1;
mqttClient.setServer(MQTT_HOST, MQTT_PORT);
connectToMqtt();  
mqttClient.onConnect(onMqttConnect);
request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  
  server.on("/Disconnectmqtt", HTTP_GET, [](AsyncWebServerRequest *request){
a = 0;
mqttClient.disconnect(); 
Serial.println("Disconnected from mqtt")  ;
request->send(SPIFFS, "/index.html", String(), false,processor);
  });

  
  server.on("/Subscribe", HTTP_GET, [](AsyncWebServerRequest *request){
mqttClient.onSubscribe(onMqttSubscribe);   
request->send(SPIFFS, "/index.html", String(), false);
  });

  
server.on("/Unsubscribe", HTTP_GET, [](AsyncWebServerRequest *request){
mqttClient.onUnsubscribe(onMqttUnsubscribe);  
request->send(SPIFFS, "/index.html", String(), false);
  });

  
mqttClient.onMessage(onMqttMessage);
server.begin(); 

 

}
void connectToWifi() {
Serial.println("Connecting to Wi-Fi...");
           //  "Подключаемся к WiFi..."
Serial.println(WIFI_SSID);
WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void WiFiEvent(WiFiEvent_t event) {
  Serial.printf("[WiFi-event] event: %d\n", event);
  switch(event) {
    case SYSTEM_EVENT_STA_GOT_IP:
      Serial.println("WiFi connected");  //  "Подключились к WiFi"
      Serial.println("IP address: ");  //  "IP-адрес: "
      Serial.println(WiFi.localIP());
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("WiFi lost connection");
                 //  "WiFi-связь потеряна"
      // делаем так, чтобы ESP32
      // не переподключалась к MQTT
      // во время переподключения к WiFi:
      break;
  }
}

void onMqttConnect(bool sessionPresent) {
    Serial.println("Connected to MQTT.");                           //  "Подключились к MQTT."
    Serial.print("Session present: ");                              //  "Текущая сессия: "
    Serial.println(sessionPresent);
    // ESP32 подписывается на топик esp32/test
    uint16_t packetIdSub = mqttClient.subscribe("test/", 0);
    Serial.print("Subscribing at QoS 0, packetId: ");               //  "Подписка при QoS 0, ID пакета: "
    Serial.println(packetIdSub);
}


void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) { //отключение от MQTT
  Serial.println("Disconnected from MQTT.");
             //  "Отключились от MQTT."

}


void onMqttSubscribe(uint16_t packetId, uint8_t qos) { //подписка на топик
  Serial.println("Subscribe acknowledged.");
             //  "Подписка подтверждена."
  Serial.print("  packetId: ");
  Serial.println(packetId);
  Serial.print("  qos: ");
  Serial.println(qos);
}


void onMqttUnsubscribe(uint16_t packetId) { //отписка от топика
  Serial.println("Unsubscribe acknowledged.");
            //  "Отписка подтверждена."
  Serial.print("  packetId: ");
  Serial.println(packetId);
}


void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  String messageTemp;
  for (int i = 0; i < len; i++) {
    //Serial.print((char)payload[i]);
    messageTemp += (char)payload[i];
  }
  Serial.println("Publish received.");
             //  "Опубликованные данные получены."
  Serial.print("  message: ");  //  "  сообщение: "
  Serial.println(messageTemp);
  Serial.print("  topic: ");  //  "  топик: "
  Serial.println(topic);
  Serial.print("  qos: ");  //  "  уровень обслуживания: "
  Serial.println(properties.qos);
  Serial.print("  dup: ");  //  "  дублирование сообщения: "
  Serial.println(properties.dup);
  Serial.print("  retain: ");  //  "сохраненные сообщения: "
  Serial.println(properties.retain);
  Serial.print("  len: ");  //  "  размер: "
  Serial.println(len);
  Serial.print("  index: ");  //  "  индекс: "
  Serial.println(index);
  Serial.print("  total: ");  //  "  суммарно: "
  Serial.println(total);
}

void connectToMqtt() {
Serial.println("Connecting to MQTT...");
           //  "Подключаемся к MQTT..."
mqttClient.connect();

}


void loop() {
if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return;
  }   
unsigned long first_byte;
unsigned long second_byte;
unsigned long third_byte;
unsigned long fourth_byte;
char mystr1[4];
char mystr2[4];
char mystr3[4];
char mystr4[4];
char id[8];
first_byte = mfrc522.uid.uidByte[0];
second_byte = mfrc522.uid.uidByte[1];
third_byte = mfrc522.uid.uidByte[2];
fourth_byte = mfrc522.uid.uidByte[3];
itoa(first_byte,mystr1,16);
itoa(second_byte,mystr2,16);
itoa(third_byte,mystr3,16);
itoa(fourth_byte,mystr4,16);
strcat(id,mystr1);
strcat(id,mystr2);
strcat(id,mystr3);
strcat(id,mystr4);
Serial.println(mfrc522.uid.uidByte[0],HEX);
Serial.println(mfrc522.uid.uidByte[1],HEX);
Serial.println(mfrc522.uid.uidByte[2],HEX);
Serial.println(mfrc522.uid.uidByte[3],HEX);
mfrc522.PICC_HaltA();
mqttClient.publish("test/", 0 , true ,id);
first_byte = 0;
second_byte = 0;
third_byte = 0;
fourth_byte = 0;
strcpy(mystr1,"");
strcpy(mystr2,"");
strcpy(mystr3,"");      
strcpy(mystr4,"");
strcpy(id,"");
}
