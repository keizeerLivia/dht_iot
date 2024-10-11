#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "DHTesp.h"
#include "iot.h"
#include "senhas.h"
#include "saidas.h"

// Defina os pinos e parâmetros
#define DHTPIN 4          // Pino onde o DHT22 está conectado

// Definição dos tópicos de inscrição
#define mqtt_topic1 "projetoProfessor/dht1"
#define mqtt_topic2 "projetoProfessor/dht2"

// Definição do ID do cliente MQTT randomico
const String cliente_id = "ESP32Client" + String(random(0xffff), HEX);

// Definição dos dados de conexão
WiFiClient espClient;
PubSubClient client(espClient);
DHTesp dht;

// Variáveis para controle de tempo
unsigned long previousMillis = 0; 
const long interval = 10000; // Intervalo de 10 segundos para ler o DHT

// Protótipos das funções
void tratar_msg(char *topic, String msg);
void callback(char *topic, byte *payload, unsigned int length);
void reconecta_mqtt();
void inscricao_topicos();

// Inicia a conexão WiFi
void setup_wifi()
{
  Serial.println();
  Serial.print("Conectando-se à Rede WiFi ");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Conectado ao WiFi com sucesso com IP: ");
  Serial.println(WiFi.localIP());
}

// Inicia a conexão MQTT
void inicializa_mqtt()
{
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

// Atualiza a conexão MQTT
void atualiza_mqtt()
{
  client.loop();
  if (!client.connected())
  {
    reconecta_mqtt();
  }
}

// Função de callback chamada quando uma mensagem é recebida
void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.printf("Mensagem recebida [ %s ] \n\r", topic);
  String msg = "";
  for (int i = 0; i < length; i++)
  {
    msg += (char)payload[i];
  }
  tratar_msg(topic, msg);
}

// Função de reconexão ao Broker MQTT
void reconecta_mqtt()
{
  while (!client.connected())
  {
    Serial.print("Tentando se conectar ao Broker MQTT: ");
    Serial.println(mqtt_server);
    if (client.connect(cliente_id.c_str()))
    {
      Serial.println("Conectado ao Broker MQTT");
      inscricao_topicos();
    }
    else
    {
      Serial.println("Falha ao conectar ao Broker.");
      delay(2000);
    }
  }
}

// Inscreve nos tópicos MQTT
void inscricao_topicos()
{
  client.subscribe(mqtt_topic1);
  client.subscribe(mqtt_topic2);
}

// Trata as mensagens recebidas
void tratar_msg(char *topic, String msg)
{
  // Tratamento do tópico 1
  if (strcmp(topic, mqtt_topic1) == 0)
  {
    if (msg == "liga")
    {
      LedBuiltInState = true;
    }
    else if (msg == "desliga")
    {
      LedBuiltInState = false;
    }
    else if (msg == "alterna")
    {
      LedBuiltInState = !LedBuiltInState;
    }
  }
  // Tratamento do tópico 2
  else if (strcmp(topic, mqtt_topic2) == 0)
  {
    if (msg == "liga")
    {
      LedExternoState = true;
    }
    else if (msg == "desliga")
    {
      LedExternoState = false;
    }
    else if (msg == "alterna")
    {
      LedExternoState = !LedExternoState;
    }
  }
}

// Função para ler e publicar os dados do DHT
void dht_wifi()
{
  // Lê a umidade e temperatura
  float h = dht.getHumidity();
  float t = dht.getTemperature();

  // Verifica se a leitura falhou e tenta novamente
  if (isnan(h) || isnan(t))
  {
    Serial.println("Falha ao ler do DHT sensor!");
    return;
  }

  // Publica os dados nos tópicos desejados
  client.publish(mqtt_topic1, String(t).c_str()); // Publica temperatura
  client.publish(mqtt_topic2, String(h).c_str()); // Publica umidade
  Serial.printf("Temperatura: %.2f°C, Umidade: %.2f%%\n", t, h);
}

void setup() 
{
  Serial.begin(115200);
  dht.setup(DHTPIN, DHTesp::DHT22); // Inicializa o sensor DHT com o pino e tipo
  setup_wifi();
  inicializa_mqtt();
}

void loop() 
{
  atualiza_mqtt();
  
  // Verifica se o intervalo de 10 segundos passou
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) 
  {
    previousMillis = currentMillis;
    dht_wifi(); // Chama a função para ler e publicar dados do DHT
  }
}