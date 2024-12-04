#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN D8
#define RST_PIN D0

MFRC522 rfid(SS_PIN, RST_PIN);  // Instancia o objeto MFRC522 para comunicação SPI

MFRC522::MIFARE_Key key;  // Chave de autenticação padrão

// Valores que serão gravados nos cartões
int valores[] = {5, 10, 20, 50};  // Valores a serem gravados: 5, 10, 20, 50
int indice = 0;  // Índice para acessar os valores no array

void setup() {
  Serial.begin(9600);  // Inicia a comunicação serial
  SPI.begin();  // Inicia o SPI
  rfid.PCD_Init();  // Inicializa o leitor RFID
  
  // Configura a chave de autenticação para todos os 6 bytes como 0xFF
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  Serial.println(F("Aguardando para gravar nos cartões..."));
}

void loop() {
  // Verifica se um novo cartão foi colocado no leitor
  if (!rfid.PICC_IsNewCardPresent()) {
    return;  // Se não houver cartão, sai da função loop
  }

  // Lê o cartão
  if (!rfid.PICC_ReadCardSerial()) {
    return;  // Se não conseguir ler o cartão, sai da função loop
  }

  Serial.print(F("Cartão detectado. Tipo: "));
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  Serial.println(rfid.PICC_GetTypeName(piccType));

  // Verifica se o cartão é MIFARE Classic 1K
  if (piccType != MFRC522::PICC_TYPE_MIFARE_1K) {
    Serial.println(F("Este cartão não é MIFARE Classic 1K."));
    return;
  }

  // Autentica o bloco 4 com a chave padrão 0xFF
  byte block = 4;
  MFRC522::StatusCode status = rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(rfid.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Falha na autenticação do bloco 4. Erro: "));
    Serial.println(rfid.GetStatusCodeName(status));
    return;
  }

  // Prepara o valor a ser gravado (usando o índice no array de valores)
  byte buffer[16] = {0};  // Buffer de 16 bytes para armazenar os dados a serem gravados
  int value = valores[indice];
  
  // O valor vai ser gravado no primeiro byte do buffer
  buffer[0] = value;

  // Grava o valor no bloco 4 do cartão
  status = rfid.MIFARE_Write(block, buffer, 16);  // Escreve 16 bytes (tamanho do bloco)
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Falha ao gravar o bloco 4. Erro: "));
    Serial.println(rfid.GetStatusCodeName(status));
    return;
  }

  // Informa que o valor foi gravado com sucesso
  Serial.print(F("Valor "));
  Serial.print(value);
  Serial.println(F(" gravado no cartão."));

  // Passa para o próximo valor do array
  indice++;
  if (indice >= sizeof(valores) / sizeof(valores[0])) {
    indice = 0;  // Volta ao primeiro valor caso todos já tenham sido gravados
  }

  // Finaliza a comunicação com o cartão
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();  // Aqui é a função correta

  delay(2000);  // Espera 2 segundos antes de tentar ler e gravar no próximo cartão
}
