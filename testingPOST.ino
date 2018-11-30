#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <NTPtimeESP.h>
#include <ESP8266httpUpdate.h>

NTPtime NTPch("lt.pool.ntp.org"); 
strDateTime dateTime;

const int FW_VERSION = 1000;
const char* fwUrlBase = "http://kurmis.net/fota/esp8266";

const char* ssid = "BarclaysWiFi";

unsigned long lastMillis = 0;

//********** CHANGE PIN FUNCTION  TO GPIO **********
//GPIO 1 (TX) swap the pin to a GPIO.
pinMode(1, FUNCTION_3); 
//GPIO 3 (RX) swap the pin to a GPIO.
pinMode(3, FUNCTION_3); 
//**************************************************


// defines pins numbers
const int trigPin = 5;  //D1
const int powerPin = 15;  //D8
int echoPins[] = {   4, 13, 12, 14, 1, 3 };       // an array of ECHO pins {  D2, D7, D6, D5, TX, RX }

String host = "http://srb-middleware-dexter-lab.e4ff.pro-eu-west-1.openshiftapps.com";
String endPoint = "/v1/data";
//const char* json = "{\"id\":\"1\",\"mac\":\"00:00:00:00:00\",\"data\":\"-1\",\"time\":\"1994-03-09 00:00:00\"}";

void setup() {
 
  Serial.begin(115200);         //Serial connection
  WiFi.begin(ssid);             //WiFi connection

  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(powerPin, OUTPUT); // Sets the powerPin as an Output
  for (int pin = 0; pin < (sizeof(echoPins)/sizeof(int)); pin++) {
    pinMode(echoPins[pin], INPUT);
  }
  
  Serial.println("");
  while (WiFi.status() != WL_CONNECTED) {  //Wait for the WiFI connection completion
    Serial.print(".");
    delay(500);
  }
  Serial.println("WiFi connected");

  if(WiFi.status() == WL_CONNECTED){   //Check WiFi connection status
        
    //String mac = "aa:aa:aa:aa:aa:aa";
    String mac = getMacAddress();
    Serial.print("MAC: "); Serial.println(mac);
  
    //String date = "1994-03-09 00:00:00";
    String date = getTime();
    Serial.print("TIME: "); Serial.println(date);
    
    Serial.println("");
    Serial.println("");
     
    for (int pin = 0; pin < (sizeof(echoPins)/sizeof(int)); pin++) {
      
      int sensor = pin + 1;
      Serial.print("Sensor id: "); Serial.println(sensor);
        
      double distance = getDistance(echoPins[pin]);
      Serial.print("Distance: "); Serial.println(distance);
  
      POSTrequest(sensor, mac, distance, date);
    }
    
  // checkForUpdates();   //call to check for awailable updates. Doesn't work with BarclaysWiFi

  }  
  else 
  {
    Serial.println("Error in WiFi connection");   
  }
  Serial.println("Deep Sleep for 10 mins");
  ESP.deepSleep(600e6, WAKE_RF_DEFAULT); // 60e6 is 1 min

}
 
void loop() {}

void POSTrequest(int sensor, String mac, double distance, String date) {

  HTTPClient http;  //Declare object of class HTTPClient
     
  String json = "{\"id\":\"" + String(sensor) + "\",\"mac\":\"" + mac + "\",\"data\":\"" + String(distance)
  + "\",\"time\":\"" + date + "\"}";
  const char* jsonFinal = json.c_str();
   
  http.begin(host + endPoint);                          //Specify request destination
  http.addHeader("Content-Type", "application/json");   //Specify content-type header
   
  int request = http.POST((uint8_t *)jsonFinal, strlen(jsonFinal));    //Send the request
  delay(500);
  String response = http.getString();                                  //Get the response
     
  Serial.print("Request: "); Serial.println(jsonFinal);     //Print HTTP return code
  Serial.print("Response: ");  Serial.println(response);    //Print request response
   
  http.end();  //Close connection
}

String getMacAddress() {
  String myMac = "";
  byte mac[6]; 
  
  WiFi.macAddress(mac);
  for (int i = 0; i < 6; ++i) {
    myMac += String(mac[i], HEX);
    if (i < 5)
    myMac += ':';
  }

  return myMac;
}

double getDistance(int echoPin) {

  int count = 0;
  long duration;
  double distance = 0;
  double averageDistance = 0;

  digitalWrite(powerPin, HIGH); // powering up sensors
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  for(int i = 0; i < 5; i++) {
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
  
    //Reads the echoPin, returns the sound wave travel time in microseconds
    duration = pulseIn(echoPin, HIGH);
    distance = duration*0.034/2;
    
    if(distance < 200 && distance > 0) {
    averageDistance += distance;
    count++;
    }
    delay(100);
  } 
  digitalWrite(powerPin, LOW); // power down sensors
  return averageDistance/count;
}

String getTime() {
    String date = "1994-03-09 00:00:00";
    
    while(!dateTime.valid){
      dateTime = NTPch.getNTPtime(2, 1);
    } 
    
    byte hour = dateTime.hour;
    byte minute = dateTime.minute;
    byte second = dateTime.second;

    int year = dateTime.year;
    byte month = dateTime.month;
    byte day = dateTime.day;
    
    date = String(year)+"-"+String(month)+"-"+String(day)+" "+String(hour)+":"+String(minute)+":"+String(second);
    
    return date;
}

void checkForUpdates() {
  String fwURL = String( fwUrlBase );
  String fwVersionURL = fwURL;
  fwVersionURL.concat( ".version" );

  Serial.println( "Checking for firmware updates." );
  Serial.print( "Firmware version URL: " );
  Serial.println( fwVersionURL );

  HTTPClient httpClient;
  httpClient.begin( fwVersionURL );
  int httpCode = httpClient.GET();
  if( httpCode == 200 ) {
    String newFWVersion = httpClient.getString();

    Serial.print( "Current firmware version: " );
    Serial.println( FW_VERSION );
    Serial.print( "Available firmware version: " );
    Serial.println( newFWVersion );

    int newVersion = newFWVersion.toInt();

    if( newVersion > FW_VERSION ) {
      Serial.println( "Preparing to update" );

      String fwImageURL = fwURL;
      fwImageURL.concat( ".txt" );
      Serial.print( "Firmware URL: " );
      Serial.println( fwImageURL );
      t_httpUpdate_return ret = ESPhttpUpdate.update( fwImageURL );

      switch(ret) {
        case HTTP_UPDATE_FAILED:
          Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
          break;
      }
    }
    else {
      Serial.println( "Already on latest version" );
    }
  }
  else {
    Serial.print( "Firmware version check failed, got HTTP response code " );
    Serial.println( httpCode );
  }
  httpClient.end();
}

