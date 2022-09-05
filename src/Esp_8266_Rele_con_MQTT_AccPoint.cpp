#include <Arduino.h>

#include <ESP8266WiFiMulti.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>

//*****************************
//******   ACCESS POINT  ******
//*****************************

#ifndef APSSID
#define APSSID "ESPap"
#define APPSK "thereisnospoon"
#endif

/* Set these to your desired credentials. */
const char *ssidAP = APSSID;
const char *passwordAP = APPSK;

WiFiServer server(80);

int config_AP = 0;

const String serial_number = "hMP5NmUVeHs4";

const char *ssid = "My_RED";
const char *password = "u2t4n6810";

//*****************************
//***   CONFIGURACION MQTT  ***
//*****************************

const char *mqtt_server = "mihome.ml";
const int mqtt_port = 1883;
const char *mqtt_user = "web_client";
const char *mqtt_pass = "121212";

const int relay = 0;

WiFiClient espClient;
PubSubClient client(espClient);

/*  Definicion de funciones*/
void setup_wifi();
void reconnect();
void callback(char *topic, byte *payload, unsigned int length);
void loop_AP();
void clear();

void setup()
{
  pinMode(relay, OUTPUT);
  digitalWrite(relay, HIGH);

  Serial.begin(115200);
  clear();

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop()
{
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();
}

void callback(char *topic, byte *payload, unsigned int length)
{

  String incoming = "";
  Serial.print("Mensaje recibido desde -> ");
  Serial.print(topic);
  Serial.println("");
  for (int i = 0; i < length; i++)
  {
    incoming += (char)payload[i];
  }
  incoming.trim();
  Serial.println("Mensaje -> " + incoming);

  String str_topic(topic);
  if (str_topic == serial_number + "/command")
  {
    if (incoming == "on")
    {
      digitalWrite(relay, LOW);
    }
    if (incoming == "off")
    {
      digitalWrite(relay, HIGH);
    }
  }
  Serial.println();
}

//*****************************
//***    CONEXION WIFI      ***
//*****************************
void setup_wifi()
{
  delay(1000);

  // Nos conectamos a nuestra red Wifi
  Serial.println();
  Serial.print("Conectando a ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  int temp = 0;

  while (WiFi.status() != WL_CONNECTED)
  {
    if (temp < 20)
    {
      delay(500);
      Serial.print(".");
    }
    else
    {
      loop_AP();
    }
    temp += 1;
  }

  Serial.println("");
  Serial.println("Conectado a red WiFi!");
  Serial.println("Dirección IP: ");
  Serial.println(WiFi.localIP());
}

void loop_AP()
{
  Serial.println();
  if (config_AP == 0)
  {
    Serial.print("Configuring access point...");
    /* You can remove the password parameter if you want the AP to be open. */
    WiFi.softAP(ssidAP, passwordAP);

    IPAddress myIP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(myIP);

    server.begin();
    Serial.println("HTTP server started");
    config_AP += 1;
  }
  int temp = 0;
  while (true)
  {
    if ((WiFi.status() == 3))
    {
      Serial.println("Volvio el internet");
      break;
      WiFi.softAPdisconnect(true);
    }
    WiFiClient clientAP = server.available();
    if (clientAP)
    {

      Serial.println("Nuevo cliente...");
      while (!clientAP.available())
      {
        delay(10);
        temp += 1;
        if ((WiFi.status() == 3))
        {

          break;
        }
      }

      String peticion = clientAP.readStringUntil('\r');
      Serial.println(peticion);
      clientAP.flush();

      if (peticion.indexOf('LED=ON') != -1)
      {
        Serial.println("Llego on");
        digitalWrite(relay, LOW);
      }
      else if (peticion.indexOf('LED=OFF') != -1)
      {
        Serial.println("Llego off");
        digitalWrite(relay, HIGH);
      }
      else if (peticion.indexOf('LED=MQ') != -1)
      {
        Serial.println("Llego mqtt");
        reconnect();
      }
      clientAP.println("HTTP/1.1 200 OK");
      clientAP.println("");
      clientAP.println("");
      clientAP.println("");
      clientAP.println("");

      //INICIA LA PAGINA

      clientAP.println("<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'>");
      clientAP.println("<meta name='viewport' content='width=device-width, initial-scale=1.0'>");
      clientAP.println("<title>Control de LED</title></head>");
      clientAP.println("<body style='font-family: Century gothic; width: 800;'><center>");
      clientAP.println("<div style='box-shadow: 0px 0px 20px 8px rgba(0,0,0,0.22); padding: 20px; width: 300px; display: inline-block; margin: 30px;'> ");
      clientAP.println("<h1>LED 1</h1>");

      if (digitalRead(relay) == 1)
      {
        clientAP.println("<h2>El led esta ENCENDIDO</h2>");
      }
      else
      {
        clientAP.println("<h2>El led esta APAGADO</h2>");
      }

      clientAP.println("<button style='background-color:red;  color:white; border-radius: 10px; border-color: rgb(255, 0, 0);' ");
      clientAP.println("type='button' onClick=location.href='/LED=OFF'><h2>Apagar</h2>");
      clientAP.println("</button> <button style='background-color:blue; color:white; border-radius: 10px; border-color: rgb(25, 255, 4);' ");
      clientAP.println("type='button' onClick=location.href='/LED=ON'><h2>Encender</h2>");
      clientAP.println("</button> <button style='background-color:black; color:white; border-radius: 10px; border-color: rgb(25, 255, 4);' ");
      clientAP.println("type='button' onClick=location.href='/MQTT=MQ'><h2>Encender</h2>");
      clientAP.println("</button></div></center></body></html>");

      //FIN DE LA PAGINA

      delay(10);
      Serial.println("Peticion finalizada");
      Serial.println("");
    }
  }
}

void reconnect()
{

  while (!client.connected())
  {
    if (WiFi.status() != 3)
    {
      setup_wifi();
    }
    Serial.print("Intentando conexión Mqtt...");
    // Creamos un cliente ID
    String clientId = "esp32_";
    clientId += String(random(0xffff), HEX);
    // Intentamos conectar
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass))
    {
      Serial.println("Conectado!");

      // Nos suscribimos a comandos
      char topic[25];
      String topic_aux = serial_number + "/command";
      topic_aux.toCharArray(topic, 25);
      client.subscribe(topic);

      // Nos suscribimos a username
      char topic2[25];
      String topic_aux2 = serial_number + "/user_name";
      topic_aux2.toCharArray(topic2, 25);
      client.subscribe(topic2);
    }
    else
    {
      Serial.print("falló :( con error -> ");
      Serial.print(client.state());
      Serial.println(" Intentamos de nuevo en 5 segundos");

      delay(5000);
    }
  }
}

void clear()
{
  Serial.write(27);
  Serial.print("[2J");
  Serial.write(27);
  Serial.print("[H");
}