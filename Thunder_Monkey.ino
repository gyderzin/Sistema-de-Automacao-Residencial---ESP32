#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TimeLib.h>

const char* baseUrl = "http://192.168.100.87/API-ThuderMonkey/public/api";
const char* ssid = "OI FIBRA 4955";
const char* senha = "30140402asd*";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
HTTPClient http;  // Declare HTTPClient only once

String traduzirDia(String dia) {
  if (dia == "dom") return "Sun";
  if (dia == "seg") return "Mon";
  if (dia == "ter") return "Tue";
  if (dia == "qua") return "Wed";
  if (dia == "qui") return "Thu";
  if (dia == "sex") return "Fri";
  if (dia == "sab") return "Sat";
  return ""; // Retorno vazio se o dia não for reconhecido
}

void atualizarCircuitos(int id_dp, const String& circuitosString, int id_agendamento) {
    String url = String(baseUrl) + "/circuito/executar_comando/agendamento";

  // Crie um objeto JSON para enviar os dados
  DynamicJsonDocument jsonDoc(1024);
  jsonDoc["id_dp"] = id_dp;
  jsonDoc["circuitos"] = circuitosString;
  jsonDoc["id_agendamento"] = id_agendamento;

  // Serialize o JSON para uma String
  String jsonData;
  serializeJson(jsonDoc, jsonData);

  // Inicialize a requisição HTTP
  HTTPClient http;
  http.begin(url.c_str());

  // Configurar o cabeçalho para indicar que é uma requisição PUT e definir o tipo de conteúdo como JSON
  http.addHeader("Content-Type", "application/json");
  int httpCode = http.PUT(jsonData);

//  if (httpCode == HTTP_CODE_OK) {
//    Serial.println("Atualização bem-sucedida!");
//  } else {
//    Serial.printf("Erro na atualização - Código HTTP: %d\n", httpCode);
//  }

  // Fechar a conexão
  http.end();
}

void setup() {
  Serial.begin(1200);

  WiFi.begin(ssid, senha);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando ao WiFi...");
  }

  if (WiFi.status() == WL_CONNECTED) {
    String url = String(baseUrl) + "/circuito/recuperar_circuitos/1/esp32";
    http.begin(url.c_str());

    int httpCode = http.GET();

    if (httpCode > 0) {
      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();

        DynamicJsonDocument jsonDoc(1024);
        DeserializationError error = deserializeJson(jsonDoc, payload);

        if (!error) {
          JsonArray jsonArray = jsonDoc.as<JsonArray>();

          if (jsonArray.size() > 0) {
            for (JsonObject jsonObject : jsonArray) {
              Serial.println("entrou aqui");
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

  timeClient.setTimeOffset(-3 * 60 * 60);
  timeClient.begin();
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    String url = String(baseUrl) + "/circuito/recuperar_circuitos/1/esp32";
    http.begin(url.c_str());

    int httpCode = http.GET();

    if (httpCode > 0) {
      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();

        DynamicJsonDocument jsonDoc(1024);
        DeserializationError error = deserializeJson(jsonDoc, payload);

        if (!error) {
          JsonArray jsonArray = jsonDoc.as<JsonArray>();

          if (jsonArray.size() > 0) {
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

    timeClient.update();
    time_t rawTime = timeClient.getEpochTime();
    struct tm* timeInfo = localtime(&rawTime);

    // Formatar a data e hora como uma string
    char formattedTime[32];
    strftime(formattedTime, sizeof(formattedTime), "%d/%m/%Y %H:%M:%S %a", timeInfo);
    String data = formattedTime;

    // Obter hora e minuto
    char hora_minuto[6];
    strftime(hora_minuto, sizeof(hora_minuto), "%H:%M", timeInfo);
    String hora_str = hora_minuto;

    // Separar a string
    char *token = strtok(formattedTime, " ");

    // Variáveis para armazenar data, hora e dia da semana
    String data_str, dia_semana_str;

    // Loop para percorrer os tokens
    int count = 0;
    while (token != NULL) {
      if (count == 0) {
        data_str = token;
      } else if (count == 1) {
        // Usar a hora formatada
      } else if (count == 2) {
        dia_semana_str = token;
      }

      token = strtok(NULL, " ");
      count++;
    }

    String urlAgendamento = String(baseUrl) + "/agendamento/recuperar_agendamentos/1";
    http.begin(urlAgendamento.c_str());

    int httpCodeAgendamento = http.GET();

    if (httpCodeAgendamento == HTTP_CODE_OK) {
      String payloadAgendamento = http.getString();

      DynamicJsonDocument jsonDocAgendamento(2048);
      DeserializationError errorAgendamento = deserializeJson(jsonDocAgendamento, payloadAgendamento);

      if (!errorAgendamento) {
        JsonArray jsonArrayAgendamento = jsonDocAgendamento.as<JsonArray>();

        if (jsonArrayAgendamento.size() > 0) {
          for (JsonObject jsonObjectAgendamento : jsonArrayAgendamento) {
            // Obtenha valores do objeto principal
            int id_agendamento = jsonObjectAgendamento["id"];
            int id_dp = jsonObjectAgendamento["id_dp"];
            String hora = jsonObjectAgendamento["hora"];
            String intervalo_dias = jsonObjectAgendamento["intervalo_dias"];
            String dia_controle = jsonObjectAgendamento["dia_controle"];

            // Use strtok para dividir a string em tokens
            char dias[50]; // Ajuste o tamanho conforme necessário
            intervalo_dias.toCharArray(dias, 50);

            char *token = strtok(dias, ", ");
            while (token != NULL) {
              String dia_agendamento = traduzirDia(token);
              if (dia_agendamento == dia_semana_str) {
                if (hora == hora_str) {
                  if (dia_controle != dia_semana_str) {
                    String circuitosString = jsonObjectAgendamento["circuitos"];
                    atualizarCircuitos(id_dp, circuitosString, id_agendamento);
                    token = strtok(NULL, ", ");
                  } else {
                    token = strtok(NULL, ", ");
                  }
                } else {
                  token = strtok(NULL, ", ");
                }
              } else {
                token = strtok(NULL, ", ");
              }
            }
          }
        } else {
          Serial.println("Array vazio no JSON.");
        }
      } else {
        Serial.println("Erro na análise do JSON.");
      }
    } else {
      Serial.printf("Erro na requisição HTTP: %d\n", httpCodeAgendamento);
    }

    http.end();
  }

  delay(100);
}
