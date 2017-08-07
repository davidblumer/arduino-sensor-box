#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
const char *ssid = "";
const char *password = "";
ESP8266WebServer server ( 80 );

float temperature;
// which analog pin to connect
#define THERMISTORPIN A0         
// resistance at 25 degrees C
#define THERMISTORNOMINAL 20000      
// temp. for nominal resistance (almost always 25 C)
#define TEMPERATURENOMINAL 25   
// how many samples to take and average, more takes longer
// but is more 'smooth'
#define NUMSAMPLES 5
// The beta coefficient of the thermistor (usually 3000-4000)
#define BCOEFFICIENT 3950
// the value of the 'other' resistor
#define SERIESRESISTOR 10000   

#define BLUELED 4
#define GREENLED 5
 
uint16_t samples[NUMSAMPLES];

void handleRoot() {
    char temperature_temp[6];
    dtostrf(temperature, 4, 2, temperature_temp);

    char temp[400];
    int sec = millis() / 1000;
    int min = sec / 60;
    int hr = min / 60;
    snprintf ( temp, 400,
"{uptime: %02d:%02d:%02d, temp: %s, name:'office'}",
        hr, min % 60, sec % 60, temperature_temp
    );
    server.send ( 200, "application/json", temp );
}
void handleNotFound() {
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
    for ( uint8_t i = 0; i < server.args(); i++ ) {
        message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
    }
    server.send ( 404, "text/plain", message );
}

 
void setup(void) {
  Serial.begin(9600);
  WiFi.begin ( ssid, password );
  Serial.println ( "" );
  
  // Blue LED
  pinMode(BLUELED, OUTPUT);

  // Green LED
  pinMode(GREENLED, OUTPUT);
  // Wait for connection
  while ( WiFi.status() != WL_CONNECTED ) {
      digitalWrite(BLUELED, HIGH);
      delay ( 250 );
      digitalWrite(BLUELED, LOW);
      delay(250);
  }

  digitalWrite(BLUELED, HIGH);
  server.on ( "/", handleRoot );
  server.onNotFound ( handleNotFound );
  server.begin();
}
 
void loop(void) {
  digitalWrite(GREENLED, HIGH);
  server.handleClient();
  uint8_t i;
  float average;
 
  // take N samples in a row, with a slight delay
  for (i=0; i< NUMSAMPLES; i++) {
   samples[i] = analogRead(THERMISTORPIN);
   delay(10);
  }
 
  // average all the samples out
  average = 0;
  for (i=0; i< NUMSAMPLES; i++) {
     average += samples[i];
  }
  average /= NUMSAMPLES;

  // convert the value to resistance
  average = 1023 / average - 1;
  average = SERIESRESISTOR / average;
 
  // (R/Ro)
  temperature = average / THERMISTORNOMINAL;     
  // ln(R/Ro)
  temperature = log(temperature);                  
  // 1/B * ln(R/Ro)
  temperature /= BCOEFFICIENT;                   
  // + (1/To)
  temperature += 1.0 / (TEMPERATURENOMINAL + 273.15);
  // Invert
  temperature = 1.0 / temperature;                 
  // convert to C
  temperature -= 273.15;
}

