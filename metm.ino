// Make Every Thing Move by aegelsky ( https://www.instructables.com/member/aegelsky/ )
// using libs and works:
// https://www.instructables.com/id/WiFi-WebSocket-Remote-Robot/
// https://github.com/bblanchon/ArduinoJson
// https://github.com/tzapu/WiFiManager
// https://github.com/Links2004/arduinoWebSockets
// https://gist.github.com/rjrodger/1011032

#include <Servo.h>
#include <ArduinoJson.h>

#include <DNSServer.h> //
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>  //
#include <ESP8266WebServer.h>
#include <Hash.h> // ??
#include <WebSocketsServer.h>
#include <strings_en.h> //
#include <WiFiManager.h>

#include <Arduino.h> //



#define MDNS_NAME "metm"

#define MOTOR1_PWM_PIN  12
#define MOTOR1_A_PIN    10
#define MOTOR1_B_PIN    5

#define MOTOR2_PWM_PIN  14
#define MOTOR2_A_PIN    16
#define MOTOR2_B_PIN    13

#define SERVO1_PIN      4
#define SERVO2_PIN      15

ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

byte usePins[] = {MOTOR1_PWM_PIN, MOTOR1_A_PIN, MOTOR1_B_PIN, MOTOR2_PWM_PIN, MOTOR2_A_PIN, MOTOR2_B_PIN};
Servo servo1, servo2;

void use_motor(int id, int pwm_amt) {
  int a_pin_val = 0, 
    b_pin_val = 0, 
    pwm_pin = MOTOR1_PWM_PIN,
    a_pin = MOTOR1_A_PIN,
    b_pin = MOTOR1_B_PIN;
  
  if (pwm_amt > 0) {
    a_pin_val = 1;
  } else {
    b_pin_val = 1;
  }

  if (id == 2) {
    pwm_pin = MOTOR2_PWM_PIN;
    a_pin = MOTOR2_A_PIN,
    b_pin = MOTOR2_B_PIN;
  }
  
  analogWrite(pwm_pin, abs(pwm_amt));
  digitalWrite(a_pin, a_pin_val);
  digitalWrite(b_pin, b_pin_val);
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      use_motor(1, 0); // stop if disconnected
      use_motor(2, 0);
      break;
    case WStype_CONNECTED: {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        // send message to client
        webSocket.sendTXT(num, "Connected");
      }
      break;
    case WStype_TEXT:
      Serial.printf("[%u] get Text: %s\n", num, payload);

      Serial.println(1);

      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, payload);

      Serial.println(2);
      
      // Test if parsing succeeds.
      if (error) {
        Serial.printf("deserializeJson() failed: %s\n", payload);
        Serial.println(error.c_str());
        return;
      }

      Serial.println(3);

      JsonObject obj = doc.as<JsonObject>();

      Serial.println(4);

      for (JsonPair p : obj) {
        Serial.println(5);
        const char* type = p.key().c_str();
        int type_id = (type[1] == '2')? 2 : 1;
        int val = p.value().as<signed int>();
        Serial.println(type); // is a JsonString
        Serial.println(val); // is a JsonVariant

        if (type[0] == 'm') {
          Serial.printf("using motor # %d, pwm = %d\n", type_id, val);
          use_motor(type_id, val);
        } else if (type[0] == 's') {
          if (type_id == 2) {
            servo2.write(val);
          } else {
            servo1.write(val);
          }
        }
      }

      Serial.println(6);

      break;
  }
}

void setup() {
  Serial.begin(74880);
  //Serial.setDebugOutput(true);

  Serial.println("11asd1");

  // init 
  for (int i = 0; i < sizeof(usePins); i++) {
    Serial.println(i);
    Serial.println(usePins[i]);
    pinMode(usePins[i], OUTPUT);
    delay(100);
//    digitalWrite(usePins[i], LOW);  // ???
  }

  Serial.println("222");

  servo1.attach(SERVO1_PIN);
  servo2.attach(SERVO2_PIN);

  delay(500);

  Serial.println("333");
  
  WiFiManager wifiManager;
  //reset saved settings
  //wifiManager.resetSettings();
  wifiManager.autoConnect(MDNS_NAME);

  Serial.println("444");

  for (uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] BOOT WAIT %d...\n", t);
    Serial.flush();
    delay(1000);
  }

  // start webSocket server
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  if (MDNS.begin(MDNS_NAME)) {
    Serial.println("MDNS responder started");
  } else {
    Serial.println("MDNS responder FAILED!");
  }

  // handle index
  server.on("/", []() {
    // send index.html
    server.send(200, "text/html", "<!DOCTYPE html><html><head><meta name='viewport' content='user-scalable=no,initial-scale=1.0,maximum-scale=1.0' /><style>body{padding:0 24px 0 24px;background-color:#ccc;}#main{margin:0 auto 0 auto;}</style><script>function nw(){return new WebSocket('ws://'+location.hostname+':81/',['arduino']);}var ws=nw();window.onload=function(){document.ontouchmove=function(e){e.preventDefault();};var cv=document.getElementById('main');var ctop=cv.offsetTop;var c=cv.getContext('2d');function clr(){c.fillStyle='#fff';c.rect(0,0,255,255);c.fill();}function t(e){e.preventDefault();var x,y,u=e.touches[0];if(u){x=u.clientX;y=u.clientY-ctop;c.beginPath();c.fillStyle='rgba(96,96,208,0.5)';c.arc(x,y,5,0,Math.PI*2,true);c.fill();c.closePath();}else{x=128;y=128;}x=x.toString(16);y=y.toString(16);if(x.length<2){x='0'+x;}if(y.length<2){y='0'+y;}if(ws.readyState===ws.CLOSED){ws=nw();}ws.send('#'+x+y);}cv.ontouchstart=function(e){t(e);clr();};cv.ontouchmove=t;cv.ontouchend=t;clr();}</script></head><body><h2>ESP TOUCH REMOTE</h2><canvas id='main' width='255' height='255'></canvas></body></html>");
  });

  server.begin();

  // Add service to MDNS
  MDNS.addService("http", "tcp", 80);
  MDNS.addService("ws", "tcp", 81);
}

unsigned long last_10sec = 0;
unsigned int counter = 0;

void loop() {
  unsigned long t = millis();
  webSocket.loop();
  server.handleClient();

  if ((t - last_10sec) > 10 * 1000) {
    counter++;
    bool ping = (counter % 2);
    int i = webSocket.connectedClients(ping);
    Serial.printf("%d Connected websocket clients ping: %d\n", i, ping);
    last_10sec = millis();
  }
}
