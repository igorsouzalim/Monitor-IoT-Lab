/*
  Rui Santos
  Complete project details at Complete project details at https://RandomNerdTutorials.com/esp32-http-get-open-weather-map-thingspeak-arduino/

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include "DHTesp.h"
#include "time.h"
//#include <WebServer.h>

#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
//WebServer server(80);

const char* ssid = "Vivo-Internet-54A4";
const char* password = "CFC92198E76";

int days,DST=0;
const char* ntpServer = "pool.ntp.org"; 
const long  gmtOffset_sec = -10800;  //GMT+1 for Greece, Default gmtOffset_sec=3600 for Europe GMT
const int   daylightOffset_sec = 0;

int flagConnection = 0;

// Your Domain name with URL path or IP address with path
String openWeatherMapApiKey = "46a756b1738cdb805ffe083cd5a2c6c4";
// Example:
//String openWeatherMapApiKey = "bd939aa3d23ff33d3c8f5dd1dd435";

// Replace with your country code and city
String city = "Rio de Janeiro";
String countryCode = "BR";

String final_time_string;
String final_time_string2;
String final_date_string;
String final_date_string2;
String final_date_string3;

uint8_t DHTPin = 4; 

char last_letter;
char last_letter2;

float temperaturein = 0;
float Temperature = 0;
float humidityin = 0;
float Humidity = 0;
float backuptemp=0;   // backup temperature in case of nan error
float backuphumi=0;   // backup humidity in case of nan error
  int weatherID = 0;    
int humout = 0, tempout=0;
int  iterations = 0;

// THE DEFAULT TIMER IS SET TO 10 SECONDS FOR TESTING PURPOSES
// For a final application, check the API call limits per hour/minute to avoid getting blocked/banned
unsigned long lastTime = 0;
// Timer set to 10 minutes (600000)
//unsigned long timerDelay = 600000;
// Set timer to 10 seconds (10000)
unsigned long timerDelay = 10000;

DHTesp dht;

String jsonBuffer;

void setup() {
  Serial.begin(115200);
  dht.setup(DHTPin, DHTesp::DHT22);    

  delay(100);
  Serial.print("page 0");    // Inicia com pagina 1 = imagem logo senai
  endNextionCommand();

  IPAddress dns1(8, 8, 8, 8);    // Servidor DNS primário
  IPAddress dns2(8, 8, 4, 4);    // Servidor DNS secundário
  WiFi.config(IPAddress(0, 0, 0, 0), IPAddress(0, 0, 0, 0), IPAddress(0, 0, 0, 0), dns1, dns2);

  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  //server.on("/", handle_OnConnect);
  //server.onNotFound(handle_NotFound);

  //server.begin(); //start server
  //Serial.println("HTTP server started");

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); //setup time
  printLocalTime(); //print time
 
 // Serial.println("Timer set to 10 seconds (timerDelay variable), it will take 10 seconds before publishing the first reading.");
}

void loop() {

  if(WiFi.status()== WL_CONNECTED && flagConnection == 0)
  {
    Serial.println();
    for(int i=0;i<3;i++)
    {
      Serial.print("page 1");    // Muda para pagina que mostra os dados do tempo
      endNextionCommand();
      delay(100);
    }

    flagConnection = 1;

  }
  else if(WiFi.status()!= WL_CONNECTED)
  {
    Serial.println();
    for(int i=0;i<3;i++)
    {
      Serial.print("page 0");    // Muda para pagina que mostra os dados do tempo
      endNextionCommand();
      delay(100);
    }
    flagConnection = 0;
  }

   if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Conexão WiFi perdida. Reconectando...");
    WiFi.reconnect();
  }

 // server.handleClient();
  printLocalTime();
  printWeatherIcon(weatherID);
  getWeather();
  printWeatherIcon(weatherID);  
  getOpenWeather();
  sendopenweatherTemperatureToNextion(); //send outside temperature from openweather to nextion display
  sendopenweatherHumidityToNextion();    //send outside humidity from openweather to nextion display
  printfinaldate();                      //send date to nextion diplay
  printfinaltime();                      //send time to nextion diplay
  getTemperature();                     //measure inside temperature every 20 seconds
  sendTemperatureToNextion();           //send inside temperature from openweather to nextion display
  getHumidity();                        //measure inside humidity every 25 seconds
  sendHumidityToNextion();              //send inside humidity from openweather to nextion display

  delay(5000);

}


void getWeather(){

  // Send an HTTP GET request
  //if ((millis() - lastTime) > timerDelay) {
    // Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED){
      String serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + countryCode + "&APPID=" + openWeatherMapApiKey;
      
      jsonBuffer = httpGETRequest(serverPath.c_str());
      //Serial.println(jsonBuffer);
      JSONVar myObject = JSON.parse(jsonBuffer);
  
      // JSON.typeof(jsonVar) can be used to get the type of the var
      if (JSON.typeof(myObject) == "undefined") {
        //Serial.println("Parsing input failed!");
        return;
      }
    
      //Serial.print("Weather Icon: ");
      //Serial.println(myObject["weather"][0]["id"]);
      int idString = myObject["weather"][0]["id"];
      weatherID = idString;

      Serial.print("\nWeatherID: ");
      Serial.print(weatherID);
      endNextionCommand(); //We need that in order the nextion to recognise the first command after the serial print

    }
    else {
      Serial.println("WiFi Disconnected");
    }
    lastTime = millis();
  //}
}

void getOpenWeather(){

   //if ((millis() - lastTime) > timerDelay) {
    // Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED){
      String serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + countryCode + "&APPID=" + openWeatherMapApiKey;
      
      jsonBuffer = httpGETRequest(serverPath.c_str());
      //Serial.println(jsonBuffer);
      JSONVar myObject = JSON.parse(jsonBuffer);
  
      // JSON.typeof(jsonVar) can be used to get the type of the var
      if (JSON.typeof(myObject) == "undefined") {
        //Serial.println("Parsing input failed!");
        return;
      }

    humout = myObject["main"]["humidity"]; // Extract local humidity now
    //Serial.print("\nhumi: ");
    //Serial.print(humout);
    
    int tOut = myObject["main"]["temp"]; // Extract local temp now
    if (tOut == 0) tOut=273;
    tempout = tOut - 273;
    //Serial.print("\ntempout: ");
    //Serial.print(tempout);
    endNextionCommand();

    }
    else {
      Serial.println("WiFi Disconnected");
    }
    lastTime = millis();
  //}
}
/*
void handle_OnConnect() {

  Temperature = dht.getTemperature(); // Gets the values of the temperature
  Humidity = dht.getHumidity(); // Gets the values of the humidity 

  if (!isnan(temperaturein))
  {
      server.send(200, "text/html", SendHTML(temperaturein,temperaturein,humidityin)); 
      //Serial.print("Temperature:");
      //Serial.println(dht.getTemperature());
  }
  if (!isnan(temperaturein))
  {
      server.send(200, "text/html", SendHTML(backuptemp,backuptemp,backuphumi)); 
     // Serial.print("Temperature:");
      //Serial.println(dht.getTemperature());    
  }
}

void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
}
*/

void printLocalTime()
{
  String min_final;
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo))
  {
    //Serial.println("Failed to obtain time");
    return;
  }
  //Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  

  if (timeinfo.tm_min<=9) final_time_string2 = "0" + String(timeinfo.tm_min); 
  else final_time_string2 = String(timeinfo.tm_min);

  if (timeinfo.tm_hour<=9) final_time_string = "0" + String(timeinfo.tm_hour); 
  else final_time_string = String(timeinfo.tm_hour);

  final_date_string = timeinfo.tm_mday;
  final_date_string2 = timeinfo.tm_mon+1;
  final_date_string3=timeinfo.tm_year+1900;

  int length = final_date_string3.length();
  final_date_string3[length-1] = last_letter;   // we need to get the last two numbers of 2019 because of lack of space
  final_date_string3[length-2] = last_letter2;
  //Serial.println(last_letter);
  //Serial.println(last_letter2);
  //Serial.println(final_date_string3[length-1]);
  //Serial.println(final_date_string3[length-2]);
  
}


void getTemperature()
{
  temperaturein = dht.getTemperature();
  if (isnan(temperaturein))
  {
     digitalWrite(DHTPin, LOW);  
     delay(1000);
     digitalWrite(DHTPin, HIGH);
     delay(1000);
  }
  if (!isnan(temperaturein)) //save last value and get a backup in case of a nan error
  {
     backuptemp=temperaturein;
  }  
}

void getHumidity()
{
  humidityin = dht.getHumidity();
  if (isnan(humidityin))  //save last value and get a backup in case of a nan error
  {
     digitalWrite(DHTPin, LOW);  
     delay(1000);
     digitalWrite(DHTPin, HIGH);
     delay(1000);
  } 
  if (!isnan(humidityin))  //save last value and get a backup in case of a nan error
  {
    backuphumi=humidityin;
  }
}



String httpGETRequest(const char* serverName) {
  WiFiClient client;
  HTTPClient http;
    
  // Your Domain name with URL path or IP address with path
  http.begin(client, serverName);
  
  // Send HTTP POST request
  int httpResponseCode = http.GET();
  
  String payload = "{}"; 
  
  if (httpResponseCode>0) {
   // Serial.print("HTTP Response code: ");
    //Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    //Serial.print("Error code: ");
    //Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

  return payload;
}

void showConnectingIcon()
{
  Serial.println();
  String command = "weatherIcon.pic=3";
  Serial.print(command);
  endNextionCommand();
}

void showconnectionweatherlater1()
{
  Serial.println();
  String command = "weatherpic1.pic=3";
  Serial.print(command);
  endNextionCommand();
}

void printfinaldate()
{
   String command2 = "date.txt=\""+String(final_date_string+"/"+final_date_string2+"/"+"23")+"\"";
   Serial.print(command2);
   endNextionCommand();
}

void printfinaltime()
{
  String command = "time.txt=\""+String(final_time_string+":"+final_time_string2)+"\"";  
  Serial.print(command);
  endNextionCommand();  
}

void sendopenweatherTemperatureToNextion()
{ 
  int t = (int)tempout;
  String command = "temperatureout.txt=\""+String(t)+".0"+"\"";
  Serial.print(command);
  endNextionCommand();
}

void sendopenweatherHumidityToNextion()
{
  int h = (int)humout;
  String command = "humidityout.txt=\""+String(h)+".0"+"\"";
  Serial.print(command);
  endNextionCommand();
}

void sendHumidityToNextion()
{
  if (!isnan(humidityin))
  {
      String command = "humidity.txt=\""+String(humidityin)+"\"";
      Serial.print(command);
      endNextionCommand();
  }
  if (isnan(humidityin))
  {
      String command = "humidity.txt=\""+String(backuphumi)+"\"";
      Serial.print(command);
      endNextionCommand();
  }
}

void sendTemperatureToNextion()
{
  if (!isnan(temperaturein))
  {
    String command = "temperaturein.txt=\""+String(temperaturein,1)+"\"";
    Serial.print(command);
    endNextionCommand();
  }
  if (isnan(temperaturein))
  {
    String command = "temperaturein.txt=\""+String(backuptemp,1)+"\"";
    Serial.print(command);
    endNextionCommand();
  }   
}

void endNextionCommand()
{
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
}



void printWeatherIcon(int id)
{
 switch(id)
 {
  case 800: drawClearWeather(); break;
  case 801: drawFewClouds(); break;
  case 802: drawFewClouds(); break;
  case 803: drawCloud(); break;
  case 804: drawCloud(); break;
  
  case 200: drawThunderstorm(); break;
  case 201: drawThunderstorm(); break;
  case 202: drawThunderstorm(); break;
  case 210: drawThunderstorm(); break;
  case 211: drawThunderstorm(); break;
  case 212: drawThunderstorm(); break;
  case 221: drawThunderstorm(); break;
  case 230: drawThunderstorm(); break;
  case 231: drawThunderstorm(); break;
  case 232: drawThunderstorm(); break;

  case 300: drawLightRain(); break;
  case 301: drawLightRain(); break;
  case 302: drawLightRain(); break;
  case 310: drawLightRain(); break;
  case 311: drawLightRain(); break;
  case 312: drawLightRain(); break;
  case 313: drawLightRain(); break;
  case 314: drawLightRain(); break;
  case 321: drawLightRain(); break;

  case 500: drawLightRainWithSunOrMoon(); break;
  case 501: drawLightRainWithSunOrMoon(); break;
  case 502: drawLightRainWithSunOrMoon(); break;
  case 503: drawLightRainWithSunOrMoon(); break;
  case 504: drawLightRainWithSunOrMoon(); break;
  case 511: drawLightRain(); break;
  case 520: drawModerateRain(); break;
  case 521: drawModerateRain(); break;
  case 522: drawHeavyRain(); break;
  case 531: drawHeavyRain(); break;

  case 600: drawLightSnowfall(); break;
  case 601: drawModerateSnowfall(); break;
  case 602: drawHeavySnowfall(); break;
  case 611: drawLightSnowfall(); break;
  case 612: drawLightSnowfall(); break;
  case 615: drawLightSnowfall(); break;
  case 616: drawLightSnowfall(); break;
  case 620: drawLightSnowfall(); break;
  case 621: drawModerateSnowfall(); break;
  case 622: drawHeavySnowfall(); break;

  case 701: drawFog(); break;
  case 711: drawFog(); break;
  case 721: drawFog(); break;
  case 731: drawFog(); break;
  case 741: drawFog(); break;
  case 751: drawFog(); break;
  case 761: drawFog(); break;
  case 762: drawFog(); break;
  case 771: drawFog(); break;
  case 781: drawFog(); break;

  default:break; 
 }
}



void drawFog()
{
  String command = "weatherIcon.pic=13";
  Serial.print(command);
  endNextionCommand();
}

void drawHeavySnowfall()
{
  String command = "weatherIcon.pic=8";
  Serial.print(command);
  endNextionCommand();
}

void drawModerateSnowfall()
{
  String command = "weatherIcon.pic=8";
  Serial.print(command);
  endNextionCommand();
}

void drawLightSnowfall()
{
  String command = "weatherIcon.pic=11";
  Serial.print(command);
  endNextionCommand();
}

void drawHeavyRain()
{
  String command = "weatherIcon.pic=10";
  Serial.print(command);
  endNextionCommand();
}

void drawModerateRain()
{
  String command = "weatherIcon.pic=6";
  Serial.print(command);
  endNextionCommand();
}

void drawLightRain()
{
  String command = "weatherIcon.pic=6";
  Serial.print(command);
  endNextionCommand();
}

void drawLightRainWithSunOrMoon()
{
  String command = "weatherIcon.pic=7";
  Serial.print(command);
  endNextionCommand(); 
}
void drawThunderstorm()
{
  String command = "weatherIcon.pic=3";
  Serial.print(command);
  endNextionCommand();
}

void drawClearWeather()
{
  String command = "weatherIcon.pic=4";
  Serial.print(command);
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
}

void drawCloud()
{
  String command = "weatherIcon.pic=9";
  Serial.print(command);
  endNextionCommand();
}

void drawFewClouds()
{
  String command = "weatherIcon.pic=5";
  Serial.print(command);
  endNextionCommand(); 
}

String SendHTML(float TempCstat,float TempFstat,float Humiditystat){
   String ptr = "<!DOCTYPE html> <html>\n";
  ptr +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr +="<link href=\"https://fonts.googleapis.com/css?family=Open+Sans:300,400,600\" rel=\"stylesheet\">\n";
  ptr +="<title>ESP32 Weather Report</title>\n";
  ptr +="<style>html { font-family: 'Open Sans', sans-serif; display: block; margin: 0px auto; text-align: center;color: #333333;}\n";
  ptr +="body{margin-top: 50px;}\n";
  ptr +="h1 {margin: 50px auto 30px;}\n";
  ptr +=".side-by-side{display: inline-block;vertical-align: middle;position: relative;}\n";
  ptr +=".humidity-icon{background-color: #3498db;width: 30px;height: 30px;border-radius: 50%;line-height: 36px;}\n";
  ptr +=".humidity-text{font-weight: 600;padding-left: 15px;font-size: 19px;width: 160px;text-align: left;}\n";
  ptr +=".humidity{font-weight: 300;font-size: 60px;color: #3498db;}\n";
  ptr +=".temperature-icon{background-color: #f39c12;width: 30px;height: 30px;border-radius: 50%;line-height: 40px;}\n";
  ptr +=".temperature-text{font-weight: 600;padding-left: 15px;font-size: 19px;width: 160px;text-align: left;}\n";
  ptr +=".temperature{font-weight: 300;font-size: 60px;color: #f39c12;}\n";
  ptr +=".superscript{font-size: 17px;font-weight: 600;position: absolute;right: -20px;top: 15px;}\n";
  ptr +=".data{padding: 10px;}\n";
  ptr +="</style>\n";
  ptr +="</head>\n";
  ptr +="<body>\n";
  
   ptr +="<div id=\"webpage\">\n";
   
   ptr +="<h1>ESP32 Weather Report</h1>\n";
   ptr +="<div class=\"data\">\n";
   ptr +="<div class=\"side-by-side temperature-icon\">\n";
   ptr +="<svg version=\"1.1\" id=\"Layer_1\" xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" x=\"0px\" y=\"0px\"\n";
   ptr +="width=\"9.915px\" height=\"22px\" viewBox=\"0 0 9.915 22\" enable-background=\"new 0 0 9.915 22\" xml:space=\"preserve\">\n";
   ptr +="<path fill=\"#FFFFFF\" d=\"M3.498,0.53c0.377-0.331,0.877-0.501,1.374-0.527C5.697-0.04,6.522,0.421,6.924,1.142\n";
   ptr +="c0.237,0.399,0.315,0.871,0.311,1.33C7.229,5.856,7.245,9.24,7.227,12.625c1.019,0.539,1.855,1.424,2.301,2.491\n";
   ptr +="c0.491,1.163,0.518,2.514,0.062,3.693c-0.414,1.102-1.24,2.038-2.276,2.594c-1.056,0.583-2.331,0.743-3.501,0.463\n";
   ptr +="c-1.417-0.323-2.659-1.314-3.3-2.617C0.014,18.26-0.115,17.104,0.1,16.022c0.296-1.443,1.274-2.717,2.58-3.394\n";
   ptr +="c0.013-3.44,0-6.881,0.007-10.322C2.674,1.634,2.974,0.955,3.498,0.53z\"/>\n";
   ptr +="</svg>\n";
   ptr +="</div>\n";
   ptr +="<div class=\"side-by-side temperature-text\">Temperature</div>\n";
   ptr +="<div class=\"side-by-side temperature\">";
   ptr +=(float)TempCstat;
   ptr +="<span class=\"superscript\">C</span></div>\n";
   ptr +="</div>\n";
   ptr +="<div class=\"data\">\n";
   ptr +="<div class=\"side-by-side humidity-icon\">\n";
   ptr +="<svg version=\"1.1\" id=\"Layer_2\" xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" x=\"0px\" y=\"0px\"\n\"; width=\"12px\" height=\"17.955px\" viewBox=\"0 0 13 17.955\" enable-background=\"new 0 0 13 17.955\" xml:space=\"preserve\">\n";
   ptr +="<path fill=\"#FFFFFF\" d=\"M1.819,6.217C3.139,4.064,6.5,0,6.5,0s3.363,4.064,4.681,6.217c1.793,2.926,2.133,5.05,1.571,7.057\n";
   ptr +="c-0.438,1.574-2.264,4.681-6.252,4.681c-3.988,0-5.813-3.107-6.252-4.681C-0.313,11.267,0.026,9.143,1.819,6.217\"></path>\n";
   ptr +="</svg>\n";
   ptr +="</div>\n";
   ptr +="<div class=\"side-by-side humidity-text\">Humidity</div>\n";
   ptr +="<div class=\"side-by-side humidity\">";
   ptr +=(int)Humiditystat;
   ptr +="<span class=\"superscript\">%</span></div>\n";
   ptr +="</div>\n";

  ptr +="</div>\n";
  ptr +="</body>\n";
  ptr +="</html>\n";
  return ptr;
}