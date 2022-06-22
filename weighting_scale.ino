#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WebSocketsServer.h>
#include <ESPAsyncWebServer.h>
#include <HX711.h>  //Weight Sensor library
#include <Ticker.h>  //Time loop library

#define HX711_dout  D5  //esp8266 >> HX711 sck pin
#define HX711_sck   D6  //esp8266 >> HX711 dout pin

HX711 scale;
Ticker timer;

AsyncWebServer server(80); // server port 80
WebSocketsServer websockets(81); // websockets port 81

int value;
char webpage[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Weight Sensor</title>
</head>
<body>
    <div style="text-align: center; margin: auto; width: 40%; margin-top: 90px;">
        <div style="margin-top: 20px; ">
            <h2 style="border: 5px solid #038cfc;border-radius: 20px; padding: 5px 0 5px 0 ;">
                Weight</h2>
        </div>
        <div style="margin-top: 30px;"><h3><span id="value">Reading.... in</span> gms</h3></div>
        <button style="margin-top: 12px; padding: 10px; border-radius: 10px;"
         onclick="tare()">Tare</button>
    </div>
    
    <script>
        var connection = new WebSocket('ws://'+location.hostname+':81/');
        
        connection.onmessage = function(event){
            var data = JSON.parse(event.data);
            weight_val = data.weight;
            document.getElementById('value').textContent = weight_val;
        }

        function tare() {
            console.log("tared");
            connection.send("tare");
        }
    </script>
</body>
</html>)=====";

void setup() {
  delay(200);
  Serial.begin(19200);delay(200);
  scale.begin(HX711_dout, HX711_sck);
  scale.set_scale(-434);
  scale.tare();
  
  WiFi.mode(WIFI_AP);
  WiFi.softAP("Weight Sensor", "");
  Serial.print("IP:");
  Serial.println(WiFi.softAPIP());

  server.on("/", [](AsyncWebServerRequest * request){ 
  request->send_P(200, "text/html", webpage);
  });
  server.onNotFound(notFound);
  server.begin();
  websockets.begin();
  websockets.onEvent(webSocketEvent);
  
  timer.attach(0.1,send_value);
  Serial.println("Setup Done");
}

void loop() {
  websockets.loop();
  value = scale.get_units(5);
}

void scale_tare(){
  scale.tare();
}

void send_value(){
  String JSON_Data = "{\"weight\":";
         JSON_Data += value;
         JSON_Data += "}";
  websockets.broadcastTXT(JSON_Data);
}

void notFound(AsyncWebServerRequest *request){
  request->send(404, "text/html", "<h2>Page Not found</h2>");
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch (type){
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED: {
        IPAddress ip = websockets.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        websockets.sendTXT(num, "Connected from server"); // send message to client
      }
      break;
    case WStype_TEXT:
      Serial.printf("[%u] get Text: %s\n", num, payload);
      String message = String((char*)( payload));
      if(message == "tare"){
        scale_tare();
      }
  }
}
