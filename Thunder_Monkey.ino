#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ir_Gree.h>
#include <ir_LG.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TimeLib.h>
#include <vector>

const char* baseUrl = "https://apithundermonkey.com.br/api";
const char* ssid = "OI FIBRA 4955";
const char* senha = "30140402asd*";

const uint16_t kIrLed = 18; // Pino GPIO do ESP8266 a ser usado. Recomendado: 4 (D2).
const uint16_t kIrLedTVBOX = 19;
IRGreeAC acGree(kIrLed);
IRLgAc acLG(kIrLed);
IRsend irsend(kIrLed);
IRsend irsendTVBOX(kIrLedTVBOX);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
HTTPClient http;  // Declare HTTPClient only once

struct ControleAir {
  int id;
  String marca;
  bool estado;
  int temp;
  String mode;
  String vent;
};
std::vector<ControleAir> controlesAir;


struct ControleTV {
  int id;
  String marca;
  String comando;
  int controle;
};
std::vector<ControleTV> controlesTV;

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

    String urlAirC = String(baseUrl) + "/controll/recuperar_controles/1/esp32";
    http.begin(urlAirC.c_str());

    int httpCodeAirC = http.GET();

    if (httpCodeAirC > 0) {
      if (httpCodeAirC == HTTP_CODE_OK) {
        String payload = http.getString();

        DynamicJsonDocument jsonDoc(2048);
        DeserializationError error = deserializeJson(jsonDoc, payload);

        if (!error) {
          // Limpa o vetor antes de adicionar novos controles
          controlesAir.clear();
          controlesTV.clear();

          // Itera sobre todos os objetos no array de controles
          for (JsonVariant controleVariant : jsonDoc.as<JsonArray>()) {
            JsonObject controleObject = controleVariant.as<JsonObject>();

            // Acessando a marca do controle
            int id = controleObject["id"];
            String marcaControle = controleObject["marca"];
            String tipoControle = controleObject["tipo"];
            // Acessando a string JSON dentro de "controles"
            String controleString = controleObject["controles"];
            // Analisando a string JSON de controle
            DynamicJsonDocument controleDoc(1024);
            DeserializationError controleError = deserializeJson(controleDoc, controleString);

            if (!controleError) {
              // Criando um novo controle e atribuindo a marca
              if (tipoControle == "Air") {
                ControleAir novoControle;
                novoControle.id = id;
                novoControle.marca = marcaControle;
                novoControle.estado = controleDoc[0]["estado"].as<bool>();
                novoControle.temp = controleDoc[1]["temp"].as<int>();
                novoControle.mode = controleDoc[2]["mode"].as<String>();
                novoControle.vent = controleDoc[3]["vent"].as<String>();

                // Adicionando o novo controle ao vetor
                controlesAir.push_back(novoControle);

                if (marcaControle == "Gree") {
                  acGree.begin();
                  if (novoControle.mode == "kGreeCool") {
                    acGree.setMode(kGreeCool);
                  } else if (novoControle.mode == "kGreeHeat") {
                    acGree.setMode(kGreeHeat);
                  } else if (novoControle.mode == "kGreeFan") {
                    acGree.setMode(kGreeFan);
                  } else if (novoControle.mode == "kGreeAuto") {
                    acGree.setMode(kGreeAuto);
                  }
                  acGree.on();
                  acGree.setTemp(novoControle.temp);
                }
                else if (marcaControle == "LG") {
                  acLG.begin();
                  if (novoControle.mode == "kLgAcCool") {
                    acLG.setMode(kLgAcCool);
                  } else if (novoControle.mode == "kLgAcHeat") {
                    acLG.setMode(kLgAcHeat);
                  } else if (novoControle.mode == "kLgAcFan") {
                    acLG.setMode(kLgAcFan);
                  } else if (novoControle.mode == "kLgAcAuto") {
                    acLG.setMode(kLgAcAuto);
                  }
                  acLG.on();
                  acLG.setTemp(novoControle.temp);
                }
              }
              else if (tipoControle == "TV") {
                ControleTV novoControle;
                novoControle.id = id;
                novoControle.marca = marcaControle;
                novoControle.comando = controleDoc[0]["comando"].as<String>();
                novoControle.controle = controleDoc[1]["controle"].as<int>();

                controlesTV.push_back(novoControle);
              }

            } else {
              Serial.println("Erro na análise do JSON de controles.");
            }
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

  timeClient.setTimeOffset(-4 * 60 * 60);
  timeClient.begin();
  irsend.begin();
  irsendTVBOX.begin();
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

    String urlAirC = String(baseUrl) + "/controll/recuperar_controles/1/esp32";
    http.begin(urlAirC.c_str());

    int httpCodeAirC = http.GET();

    if (httpCodeAirC > 0) {
      if (httpCodeAirC == HTTP_CODE_OK) {
        String payload = http.getString();

        DynamicJsonDocument jsonDoc(1024);
        DeserializationError error = deserializeJson(jsonDoc, payload);

        if (!error) {
          JsonArray jsonArray = jsonDoc.as<JsonArray>();

          // Itera sobre cada objeto no array JSON
          for (JsonObject jsonObject : jsonArray) {
            // Extrai a string JSON contendo informações de controle
            String controleString = jsonObject["controles"];

            // Analisa a string JSON de controle
            DynamicJsonDocument controleDoc(1024);
            DeserializationError controleError = deserializeJson(controleDoc, controleString);
            String marcaDispositivo = jsonObject["marca"].as<String>();
            int id = jsonObject["id"].as<int>();
            String tipo = jsonObject["tipo"].as<String>();

            if (!controleError) {
              // Acessa os valores individuais de controle
              if (tipo == "Air") {
                bool estadoControle = controleDoc[0]["estado"].as<bool>();
                int tempControle = controleDoc[1]["temp"].as<int>();
                String modeControle = controleDoc[2]["mode"].as<String>();
                String ventControle = controleDoc[3]["vent"].as<String>();


                for (ControleAir& controle : controlesAir) {
                  // Acessa os valores individuais de controle
                  int idControleOld = controle.id;
                  String marcaControleOld = controle.marca;
                  bool estadoControleOld = controle.estado;
                  int tempControleOld = controle.temp;
                  String modeControleOld = controle.mode;
                  String ventControleOld = controle.vent;

                  if (id == idControleOld) {
                    // Verifica se houve alguma alteração nos valores de controle
                    if (estadoControle != estadoControleOld) {
                      // Faça algo se o estado do controle foi alterado
                      Serial.println("Estado alterado");
                      if (marcaDispositivo == "Gree") {
                        enviarGreeState("estado", estadoControle);
                      }
                      else if (marcaDispositivo == "LG") {
                        enviarLGState("estado", estadoControle);
                      }
                      controle.estado = estadoControle;
                    }

                    if (tempControle != tempControleOld) {
                      // Faça algo se a temperatura do controle foi alterada
                      Serial.println("Temperatura alterada");
                      if (marcaDispositivo == "Gree") {
                        enviarGreeTemp("temp", tempControle);
                      }
                      else if (marcaDispositivo == "LG") {
                        enviarLGTemp("estado", tempControle);
                      }
                      controle.temp = tempControle;
                    }

                    if (modeControle != modeControleOld) {
                      // Faça algo se o modo do controle foi alterado
                      Serial.println("Modo alterado");
                      if (marcaDispositivo == "Gree") {
                        enviarGreeModeAndVent("mode", modeControle);
                      }
                      else if (marcaDispositivo == "LG") {
                        enviarLGModeAndVent("estado", modeControle);
                      }
                      controle.mode = modeControle;
                    }

                    if (ventControle != ventControleOld) {
                      // Faça algo se a ventilação do controle foi alterada
                      Serial.println("Ventilação alterada");
                      if (marcaDispositivo == "Gree") {
                        enviarGreeModeAndVent("vent", ventControle);
                      }
                      else if (marcaDispositivo == "LG") {
                        enviarLGModeAndVent("vent", ventControle);
                      }
                      controle.vent = ventControle;
                    }
                  }
                }
              } else if (tipo == "TV") {
                String comandoControle = controleDoc[0]["comando"].as<String>();
                int controleControle = controleDoc[1]["controle"].as<int>();


                for (ControleTV& controle : controlesTV) {
                  // Acessa os valores individuais de controle
                  int idControleOld = controle.id;
                  String marcaControleOld = controle.marca;
                  String comandoOld = controle.comando;
                  int controleOld = controle.controle;
                  if (id == idControleOld) {
                    if (controleControle != controleOld) {
                      uint64_t codigoIR = strtoull(comandoControle.c_str(), NULL, 16);
                      if (marcaDispositivo == "Samsung") {
                        irsend.sendSAMSUNG(codigoIR);
                      } else if (marcaDispositivo == "LG") {
                        irsend.sendNEC(codigoIR);
                      } else if (marcaDispositivo == "TV Box") {
                        Serial.println(codigoIR, HEX);                                    
                        irsendTVBOX.sendNEC(codigoIR);
                      }
                      Serial.print("exec");
                      controle.controle = controleControle;
                    }
                  }
                }
              }

            } else {
              Serial.println("Erro na análise do JSON de controles.");
            }
          }
        } else {
          Serial.println("Erro na análise do JSON.");
        }
      } else {
        Serial.printf("Erro na requisição HTTP: %d\n", httpCodeAirC);
      }
    } else {
      Serial.println("Não foi possível conectar à API.");
    }
    http.end();
  }

  delay(100);
}


// Função para lidar com booleanos
void enviarGreeState(String envio, bool controle) {
  if (envio == "estado") {
    if (controle == true) {
      Serial.println("teste");
      acGree.on();
    } else {
      acGree.off();
    }
    acGree.send();
  }
}

void enviarLGState(String envio, bool controle) {
  if (envio == "estado") {
    if (controle == true) {
      acLG.on();
    } else {
      acLG.off();
    }
    acLG.send();
  }
}

// Função para lidar com strings
void enviarGreeModeAndVent(String envio, String controle) {
  if (envio == "mode") {
    // Mapeamento de strings para valores de modo
    if (controle == "kGreeCool") {
      acGree.setMode(kGreeCool);
    } else if (controle == "kGreeHeat") {
      acGree.setMode(kGreeHeat);
    } else if (controle == "kGreeFan") {
      acGree.setMode(kGreeFan);
    } else if (controle == "kGreeAuto") {
      acGree.setMode(kGreeAuto);
    }
  } else if (envio == "vent") {
    // Mapeamento de strings para valores de ventilação
    if (controle == "kGreeAuto") {
      acGree.setFan(kGreeAuto);
      Serial.print(acGree.getFan());
      Serial.print(controle);
    }
  }
  acGree.send();
}

void enviarLGModeAndVent(String envio, String controle) {
  if (envio == "mode") {
    // Mapeamento de strings para valores de modo
    if (controle == "kLgAcCool") {
      acLG.setMode(kLgAcCool);
      Serial.print(acLG.getMode());
      Serial.print(controle);
    } else if (controle == "kLgAcHeat") {
      acLG.setMode(kLgAcHeat);
    } else if (controle == "kLgAcFan") {
      acLG.setMode(kLgAcFan);
    } else if (controle == "kLgAcAuto") {
      acLG.setMode(kLgAcAuto);
    }
  } else if (envio == "vent") {
    // Mapeamento de strings para valores de ventilação
    if (controle == "kLgAcAuto") {
      acLG.setFan(kLgAcAuto);
      Serial.print(acGree.getFan());
      Serial.print(controle);
    }
  }
  acLG.send();
}

// Função para lidar com inteiros
void enviarGreeTemp(String envio, int controle) {
  if (envio == "temp") {
    acGree.setTemp(controle);
    Serial.println(acGree.getTemp());
    Serial.println(controle);
  }
  acGree.send();
}

void enviarLGTemp(String envio, int controle) {
  if (envio == "temp") {
    acLG.setTemp(controle);
    Serial.println(acLG.getTemp());
    Serial.println(controle);
  }
  acLG.send();
}

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

void atualizarCircuitos(int id_dp, const String & circuitosString, int id_agendamento) {
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
  http.addHeader("Content-Type", "application/json");
  int httpCode = http.PUT(jsonData);
  http.end();
}
