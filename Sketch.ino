#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include "DHT.h"

// SENSORS
#define WATER_SENSOR A0 //Set water sensor pin.                                 | Ustaw pin detektora wody.
DHT dht(5, DHT11);      //Create temperature-humidity sensor with PIN and TYPE. | Stworz obiekt sensora temperatury i wilgotnosci z PINem i TYPem.
WiFiClientSecure client;//Create secure connection to server.                   | Stworz obiekt bezpiecznego polaczenia z serwerem.

// WI-FI SETUP
const char* ssid = "Best WiFi Ever";
const char* password = "adminadminadmin";

//REQUEST SETUP
const char* host = "host.com";                                        //<- Server domain                    | Domena serwera
const int httpsPort = 443;                                            //<- HTTPS port                       | port HTTPS
const char* fingerprint = "";                                         //<- SHA1 Certificate fingerprint     | Odcisk palca certyfikatu SHA1   *(use web browser to copy and paste. | uzyj przegladarki do skopiowania i wklejenia.)
const char* url = "/path/script.php";                                 //<- Url to PHP script                | Adres do skryptu PHP
const char*verify_key = "yousupersecretpassword";                     //<- Authentication key (protection)  | Klucz autoryzacyjny (ochrona)

void setup() {
  dht.begin();                  //<- Start temperature-humidity sensor.
  Serial.begin(9600);           //<- Start serial port.
  pinMode(WATER_SENSOR, INPUT); //<- Set Water Sensor pin to INPUT (Read).
  
  //Try to connect to WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
  //if (line.startsWith("{\"state\":\"success\"")) {
  
}
void loop() {

  int rain = analogRead(WATER_SENSOR);            //<- Read value from water sensor.                 | Odczytaj wartosc z czujnika wody.
  int humidity = dht.readHumidity();              //<- Read Humidity from sensor.                    | Odczytaj wilgotnosc z czujnika.
  int temperature = dht.readTemperature();        //<- Read temperature in Celcius from sensor.      | Odczytaj temperature w Celcjuszach z czujnika.
  //int temperature = dht.readTemperature(true);  //<- Read temperature in Fahrenheits from sensor.  | Odczytaj temperature w Fahrenheitach z czujnika.

  // Check if read is successfull, if no retry after 1 second | Sprawdz czy udalo sie przeczytac wartosc z czujnika, jesli nie sprobuj ponownie po 1 sekundzie.
  // That's why variables are declarated again in every loop course. | Dlatego zmienne deklarowane są co każdy przebieg pętli od nowa.
  if(isnan(humidity) || isnan(temperature)){
    Serial.println("Failed to read from DTH11 sensor! Retry after 1 second...");
    delay(1000);
    return;
  }

  //Info message to Serial. | Wiadomosc do portu szeregowego.
  Serial.println("Successfully read data from sensors:");
  Serial.println("temperature -> " + String(temperature));
  Serial.println("humidity -> " + String(humidity));
  Serial.println("rain -> " + String(rain));
  Serial.print("Connecting to ");
  Serial.println(host);
  
  //Try to connect to web server, if failed return to start. | Sprobuj polaczyc sie z serwerem web, jesli sie nie uda wroc na poczatek.
  if (!client.connect(host, httpsPort)){
    Serial.println("Connection to "+String(host)+"on port"+String(httpsPort)+" failed!");
    return;
  }
  
  //Verify if certificate match, if not, return to start. | Sprawdz czy certyfikat sie zgadza, jesli nie, wwroc na poczatek.
  if (client.verify(fingerprint, host)) Serial.println("certificate matches");
  else{
    Serial.println("certificate doesn't match");
    return;
  }

  //HTTP header that will be send to server | Naglowek HTTP ktory bedzie wyslany do serwera.
  String post_data = "key=" + String(verify_key) + "&temp=" + String(temperature) + "&hum=" + String(humidity) + "&rain="+String(rain); //<- Prepare POST data string. | Przygotowanie zmiennej z danymi wysłanymi metodą POST.
  
  client.println("POST https://" + String(host) + String(url) + " HTTP/1.1");     //<- web adress       | adres storny
  client.println("Host: "+String(host));                                          //<- host             | host
  client.println("Cache-Control: no-cache");                                      //<- Distable cache   | Dezaktywuj cache storny
  client.println("User-Agent: MSCWeatherStation 0.3");                            //<- Browser name     | Nazwa przeglądarki      *(Not required | Nie wymagana)
  client.println("Content-Length: " + String(post_data.length()));                //<- POST data length | Dlugosc danych POST
  client.println("Content-Type: application/x-www-form-urlencoded");              //<- POST request     | Rzadanie POST
  client.println();
  client.println(post_data);                                                      //<- Our POST data    | Nasze dane POST

  //Info message to Serial. | Wiadomosc do portu szeregowego.
  Serial.println("==================[ REQUEST SENT! ]==================");
  Serial.println("============[ RESPONDE WILL APPEAR BELOW ]===========");
  String responde_line = "empty!"; //<- declarate var to check if read all responde \r\n (header end) | Deklarowanie zmiennej do sprawdzania czy otrzymano cala odpowiedz \r\n (koniec naglowka).
  do{
    //Read responde per line to end. | Czytaj odpowedz liniami az do konca.
    while (client.available()) {
    responde_line = client.readStringUntil('\n'); //<- Read responde to \n (line break) symbol. | Czytaj odpowedz do znaku \n (przerwania linii).
    Serial.print("  --> ");
    Serial.print(responde_line);
    Serial.println();
    }
  }while(responde_line!="\r"); //<- Repeat until get and read responde. | Powtarzaj do poki nie otrzymano odpowiedzi i jej nie przeczytano do konca.
  Serial.println("=======================[ END! ]======================");
 
  delay(60000); // Repeat every minute. | Powtarzaj co minute.
}
