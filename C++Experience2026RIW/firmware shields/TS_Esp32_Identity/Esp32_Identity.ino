// ============================================================
// RIW2026 - ESP32-S3 N16R8
// Protocolo TSX-HW/1.0
//
// GPIO 4 -> Relé 1 - active LOW
// GPIO 5 -> Relé 2 - active LOW
// Temperatura -> Sensor interno do ESP32-S3
// GPIO 7 -> HC-SR501 - presença
// ============================================================

#include <Arduino.h>
#include <DHT.h>
#include <math.h>

// ------------------------------------------------------------
// Configuração geral
// ------------------------------------------------------------

// O navegador receberá um novo status a cada 1 segundo.
const unsigned long STATUS_INTERVAL =
  250;

const unsigned long TEMPERATURE_INTERVAL =
  2000;

// Mantém "presença detectada" por 3 segundos
// após o último sinal HIGH do PIR.
const unsigned long PRESENCE_HOLD_TIME =
  3000;

unsigned long lastStatusTime =
  0;

unsigned long lastTemperatureTime =
  0;  

unsigned long lastPresenceTime =
  0;

bool hostDetected =
  false;

// ------------------------------------------------------------
// GPIOs
// ------------------------------------------------------------

const int RELAY1_PIN =
  4;

const int RELAY2_PIN =
  5;

const int TEMPERATURE_PIN =
  6;

const int PRESENCE_PIN =
  7;

// ------------------------------------------------------------
// Relés active LOW
// ------------------------------------------------------------

const int RELAY_ON_LEVEL =
  LOW;

const int RELAY_OFF_LEVEL =
  HIGH;

//DHT
#define DHT_TYPE DHT11

DHT dht(
  TEMPERATURE_PIN,
  DHT_TYPE
);


// ------------------------------------------------------------
// Sensor PIR
//
// HC-SR501:
// HIGH = movimento detectado
// LOW  = nenhum movimento
// ------------------------------------------------------------

const int PRESENCE_ACTIVE_LEVEL =
  HIGH;

// ------------------------------------------------------------
// Estados atuais
// ------------------------------------------------------------

bool relay1State =
  false;

bool relay2State =
  false;

bool presenceState =
  false;

bool presenceArmed =
  true;  

float temperatureC =
  NAN;

// ------------------------------------------------------------
// Protótipos
// ------------------------------------------------------------

void readSerialCommand();

void processCommand(
  const char* command
);

void updatePresenceSensor();

void updateTemperatureSensor();

void sendBoot();

void sendIdentity();

void sendStatus();

void sendPong();

void sendCommandReceived(
  const char* command
);

void sendError(
  const char* message
);

// ------------------------------------------------------------
// Setup
// ------------------------------------------------------------

void setup()
{
  Serial.begin(
    115200
  );

  // --------------------------------------------------------
  // Relés
  // --------------------------------------------------------

  pinMode(
    RELAY1_PIN,
    OUTPUT
  );

  pinMode(
    RELAY2_PIN,
    OUTPUT
  );

  /*
   * Relé active LOW:
   *
   * HIGH = desligado
   * LOW  = ligado
   */
  digitalWrite(
    RELAY1_PIN,
    RELAY_OFF_LEVEL
  );

  digitalWrite(
    RELAY2_PIN,
    RELAY_OFF_LEVEL
  );

  relay1State =
    false;

  relay2State =
    false;

  //DHT
  dht.begin();  

  // --------------------------------------------------------
  // PIR HC-SR501
  // --------------------------------------------------------

  pinMode(
    PRESENCE_PIN,
    INPUT
  );

  /*
   * Atraso curto apenas para inicialização geral.
   *
   * O PIR pode levar mais tempo para estabilizar
   * fisicamente, mas isso não deve bloquear a
   * identificação da placa pelo C++Builder.
   */
  delay(
    1200
  );

  updatePresenceSensor();

  updateTemperatureSensor();

  sendBoot();
}

// ------------------------------------------------------------
// Loop
// ------------------------------------------------------------

void loop()
{
  readSerialCommand();

  /*
   * O PIR continua sendo verificado
   * continuamente.
   */
  updatePresenceSensor();

  const unsigned long now =
    millis();

  /*
   * O DHT11 é atualizado somente
   * a cada dois segundos.
   */
  if (
    now - lastTemperatureTime >=
      TEMPERATURE_INTERVAL
  )
  {
    lastTemperatureTime =
      now;

    updateTemperatureSensor();
  }

  /*
   * Presença, relés e última temperatura
   * são enviados quatro vezes por segundo.
   */
  if (
    hostDetected &&
    now - lastStatusTime >=
      STATUS_INTERVAL
  )
  {
    lastStatusTime =
      now;

    sendStatus();
  }

  delay(
    1
  );
}

// ------------------------------------------------------------
// Atualização da temperatura interna
// ------------------------------------------------------------
void updateTemperatureSensor()
{
  const float newTemperature =
    dht.readTemperature();

  if (
    !isnan(
      newTemperature
    )
  )
  {
    temperatureC =
      newTemperature;
  }
}

// ------------------------------------------------------------
// Atualização do PIR
// ------------------------------------------------------------

void updatePresenceSensor()
{
  const unsigned long now =
    millis();

  const bool pirDetected =
    digitalRead(
      PRESENCE_PIN
    ) ==
    PRESENCE_ACTIVE_LEVEL;

  // --------------------------------------------------------
  // Nova detecção
  // --------------------------------------------------------

  if (
    pirDetected &&
    presenceArmed
  )
  {
    presenceState =
      true;

    presenceArmed =
      false;

    lastPresenceTime =
      now;

    return;
  }

  // --------------------------------------------------------
  // Após 3 segundos, desliga a indicação
  // --------------------------------------------------------

  if (
    presenceState &&
    now - lastPresenceTime >=
      PRESENCE_HOLD_TIME
  )
  {
    presenceState =
      false;
  }

  // --------------------------------------------------------
  // Rearma somente quando o PIR voltar para LOW
  // --------------------------------------------------------

  if (
    !pirDetected
  )
  {
    presenceArmed =
      true;
  }
}

// ------------------------------------------------------------
// Leitura serial
// ------------------------------------------------------------

void readSerialCommand()
{
  static char commandBuffer[
    192
  ];

  static size_t commandLength =
    0;

  while (
    Serial.available() > 0
  )
  {
    const char received =
      static_cast<char>(
        Serial.read()
      );

    if (
      received == '\r'
    )
    {
      continue;
    }

    if (
      received == '\n'
    )
    {
      commandBuffer[
        commandLength
      ] = '\0';

      if (
        commandLength > 0
      )
      {
        processCommand(
          commandBuffer
        );
      }

      commandLength =
        0;

      commandBuffer[0] =
        '\0';

      continue;
    }

    if (
      commandLength <
      sizeof(
        commandBuffer
      ) - 1
    )
    {
      commandBuffer[
        commandLength
      ] = received;

      ++commandLength;
    }
    else
    {
      commandLength =
        0;

      commandBuffer[0] =
        '\0';

      sendError(
        "Buffer de comando excedido"
      );
    }
  }
}

// ------------------------------------------------------------
// Processamento dos comandos
// ------------------------------------------------------------

void processCommand(
  const char* command
)
{
  if (
    command == nullptr ||
    command[0] == '\0'
  )
  {
    return;
  }

  sendCommandReceived(
    command
  );

  // --------------------------------------------------------
  // IDENTIFY
  // --------------------------------------------------------

  if (
    strcmp(
      command,
      "IDENTIFY"
    ) == 0
  )
  {
    hostDetected =
      true;

    sendIdentity();

    return;
  }

  // --------------------------------------------------------
  // STATUS
  // --------------------------------------------------------

  if (
    strcmp(
      command,
      "STATUS"
    ) == 0
  )
  {
    hostDetected =
      true;

    updatePresenceSensor();

    updateTemperatureSensor();

    sendStatus();

    return;
  }

  // --------------------------------------------------------
  // PING
  // --------------------------------------------------------

  if (
    strcmp(
      command,
      "PING"
    ) == 0
  )
  {
    hostDetected =
      true;

    sendPong();

    return;
  }

  // --------------------------------------------------------
  // RELAY 1 ON
  // --------------------------------------------------------

  if (
    strcmp(
      command,
      "RELAY1_ON"
    ) == 0
  )
  {
    hostDetected =
      true;

    digitalWrite(
      RELAY1_PIN,
      RELAY_ON_LEVEL
    );

    relay1State =
      true;

    Serial.println(
      F(
        "{\"type\":\"relay\","
        "\"id\":\"relay1\","
        "\"state\":true}"
      )
    );

    return;
  }

  // --------------------------------------------------------
  // RELAY 1 OFF
  // --------------------------------------------------------

  if (
    strcmp(
      command,
      "RELAY1_OFF"
    ) == 0
  )
  {
    hostDetected =
      true;

    digitalWrite(
      RELAY1_PIN,
      RELAY_OFF_LEVEL
    );

    relay1State =
      false;

    Serial.println(
      F(
        "{\"type\":\"relay\","
        "\"id\":\"relay1\","
        "\"state\":false}"
      )
    );

    return;
  }

  // --------------------------------------------------------
  // RELAY 2 ON
  // --------------------------------------------------------

  if (
    strcmp(
      command,
      "RELAY2_ON"
    ) == 0
  )
  {
    hostDetected =
      true;

    digitalWrite(
      RELAY2_PIN,
      RELAY_ON_LEVEL
    );

    relay2State =
      true;

    Serial.println(
      F(
        "{\"type\":\"relay\","
        "\"id\":\"relay2\","
        "\"state\":true}"
      )
    );

    return;
  }

  // --------------------------------------------------------
  // RELAY 2 OFF
  // --------------------------------------------------------

  if (
    strcmp(
      command,
      "RELAY2_OFF"
    ) == 0
  )
  {
    hostDetected =
      true;

    digitalWrite(
      RELAY2_PIN,
      RELAY_OFF_LEVEL
    );

    relay2State =
      false;

    Serial.println(
      F(
        "{\"type\":\"relay\","
        "\"id\":\"relay2\","
        "\"state\":false}"
      )
    );

    return;
  }

  sendError(
    "Comando desconhecido"
  );
}

// ------------------------------------------------------------
// Boot
// ------------------------------------------------------------

void sendBoot()
{
  Serial.println(
    F(
      "{\"type\":\"boot\","
      "\"device\":\"TSX-ESP32-001\","
      "\"message\":\"ESP32-S3 iniciado\","
      "\"baud\":115200}"
    )
  );
}

// ------------------------------------------------------------
// Identity
// ------------------------------------------------------------

void sendIdentity()
{
  Serial.print(
    F(
      "{\"type\":\"identity\","
      "\"device\":{"
    )
  );

  Serial.print(
    F(
      "\"name\":\"ESP32-S3\","
      "\"id\":\"TSX-ESP32-001\","
      "\"category\":\"IoT Gateway\","
      "\"mcu\":\"ESP32-S3\","
      "\"clock\":\"240 MHz\","
      "\"firmware\":\"1.2.0\","
      "\"protocol\":\"TSX-HW/1.0\""
    )
  );

  Serial.print(
    F(
      "},"
      "\"serial\":{"
      "\"baud\":115200"
      "},"
      "\"memory\":{"
      "\"flash\":\"16 MB\","
      "\"psram\":\"8 MB\""
      "},"
      "\"capabilities\":["
      "{"
      "\"id\":\"relay1\","
      "\"type\":\"actuator\","
      "\"name\":\"Rele 1\","
      "\"mode\":\"digital\","
      "\"pin\":4,"
      "\"active_low\":true"
      "},"
      "{"
      "\"id\":\"relay2\","
      "\"type\":\"actuator\","
      "\"name\":\"Rele 2\","
      "\"mode\":\"digital\","
      "\"pin\":5,"
      "\"active_low\":true"
      "},"
      "{"
      "\"id\":\"temperature\","
      "\"type\":\"sensor\","
      "\"name\":\"Temperatura\","
      "\"sensor\":\"DHT11\","
      "\"unit\":\"C\","
      "\"pin\":6"
      "},"
      "{"
      "\"id\":\"presence\","
      "\"type\":\"sensor\","
      "\"name\":\"Presenca\","
      "\"sensor\":\"HC-SR501\","
      "\"mode\":\"digital\","
      "\"pin\":7,"
      "\"hold_ms\":3000"
      "}"
      "]"
    )
  );

  Serial.println(
    F("}")
  );
}

// ------------------------------------------------------------
// Status
// ------------------------------------------------------------

void sendStatus()
{
  Serial.print(
    F(
      "{\"type\":\"status\","
      "\"device_id\":\"TSX-ESP32-001\","
      "\"online\":true,"
      "\"uptime_ms\":"
    )
  );

  Serial.print(
    millis()
  );

  Serial.print(
    F(
      ",\"free_heap\":"
    )
  );

  Serial.print(
    ESP.getFreeHeap()
  );

  Serial.print(
    F(
      ",\"capabilities\":["
    )
  );

  // --------------------------------------------------------
  // Relé 1
  // --------------------------------------------------------

  Serial.print(
    F(
      "{"
      "\"id\":\"relay1\","
      "\"state\":"
    )
  );

  Serial.print(
    relay1State
      ? F("true")
      : F("false")
  );

  Serial.print(
    F("},")
  );

  // --------------------------------------------------------
  // Relé 2
  // --------------------------------------------------------

  Serial.print(
    F(
      "{"
      "\"id\":\"relay2\","
      "\"state\":"
    )
  );

  Serial.print(
    relay2State
      ? F("true")
      : F("false")
  );

  Serial.print(
    F("},")
  );

  // --------------------------------------------------------
  // Temperatura ambiente DHT11  // --------------------------------------------------------

  Serial.print(
    F(
      "{"
      "\"id\":\"temperature\","
      "\"value\":"
    )
  );

  if (
    isnan(
      temperatureC
    )
  )
  {
    Serial.print(
      F("null")
    );
  }
  else
  {
    Serial.print(
      temperatureC,
      1
    );
  }

  Serial.print(
    F(
      ",\"unit\":\"C\""
      "},"
    )
  );

  // --------------------------------------------------------
  // Presença HC-SR501
  // --------------------------------------------------------

  Serial.print(
    F(
      "{"
      "\"id\":\"presence\","
      "\"state\":"
    )
  );

  Serial.print(
    presenceState
      ? F("true")
      : F("false")
  );

  Serial.print(
    F("}")
  );

  Serial.println(
    F("]}")
  );
}

// ------------------------------------------------------------
// Pong
// ------------------------------------------------------------

void sendPong()
{
  Serial.print(
    F(
      "{\"type\":\"pong\","
      "\"device\":\"TSX-ESP32-001\","
      "\"uptime_ms\":"
    )
  );

  Serial.print(
    millis()
  );

  Serial.println(
    F("}")
  );
}

// ------------------------------------------------------------
// Command Received
// ------------------------------------------------------------

void sendCommandReceived(
  const char* command
)
{
  Serial.print(
    F(
      "{\"type\":\"command_received\","
      "\"command\":\""
    )
  );

  Serial.print(
    command
  );

  Serial.println(
    F("\"}")
  );
}

// ------------------------------------------------------------
// Error
// ------------------------------------------------------------

void sendError(
  const char* message
)
{
  Serial.print(
    F(
      "{\"type\":\"error\","
      "\"message\":\""
    )
  );

  Serial.print(
    message
  );

  Serial.println(
    F("\"}")
  );
}