#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <WebSocketsServer.h>
#include <SPIFFS.h>
#include <esp_now.h>
#include <ArduinoJson.h>

AsyncWebServer server(80); // server on port 80
WebSocketsServer webSocket = WebSocketsServer(81); // websocket on port 81

// Relay MAC address
uint8_t receiverMacAddr1[] = {0x58, 0xBF, 0x25, 0xDC, 0x54, 0x8E};
esp_now_peer_info_t peer1;

uint8_t receiverMacAddr2[] = {0x58, 0xBF, 0x25, 0xDC, 0x52, 0x18};
esp_now_peer_info_t peer2;

// Relay state data structure
struct __attribute__((packed)) datapacket {
  bool relayState;
};

datapacket Relay1State;
datapacket Relay2State;

/******************************************************************************************************
 *****************************************************************************************************/

/**
 * @brief ESP-NOW configuration
 * 
 * @param receiverMacAddr 
 * @param channel 
 * @param encrypt 
 * @return ** esp_now_peer_info_t 
 */
esp_now_peer_info_t addPeer(uint8_t *receiverMacAddr, int channel, bool encrypt){
  esp_now_peer_info_t peer;
  memcpy(peer.peer_addr, receiverMacAddr, 6);
  peer.channel = channel;
  peer.encrypt = encrypt;
  if (esp_now_add_peer(&peer) == ESP_OK){
    Serial.printf("Peered: %02x:%02x:%02x:%02x:%02x:%02x\n", peer.peer_addr[0], peer.peer_addr[1], peer.peer_addr[2], peer.peer_addr[3], peer.peer_addr[4], peer.peer_addr[5]);
  }
  return peer;
}

void transmissionComplete(uint8_t *receiver_mac, uint8_t transmissionStatus){
  if (transmissionStatus == 0){
    Serial.println("Data sent");
  } else {
    Serial.print("Error Code: ");
    Serial.println(transmissionStatus);
  }
}

/**
 * @brief Handles Clients Request
 * 
 * @param request 
 * @return ** void 
 */
void onIndexRequest(AsyncWebServerRequest *request){
  request->send(SPIFFS, "/index.html", "text/html");
}

void notFound(AsyncWebServerRequest *request){
  request->send(404, "text/html", "<h1>Not Found</h1>");
}

/**
 * @brief Handles WebSocket Event
 * 
 * @param num 
 * @param type 
 * @param payload 
 * @param length 
 * @return ** void 
 */
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {

  switch(type) {
    
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;

    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

				// send message to client
				webSocket.sendTXT(num, "Connected");
      }
      break;

    case WStype_TEXT:
      Serial.printf("[%u] get Text: %s\n", num, payload);
      String message = (String)((char*)payload);
      // Serial.println(message);

      DynamicJsonDocument data(200);
      DeserializationError error = deserializeJson(data, message);
      if(error){
        Serial.print("deserialization failed: ");
        Serial.println(error.c_str());
        return;
      }
      int relay1State = data["Relay1State"];
      int relay2State = data["Relay2State"];

      Serial.printf("Relay1State: %d\n", relay1State);
      Serial.printf("Relay2State: %d\n", relay2State);

      if (data["Action"] == "toggle"){
        Relay1State.relayState = relay1State;
        Relay2State.relayState = relay2State;
        esp_now_send(peer1.peer_addr, (uint8_t *)&Relay1State, sizeof(Relay1State));
        esp_now_send(peer2.peer_addr, (uint8_t *)&Relay2State, sizeof(Relay2State));
        String toClient;
        DynamicJsonDocument Data(200);
        Data["relay1state"] = ((relay1State == 1) ? "ON" : "OFF");
        Data["relay2state"] = ((relay2State == 1) ? "ON" : "OFF");
        serializeJson(data, toClient);
        // webSocket.broadcastTXT((Relay1State.relayState == 1) ? "ON" : "OFF");
        webSocket.broadcastTXT(toClient);        
      }
      break;

  }

}

/************************************************************************************************
 ***********************************************************************************************/

void setup(){
  Serial.begin(115200);

  WiFi.mode(WIFI_AP_STA);
  WiFi.disconnect();
  WiFi.softAP("ESP-phantom", "");
  Serial.println("softap");
  Serial.println();
  Serial.println(WiFi.softAPIP());

  if(esp_now_init() != ESP_OK ){
    Serial.println("ESP-NOW initialization failed");
    return;
  }

  peer1 = addPeer(receiverMacAddr1, 0, false);
  peer2 = addPeer(receiverMacAddr2, 0, false);

  esp_now_register_send_cb((esp_now_send_cb_t)transmissionComplete);

  server.on("/", onIndexRequest);
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest * request){
    request->send(SPIFFS, "/style.css", "text/css");
  });
  server.on("/control.js", HTTP_GET, [](AsyncWebServerRequest * request){
    request->send(SPIFFS, "/control.js", "text/javascript");
  });
  server.onNotFound(notFound);

  SPIFFS.begin();
  server.begin();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void loop(){
  webSocket.loop();
}







