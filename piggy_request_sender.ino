#include <SPI.h>
#include <MFRC522.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#define SS_PIN D8
#define RST_PIN D0
#define BUZZER_PIN D1  // Pino do buzzer

// Defina suas credenciais Wi-Fi
const char* ssid = "SSID";
const char* password = "PASSWORD";

// Definindo os pinos dos LEDs
#define LED_VERDE_PIN D2
#define LED_VERMELHO_PIN D3

// Inicializa o objeto RFID
MFRC522 rfid(SS_PIN, RST_PIN);

// Inicializa a chave de autenticação (padrão 0xFF)
MFRC522::MIFARE_Key key;

// Defina a URL de destino para enviar a requisição GET
const String baseURL = "http://piggywise.com.br:8080/piggies/PIGGY77/deposit?value=";

WiFiClient client;  // Criação do objeto WiFiClient

void setup() {
  Serial.begin(9600);
  SPI.begin();  // Inicializa o SPI
  rfid.PCD_Init();  // Inicializa o MFRC522

  // Configuração dos LEDs
  pinMode(LED_VERDE_PIN, OUTPUT);
  pinMode(LED_VERMELHO_PIN, OUTPUT);

  // Configura o pino do buzzer
  pinMode(BUZZER_PIN, OUTPUT);

  // Configura a chave de autenticação para todos os 6 bytes como 0xFF
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  // Inicia a conexão Wi-Fi
  WiFi.begin(ssid, password);  // Conecta ao Wi-Fi

  // Acende o LED vermelho e apaga o verde enquanto tenta conectar
  digitalWrite(LED_VERDE_PIN, LOW);
  digitalWrite(LED_VERMELHO_PIN, HIGH);
  Serial.print("Conectando-se à rede WiFi");
  
  // Espera até que o Wi-Fi esteja conectado
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  // Quando conectado, apaga o LED vermelho e acende o verde
  digitalWrite(LED_VERMELHO_PIN, LOW);
  digitalWrite(LED_VERDE_PIN, HIGH);
  
  Serial.println();
  Serial.println("Conectado ao WiFi!");
  Serial.print("IP do dispositivo: ");
  Serial.println(WiFi.localIP());
}

void playCashMachineSound() {
  // Frequências aproximadas de uma máquina de caixa eletrônico
  int tones[] = {600, 700, 800, 1000, 1200};
  int durations[] = {200, 150, 100, 200, 100};  // Duração de cada tom em milissegundos

  // Executa a sequência de sons
  for (int i = 0; i < 5; i++) {
    tone(BUZZER_PIN, tones[i]);  // Toca o tom no buzzer
    delay(durations[i]);         // Espera a duração do tom
    noTone(BUZZER_PIN);          // Para o som
    delay(50);                   // Intervalo entre os tons
  }
}

void loop() {
  // Verifica se um cartão foi apresentado
  if (!rfid.PICC_IsNewCardPresent()) {
    return;
  }

  // Lê os dados do cartão
  if (!rfid.PICC_ReadCardSerial()) {
    return;
  }

  // Exibe o tipo de cartão
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  Serial.print(F("Tipo de cartão: "));
  Serial.println(rfid.PICC_GetTypeName(piccType));

  // Verifica se o cartão é MIFARE Classic 1K
  if (piccType != MFRC522::PICC_TYPE_MIFARE_1K) {
    Serial.println(F("Este cartão não é MIFARE Classic 1K."));
    return;
  }

  // Tenta autenticar o bloco 4 (com a chave de autenticação padrão)
  byte block = 4;
  MFRC522::StatusCode status = rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(rfid.uid));

  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Falha na autenticação do bloco 4. Erro: "));
    Serial.println(rfid.GetStatusCodeName(status));
    return;
  }

  // Lê o conteúdo do bloco 4
  byte buffer[18];  // Buffer para armazenar o conteúdo lido
  byte size = sizeof(buffer);
  status = rfid.MIFARE_Read(block, buffer, &size);

  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Falha ao ler o bloco 4. Erro: "));
    Serial.println(rfid.GetStatusCodeName(status));
    return;
  }

  // O valor lido (exemplo: 32 que é 50 em decimal)
  int value = buffer[0];  // O valor está no primeiro byte do bloco
  Serial.print("Valor lido (hexadecimal): ");
  Serial.print(buffer[0], HEX);  // Exibe o valor em hexadecimal
  Serial.print(" (decimal): ");
  Serial.println(value);  // Exibe o valor em decimal

  // Envia a requisição GET com o valor lido
  String url = baseURL + String(value);
  Serial.print("Enviando requisição GET: ");
  Serial.println(url);

  HTTPClient http;  // Cria o objeto HTTPClient
  http.begin(client, url); 
  http.addHeader("Content-Type", "application/json");
  int httpCode = http.GET();  // Envia a requisição GET

  // Verifica o código de resposta HTTP
  if (httpCode > 0) {
    Serial.print("Código HTTP: ");
    Serial.println(httpCode);  // Exibe o código HTTP (200 significa sucesso)
    playCashMachineSound();    // Toca o som de caixa eletrônico em caso de sucesso
  } else {
    Serial.print("Falha na requisição. Erro: ");
    Serial.println(httpCode);
  }

  // Fecha a conexão HTTP
  http.end();

  // Finaliza a comunicação com o cartão
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();  // Aqui é a função correta

  delay(2000);  // Espera 2 segundos antes de ler o próximo cartão
}
