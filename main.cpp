#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include "SPIFFS.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>


#define leftF 2
#define rightF 4
#define leftB 5
#define rightB 6
#define Trig 9
#define Echo 10
#define led 8


WiFiServer server(80);
WiFiClient client;

QueueHandle_t xQueue;

void loop2(void * parameter);



void handleButton(int pin, bool buttonPressed,int &value) {
  if (buttonPressed) {
    //Serial.print("GPIO ");
    Serial.print(pin);
    //Serial.println(" ON");
    analogWrite(pin, value * 28);
  } else {
    //Serial.print("GPIO ");
    Serial.print(pin);
   //Serial.println(" OFF");
    analogWrite(pin, 0);
  }
}

void handleSliderValue(int &value) {
  Serial.print("Slider Value: ");
  Serial.println(value);
}

void serveIndexPage(WiFiClient client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println("Connection: close");
  client.println();

  //sending html to client
  File file = SPIFFS.open("/index.html", "r");
  if (file) {
    while (file.available()) {
      client.write(file.read());
    }
    file.close();
  }
}

void serveCssFile(WiFiClient client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/css");
  client.println("Connection: close");
  client.println();

  //sending css to client
  File file = SPIFFS.open("/style.css", "r");
  if (file) {
    while (file.available()) {
      client.write(file.read());
    }
    file.close();
  }
}

void setup() {
  Serial.begin(115200);

  const char *ssid = "Robot";
  const char *password = "12345678";

  pinMode(leftF, OUTPUT);
  digitalWrite(leftF, LOW);
  pinMode(leftB, OUTPUT);
  digitalWrite(leftB, LOW);
  pinMode(rightF, OUTPUT);
  digitalWrite(rightF, LOW);
  pinMode(rightB, OUTPUT);
  digitalWrite(rightB, LOW);
  pinMode(led, OUTPUT);
  digitalWrite(led, LOW);
  pinMode(Trig, OUTPUT);  
  pinMode(Echo, INPUT);  

  Serial.println(xPortGetCoreID());
  WiFi.mode(WIFI_AP);

  Serial.println();
  Serial.println("Configuring access point...");
  WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.begin();


  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  xQueue = xQueueCreate(1, sizeof(int));
  xTaskCreatePinnedToCore(
      loop2,   
      "Task1", 
      10000,      
      NULL,       
      1,           
      NULL,        
      0          
  );


}


void loop() {

  WiFiClient client = server.available();

  bool upButtonPressed = false;
  bool downButtonPressed = false;
  bool leftButtonPressed = false;
  bool rightButtonPressed = false;
  int sliderValue;
  int cmFromQueue;
 
  if (xQueueReceive(xQueue, &cmFromQueue, 0) == pdTRUE) {
    //Serial.println(cmFromQueue);
    if (cmFromQueue < 10){
    //Serial.println("CM < 10");
    digitalWrite(led, HIGH);
    }
    else{
    //Serial.pritln("CM >= 10");n
    digitalWrite(led, LOW);
  }

  }
   if (client)
   {

     String currentLine = "";

     while (client.connected())
     
     {
     
       if (client.available())
       {
         char c = client.read();
         if (c == '\n')
         {
           if (currentLine.startsWith("GET /upButtonPressed")) //handling buttons
           {
             upButtonPressed = true;
             handleButton(leftF, upButtonPressed, sliderValue);
             handleButton(rightF, upButtonPressed, sliderValue);
             break;
           }
           else if (currentLine.startsWith("GET /upButtonReleased"))
           {
             upButtonPressed = false;
             handleButton(leftF, upButtonPressed, sliderValue);
             handleButton(rightF, upButtonPressed, sliderValue);
             break;
           }
           
           else if (currentLine.startsWith("GET /leftButtonPressed"))
           {
             leftButtonPressed = true;
             handleButton(leftF, leftButtonPressed, sliderValue);

             break;
           }
           else if (currentLine.startsWith("GET /leftButtonReleased"))
           {
             leftButtonPressed = false;
             handleButton(leftF, leftButtonPressed, sliderValue);
             break;
           }
           else if (currentLine.startsWith("GET /rightButtonPressed"))
           {
             rightButtonPressed = true;
             handleButton(rightF, rightButtonPressed, sliderValue);
             break;
           }
           else if (currentLine.startsWith("GET /rightButtonReleased"))
           {
             rightButtonPressed = false;
             handleButton(rightF, rightButtonPressed,sliderValue);
             break;
           }
           else if (currentLine.startsWith("GET /downButtonPressed"))
           {
             downButtonPressed = true;
             handleButton(leftB, downButtonPressed, sliderValue);
             handleButton(rightB, downButtonPressed, sliderValue);
             break;
           }
           else if (currentLine.startsWith("GET /downButtonReleased"))
           {
             downButtonPressed = false;
             handleButton(leftB, downButtonPressed, sliderValue);
             handleButton(rightB, downButtonPressed, sliderValue);
             break;
           }
           else if (currentLine.startsWith("GET /setSliderValue")) //handling slider
           {
             int valueIndex = currentLine.indexOf("value=");
             if (valueIndex != -1)
             {
               String valueStr = currentLine.substring(valueIndex + 6);
               sliderValue = valueStr.toInt();
               handleSliderValue(sliderValue);
             }
             break;
           }
          
           else if (currentLine.startsWith("GET /"))
           {
             if (currentLine.indexOf("GET /style.css") != -1) {
              serveCssFile(client);
            } else {
                serveIndexPage(client);
            }
            break;
           } else {
            currentLine = "";
             }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
     }

    client.stop();
    //Serial.println("Client disconnected.");
    //Serial.println("");
  }

}
void loop2(void *parameter){

  int cm;        //odległość w cm
  long time;     //długość powrotnego impulsu w uS

 while (1) {
  digitalWrite(Trig, LOW); 
  vTaskDelay(2 / portTICK_PERIOD_MS);
  digitalWrite(Trig, HIGH);
  vTaskDelay(10 / portTICK_PERIOD_MS);
  digitalWrite(Trig, LOW);
  digitalWrite(Echo, HIGH); 
  time = pulseIn(Echo, HIGH);
  cm = time / 58;   


  xQueueSend(xQueue, &cm, portMAX_DELAY);
  vTaskDelay(100 / portTICK_PERIOD_MS);
  }

}

