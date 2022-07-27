/***************************************************************************************************************************************
                       WEATHER STATION WITH ESP32 - DHT 22 - NEXTION DISPLAY - HTTP SERVER 
DIY ESP32 Projects
****************************************************************************************************************************************/
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>    //https://github.com/bblanchon/ArduinoJson
#include "DHTesp.h"
#include "time.h"

#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

/*Put your SSID & Password*/
const char* ssid = "0L4G";  // Enter SSID here
const char* password = "eletronica";  //Enter Password here
String CityID = "3451190"; //Enter your City ID from api.weathermap.org   
String APIKEY = "e390bf21100c1b678be82d6000245002";  // your api_key
#define LED_PIN 2
WebServer server(80);

// DHT Sensor
uint8_t DHTPin = 4; 
               
// Initialize DHT sensor.               
DHTesp dht;


int days,DST=0;
const char* ntpServer = "pool.ntp.org"; 
const long  gmtOffset_sec = 0;  //GMT+1 for Greece, Default gmtOffset_sec=3600 for Europe GMT
const int   daylightOffset_sec = -3600*3;



String final_time_string;
String final_time_string2;
String final_date_string;
String final_date_string2;
String final_date_string3;


float temperaturein = 0;
float Temperature = 0;
float humidityin = 0;
float Humidity = 0;
float backuptemp=0;   // backup temperature in case of nan error
float backuphumi=0;   // backup humidity in case of nan error
  int weatherID = 0;    
float hum = 0;
float tempout=0;

char* servername ="api.openweathermap.org";  // remote server we will connect to
String result;
String result2;
int  iterations = 1800;
String weatherDescription ="";
String weatherLocation = "";
char last_letter;
char last_letter2;

void setup() {
  
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(115200);

  dht.setup(DHTPin, DHTesp::DHT22);          

  Serial.println("Connecting to ");
  Serial.println(ssid);

  //connect to your local wi-fi network
  WiFi.begin(ssid, password);

  //check wi-fi is connected to wi-fi network
  while (WiFi.status() != WL_CONNECTED)
  {
     delay(1000);
     Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected..!");
  Serial.print("Got IP: ");  
  Serial.println(WiFi.localIP());

  server.on("/", handle_OnConnect);
  server.onNotFound(handle_NotFound);

  server.begin(); //start server
  Serial.println("HTTP server started");

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); //setup time
  printLocalTime(); //print time
}



void loop() 
{
   server.handleClient();
   printLocalTime();
   printWeatherIcon(weatherID);
   delay(2000);
   
   if(iterations == 3)//We check for changes in weather every half an hour
   {
      getWeatherData();               
      getWeatheropenweather();
       
      printWeatherIcon(weatherID);  
      iterations = 0;   
   }

    sendopenweatherTemperatureToNextion(); //send outside temperature from openweather to nextion display
    sendopenweatherHumidityToNextion();    //send outside humidity from openweather to nextion display
    printfinaldate();                      //send date to nextion diplay
    printfinaltime();                      //send time to nextion diplay
    delay(5000);
    getTemperature();                     //measure inside temperature every 20 seconds
    sendTemperatureToNextion();           //send inside temperature from openweather to nextion display
    delay(2000);                          //small delay between measurements
    getHumidity();                        //measure inside humidity every 25 seconds
    sendHumidityToNextion();              //send inside humidity from openweather to nextion display
    iterations++;
}


void printLocalTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  


  
  final_time_string=timeinfo.tm_hour;
  final_time_string2=String(timeinfo.tm_min);


  final_date_string=timeinfo.tm_mday;
  final_date_string2=timeinfo.tm_mon+1;
  final_date_string3=timeinfo.tm_year+1900;

  int length = final_date_string3.length();
  final_date_string3[length-1] = last_letter;   // we need to get the last two numbers of 2019 because of lack of space
  final_date_string3[length-2] = last_letter2;
  Serial.println(last_letter);
  Serial.println(last_letter2);
  Serial.println(final_date_string3[length-1]);
  Serial.println(final_date_string3[length-2]);
  
}


float getTemperature()
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

float getHumidity()
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

//get forecast
void getWeatherData() //client function to send/receive GET request data.
{
  String result ="";
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(servername, httpPort)) {
        return;
    }
      // We now create a URI for the request
    String url = "/data/2.5/forecast?id="+CityID+"&units=metric&cnt=1&APPID="+APIKEY;
   
       // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + servername + "\r\n" +
                 "Connection: close\r\n\r\n");
                 
    unsigned long timeout = millis();
    while (client.available() == 0) 
    {
        if (millis() - timeout > 5000)
        {
            client.stop();
            return;
        }
    }

    // Read all the lines of the reply from server
    while(client.available()) 
    {
        result = client.readStringUntil('\r');
    }

    result.replace('[', ' ');
    result.replace(']', ' ');

    char jsonArray [result.length()+1];
    result.toCharArray(jsonArray,sizeof(jsonArray));
    jsonArray[result.length() + 1] = '\0';
    
    StaticJsonBuffer<1024> json_buf;
    JsonObject &root = json_buf.parseObject(jsonArray);
    if (!root.success())
    {
      Serial.println("parseObject() failed");
    }
    
    String location = root["city"]["name"];
    String temperature = root["list"]["main"]["temp"];
    String weather = root["list"]["weather"]["main"];
    String description = root["list"]["weather"]["description"];
    String idString = root["list"]["weather"]["id"];
    String timeS = root["list"]["dt_txt"];
    
    weatherID = idString.toInt();
    Serial.print("\nWeatherID: ");
    Serial.print(weatherID);
    endNextionCommand(); //We need that in order the nextion to recognise the first command after the serial print
}


//get current weather
void getWeatheropenweather()
{
  String result2 ="";
  WiFiClient client2;
  const int httpPort2 = 80;
  if (!client2.connect(servername, httpPort2)) 
  {
      return;
  }

  String url2 = "/data/2.5/weather?id="+CityID+"&units=metric"+"&APPID="+APIKEY;
  client2.print(String("GET ") + url2 + " HTTP/1.1\r\n" +
                 "Host: " + servername + "\r\n" +
                 "Connection: close\r\n\r\n");
   unsigned long timeout2 = millis();
   while (client2.available() == 0) 
   {
      if (millis() - timeout2 > 5000) 
      {
          client2.stop();
          return;
      }
   }

    // Read all the lines of the reply from server
    while(client2.available()) 
    {
        result2 = client2.readStringUntil('\r');
    }

    result2.replace('[', ' ');
    result2.replace(']', ' ');

    char jsonArray2 [result2.length()+1];
    result2.toCharArray(jsonArray2,sizeof(jsonArray2));
    jsonArray2[result2.length() + 1] = '\0';
    StaticJsonBuffer<1024> json_buf2;
    JsonObject &root2 = json_buf2.parseObject(jsonArray2);
    
    String  humi = root2["main"]["humidity"]; // Extract local humidity now
    hum = humi.toInt();
    Serial.print("\nhumi: ");
    Serial.print(hum);
    
    String  temp = root2["main"]["temp"]; // Extract local temp now
    tempout = temp.toInt();
    Serial.print("\tempout: ");
    Serial.print(tempout);

    endNextionCommand();
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
   String command2 = "date.txt=\""+String(final_date_string+"/"+final_date_string2+"/"+"22")+"\"";
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
  String command = "temperatureout.txt=\""+String(tempout,1)+"\"";
  Serial.print(command);
  endNextionCommand();
}

void sendopenweatherHumidityToNextion()
{
  String command = "humidityout.txt=\""+String(hum,1)+"\"";
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



// HTTP server handling

void handle_OnConnect() {

  Temperature = dht.getTemperature(); // Gets the values of the temperature
  Humidity = dht.getHumidity(); // Gets the values of the humidity 

  if (!isnan(temperaturein))
  {
      server.send(200, "text/html", SendHTML(temperaturein,temperaturein,humidityin)); 
      Serial.print("Temperature:");
      Serial.println(dht.getTemperature());
  }
  if (!isnan(temperaturein))
  {
      server.send(200, "text/html", SendHTML(backuptemp,backuptemp,backuphumi)); 
      Serial.print("Temperature:");
      Serial.println(dht.getTemperature());    
  }
}

void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
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
