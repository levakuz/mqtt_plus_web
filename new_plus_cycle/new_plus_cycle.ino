#include "WiFi.h" //библиотека для использования Wi-Fi на esp
#include "ESPAsyncWebServer.h" //библиотека веб сервера esp
#include "SPIFFS.h" //библиотека файловой системы SPIFFS
//#define WIFI_SSID "levakuz" //имя Wi-Fi сети
//#define WIFI_PASSWORD "qazwsxtgb" //пароль Wi-Fi сети
//#define WIFI_SSID "LAPTOP-AVNJMMV1 9031" //имя Wi-Fi сети
//#define WIFI_PASSWORD "80G:3s51" //пароль Wi-Fi сети
#define WIFI_SSID "SPEECH_405" //имя Wi-Fi сети
#define WIFI_PASSWORD "multimodal" //пароль Wi-Fi сети
#define MQTT_HOST IPAddress(192,168,0,160)//ip адрес mqtt брокера
#define MQTT_PORT 1883 //порт mqtt брокера
#include <AsyncMqttClient.h> //библиотека mqtt клиента
#include <OneWire.h> //библиотека протокола 1-wire
#include <SPI.h> //библиотека SPI интерфейса
#include <MFRC522.h>//библиотека для RFID модуля

#define RST_PIN         22         // указание RST пина, подключенного к esp
#define SS_PIN          21         // указание SS пина, подключенного к esp

#define esp32_id ".01" //Номер платы esp32 (номер стола)

MFRC522 mfrc522(SS_PIN, RST_PIN);  // инициализация RFID модуля с указанием пинов
AsyncMqttClient mqttClient; //инициализация mqtt клиента

// Создаем экземпляр класса «AsyncWebServer»
// под названием «server» и задаем ему номер порта «80»:
AsyncWebServer server(80);
int a;
unsigned long one_byte;
char byte_to_str[20];
char id[20];
int i;
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
SPI.begin();      // инициализация SPI шины, используется для загрузки в память устройства данных о веб интерфейсе
mfrc522.PCD_Init();   // Инициализация RFID модуля
delay(4);        
mfrc522.PCD_DumpVersionToSerial();  // функция считывания данных с RFID меток
WiFi.onEvent(WiFiEvent); //задает то. что при подключении к wi-fi будет запущена функция обратного вызова WiFiEvent(), которая напечатает данные о WiFi подключении
connectToWifi();
// Инициализируем фалойвую систему SPIFFS:
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

// URL для подключения к mqtt брокеру   
server.on("/Connecttomqtt", HTTP_GET, [](AsyncWebServerRequest *request){
a = 1;
mqttClient.setServer(MQTT_HOST, MQTT_PORT);
connectToMqtt();  
mqttClient.onConnect(onMqttConnect);
request->send(SPIFFS, "/index.html", String(), false, processor);
  });

// URL для отключения от mqtt брокера
  server.on("/Disconnectmqtt", HTTP_GET, [](AsyncWebServerRequest *request){
a = 0;
mqttClient.disconnect(); 
Serial.println("Disconnected from mqtt")  ;
request->send(SPIFFS, "/index.html", String(), false,processor);
  });

// URL для подписки на топик (еще не реализовано)     
  server.on("/Subscribe", HTTP_GET, [](AsyncWebServerRequest *request){
mqttClient.onSubscribe(onMqttSubscribe);   
request->send(SPIFFS, "/index.html", String(), false);
  });

// URL для отписки от топика (еще не реализовано)  
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
    uint16_t packetIdSub = mqttClient.subscribe("tables/", 0);
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
  //блок считывания и отправки uid RFID метки

for(i=0;i<4;i++){
    one_byte = mfrc522.uid.uidByte[i];
    itoa(one_byte,byte_to_str,16);
    strcat(id,byte_to_str);
      }
Serial.println(mfrc522.uid.uidByte[0],HEX);
Serial.println(mfrc522.uid.uidByte[1],HEX);
Serial.println(mfrc522.uid.uidByte[2],HEX);
Serial.println(mfrc522.uid.uidByte[3],HEX);
Serial.println(id);
strcat(id,esp32_id);
Serial.println(esp32_id);
mfrc522.PICC_HaltA(); //функция приостановки считывания меток
mqttClient.publish("tables/", 0 , true ,id); //отправка переменной, содержащей uid в топик test 
//сброс переменных
one_byte = 0;
strcpy(byte_to_str,"");
strcpy(id,"");

}
