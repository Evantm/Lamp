#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <math.h>
#include <Wire.h>
#include <BlynkSimpleEsp8266.h>
#include <BMP280.h>
#include <WiFiUdp.h>
#include "time_ntp.h"

#define P0 1022.5 //kbar of pressure at sea level at current location

int gpio0_pin = 16;

const char* ssid = "Evan";
const char* password = "#######";
char auth[] = "############"; //blynk auth

ESP8266WebServer server(80);
MDNSResponder mdns;
BlynkTimer timer;
BMP280 bmp;


String webPage;
String points = "";


void setup() {
  pinMode(16, OUTPUT); //for the relay
  Serial.begin(9600); 
  WiFi.begin(ssid, password);
  
  
  Wire.begin(4, 5); // sda, scl I2C for BMP
  Blynk.begin(auth, ssid, password);

  bmp.setOversampling(4);
  
  if(!bmp.begin()){
    Serial.println("BMP init failed!");
    while(1);
  }
  else Serial.println("BMP init success!");

  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.print("WiFi connected with ip ");  
  Serial.println(WiFi.localIP());

  server.on("/", [](){

      if(server.hasArg("status")){
      String UsrName = "name";
      points +=   "\n data.addRows([\n [new Date(";
      points += String(getNTPTimestamp())+= "000"; //What even. multiplying by 1000 doesnt work but appending 000 to the end does. 
      if(server.hasArg("name")){
        UsrName = server.arg("name") != "" ? server.arg("name") : "Name";
      }
      if(server.arg("status") == "on"){
          digitalWrite(16, LOW);
          points += "), 1,'";
          points += UsrName;
          points += " turned lamp On'],\n  ]);\n \n";
      }else{
          digitalWrite(16, HIGH);
          points += "), 0,'";
          points += UsrName;
          points += " turned lamp Off'],\n  ]);\n \n";
      }
    }

    webPage.replace("//--",points);
    server.send(200, "text/html", webPage);
    delay(1000);
    });

  
  
  webPage += "<p>Lamp <a href=\"LampOn\"><button>ON</button></a>&nbsp;<a href=\"LampOff\"><button>OFF</button></a></p>";
  
  server.begin();
  timer.setInterval(1000L, sendSensor);
}

void loop() {
        webPage = 
      "<html>\n"
      "<head>\n"
      "    <script type=\"text/javascript\" src=\"https://www.gstatic.com/charts/loader.js\"></script>\n"
      "    <script type=\"text/javascript\">\n"
      "        google.charts.load('current', {'packages':['annotationchart']});\n"
      "        google.charts.setOnLoadCallback(drawChart);\n"
      "\n"
      "        function drawChart() {\n"
      "            var data = new google.visualization.DataTable();\n"
      "            data.addColumn('date', 'Time');\n"
      "            data.addColumn('number', 'Lamp');\n"
      "            data.addColumn('string', 'On/Off');\n";
      webPage += "//--";
      webPage += "\n"
      "            var chart = new google.visualization.AnnotationChart(document.getElementById('chart_div'));\n"
      "\n"
      "            var options = {\n"
      "                displayAnnotations: true\n"
      "            };\n"
      "\n"
      "            chart.draw(data, options);\n"
      "        }\n"
      "    </script>\n"
      "</head>\n"
      "<body>\n"
      "<div id=\"chart_div\"></div>\n<br>";

      //  -- Form --
      webPage += "<form action=\"/\">"
      "Name: <input type=\"text\" name=\"name\"  placeholder=\"Name\">"
      "Lamp Status: <select name=\"status\">"
      "<option value=\"on\">On</option>"
      "<option value=\"off\">Off</option>"
      "</select>"
      "<br><br>"
      "<input type=\"submit\">"
      "</form>";

  double T,P;
  char result = bmp.startMeasurment();
 
  if(result!=0){
    delay(result);
    result = bmp.getTemperatureAndPressure(T,P);
    
      if(result!=0)
      {
        double A = bmp.altitude(P,P0);
        
        webPage += "<p>Pressure  ";
        webPage +=  (floorf(P * 100) / 100);
        webPage += " mBar</p>";
        
        webPage += "<p>Altitute  ";
        webPage +=  (floorf(A * 100) / 100);
        webPage += " m</p>";
        
        webPage += "<p>Temp  ";
        webPage +=  (floorf(T * 100) /  100);
        webPage += " deg C</p></body></html>";
       
      }
      else {
        Serial.println("Error.");
      }
  }
  else {
    Serial.println("Error.");
  }
  
  server.handleClient();  
  Blynk.run();
  timer.run();

}

void sendSensor()
{
  double T,P;
  char result = bmp.startMeasurment();
 
  if(result!=0){
    delay(result);
    result = bmp.getTemperatureAndPressure(T,P);
    
      if(result!=0)
      {  
          double A = bmp.altitude(P,P0);
          Blynk.virtualWrite(V7 ,floorf(P * 100) / 100);
          Blynk.virtualWrite(V8 ,floorf(T * 100) / 100);
          Blynk.virtualWrite(V9 ,floorf(A * 100) / 100);
      }
  }
 
  Serial.println("Blynk good");
}
