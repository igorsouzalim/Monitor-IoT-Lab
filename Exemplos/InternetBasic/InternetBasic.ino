#include <WiFi.h>

const char* ssid = "Manutencao";
const char* password = "eletronica@23";
const char* host = "www.google.com";
const int httpPort = 80;

void setup() {
  Serial.begin(115200);
  
  // Conectar-se à rede WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando ao WiFi...");
  }
  
  Serial.println("Conexão WiFi estabelecida!");
}

void loop() {
  // Criação de um objeto WiFiClient para fazer a requisição HTTP
  WiFiClient client;
  
  // Conexão ao servidor
  if (client.connect(host, httpPort)) {
    Serial.println("Conectado ao servidor!");
    
    // Envio da requisição HTTP GET
    client.print("GET / HTTP/1.1\r\n");
    client.print("Host: ");
    client.print(host);
    client.print("\r\n");
    client.print("Connection: close\r\n\r\n");
    delay(500); // Aguarda um breve período de tempo para os dados chegarem
    
    // Leitura da resposta HTTP
    while (client.available()) {
      String line = client.readStringUntil('\r');
      Serial.print(line);
    }
    
    Serial.println("\nResposta HTTP recebida!");
  }
  else {
    Serial.println("Falha na conexão ao servidor.");
  }
  
  // Aguarda um intervalo antes de fazer a próxima requisição
  delay(5000);
}
