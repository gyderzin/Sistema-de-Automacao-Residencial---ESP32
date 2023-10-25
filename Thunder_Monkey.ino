#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>


const char* baseUrl = "http://192.168.100.89/API-Mundo-Senai/public/api/circuitos";
const char* ssid = "OI FIBRA 4955";
const char* senha = "30140402asd*";

void setup() {
  Serial.begin(1200);

  WiFi.begin(ssid, senha);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando ao WiFi...");
  }
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(baseUrl);


    // Inicia a conexão HTTP GET

    int httpCode = http.GET();

    if (httpCode > 0) {
      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();

        // Crie um objeto JSON para analisar a resposta
        DynamicJsonDocument jsonDoc(1024);  // Tamanho máximo do documento JSON

        DeserializationError error = deserializeJson(jsonDoc, payload);
        if (!error) {
          // A análise foi bem-sucedida, agora você pode acessar as propriedades do objeto JSON
          JsonArray jsonArray = jsonDoc.as<JsonArray>();

          if (jsonArray.size() > 0) {
            JsonArray jsonArray = jsonDoc.as<JsonArray>();

            for (JsonObject jsonObject : jsonArray) {
              int porta = jsonObject["porta"];
              int estado = jsonObject["estado"];
              pinMode(porta, OUTPUT);
              if (estado == true) {
                digitalWrite(porta, HIGH);
              } else {
                digitalWrite(porta, LOW);
              }
            }

          } else {
            Serial.println("Array vazio no JSON.");
          }
        } else {
          Serial.println("Erro na análise do JSON.");
        }
      } else {
        Serial.printf("Erro na requisição HTTP: %d\n", httpCode);
      }
    } else {
      Serial.println("Não foi possível conectar à API.");
    }

    http.end();
  }
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(baseUrl);

    int httpCode = http.GET();

    if (httpCode > 0) {
      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();

        // Crie um objeto JSON para analisar a resposta
        DynamicJsonDocument jsonDoc(1024);  // Tamanho máximo do documento JSON

        DeserializationError error = deserializeJson(jsonDoc, payload);
        if (!error) {
          // A análise foi bem-sucedida, agora você pode acessar as propriedades do objeto JSON
          JsonArray jsonArray = jsonDoc.as<JsonArray>();

          if (jsonArray.size() > 0) {
            JsonArray jsonArray = jsonDoc.as<JsonArray>();

            for (JsonObject jsonObject : jsonArray) {
              int estado = jsonObject["estado"];
              int porta = jsonObject["porta"];
              if (estado == true) {
                digitalWrite(porta, HIGH);
              } else {
                digitalWrite(porta, LOW);
              }
            }

          } else {
            Serial.println("Array vazio no JSON.");
          }
        } else {
          Serial.println("Erro na análise do JSON.");
        }
      } else {
        Serial.printf("Erro na requisição HTTP: %d\n", httpCode);
      }
    } else {
      Serial.println("Não foi possível conectar à API.");
    }

    http.end();
  }

  delay(100);  //
}