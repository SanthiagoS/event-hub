/*
  RIW 2026 - C++ Hardware Experience
  Dispositivo: Arduino Uno
  Protocolo: TSX-HW/1.0

  Componentes:
  - LED 1  -> D4
  - LED 2  -> D5
  - LED 3  -> D6
  - Relé   -> D8
  - Buzzer -> D9

  Serial:
  - 115200 baud
  - Uma mensagem JSON por linha
*/

#include <Arduino.h>
#include <string.h>

// =====================================================
// PINOS
// =====================================================

constexpr uint8_t PIN_LED_1  = 4;
constexpr uint8_t PIN_LED_2  = 5;
constexpr uint8_t PIN_LED_3  = 6;
constexpr uint8_t PIN_RELAY  = 8;
constexpr uint8_t PIN_BUZZER = 9;

constexpr bool RELAY_ACTIVE_LOW = true;

// =====================================================
// INTERVALOS
// =====================================================

constexpr unsigned long STATUS_INTERVAL_MS = 3000;

// Durante os primeiros segundos, o Uno repete sua
// identificação para facilitar a captura pelo C++Builder.
constexpr unsigned long IDENTITY_RETRY_INTERVAL_MS = 2000;
constexpr unsigned long IDENTITY_RETRY_DURATION_MS = 10000;

// =====================================================
// ESTADOS
// =====================================================

bool led1State = false;
bool led2State = false;
bool led3State = false;
bool relayState = false;
bool buzzerState = false;

bool hostDetected = false;

unsigned long lastStatusTime = 0;
unsigned long lastIdentityTime = 0;

// =====================================================
// BUFFER SERIAL
// =====================================================

constexpr size_t SERIAL_BUFFER_SIZE = 192;

char serialBuffer[SERIAL_BUFFER_SIZE];
size_t serialBufferIndex = 0;

// =====================================================
// MEMÓRIA LIVRE
// =====================================================

int getFreeRam()
{
  extern int __heap_start;
  extern void* __brkval;

  int localVariable;

  return reinterpret_cast<int>(&localVariable) -
         reinterpret_cast<int>(
           __brkval == nullptr
             ? &__heap_start
             : __brkval
         );
}

// =====================================================
// CONTROLE DAS SAÍDAS
// =====================================================

void writeRelay(bool enabled)
{
  relayState = enabled;

  if (RELAY_ACTIVE_LOW)
  {
    digitalWrite(
      PIN_RELAY,
      enabled ? LOW : HIGH
    );
  }
  else
  {
    digitalWrite(
      PIN_RELAY,
      enabled ? HIGH : LOW
    );
  }
}

void writeBuzzer(bool enabled)
{
  buzzerState = enabled;

  if (enabled)
  {
    tone(PIN_BUZZER, 1200);
  }
  else
  {
    noTone(PIN_BUZZER);
    digitalWrite(PIN_BUZZER, LOW);
  }
}

bool setCapabilityState(
  const char* target,
  bool value)
{
  if (strcmp(target, "led1") == 0)
  {
    led1State = value;
    digitalWrite(
      PIN_LED_1,
      value ? HIGH : LOW
    );

    return true;
  }

  if (strcmp(target, "led2") == 0)
  {
    led2State = value;
    digitalWrite(
      PIN_LED_2,
      value ? HIGH : LOW
    );

    return true;
  }

  if (strcmp(target, "led3") == 0)
  {
    led3State = value;
    digitalWrite(
      PIN_LED_3,
      value ? HIGH : LOW
    );

    return true;
  }

  if (strcmp(target, "relay1") == 0)
  {
    writeRelay(value);
    return true;
  }

  if (strcmp(target, "buzzer1") == 0)
  {
    writeBuzzer(value);
    return true;
  }

  return false;
}

// =====================================================
// MENSAGENS JSON
// =====================================================

void sendBoot()
{
  Serial.println(
    F("{\"type\":\"boot\","
      "\"device\":\"TSX-UNO-001\","
      "\"message\":\"Arduino Uno iniciado\","
      "\"baud\":115200}")
  );
}

void sendIdentity()
{
  Serial.print(F("{\"type\":\"identity\","));

  Serial.print(F("\"device\":{"));
  Serial.print(F("\"name\":\"Arduino Uno\","));
  Serial.print(F("\"id\":\"TSX-UNO-001\","));
  Serial.print(F("\"category\":\"Digital IO\","));
  Serial.print(F("\"mcu\":\"ATmega328P\","));
  Serial.print(F("\"clock\":\"16 MHz\","));
  Serial.print(F("\"firmware\":\"2.1.0\","));
  Serial.print(F("\"protocol\":\"TSX-HW/1.0\""));
  Serial.print(F("},"));

  Serial.print(F("\"serial\":{"));
  Serial.print(F("\"baud\":115200"));
  Serial.print(F("},"));

  Serial.print(F("\"memory\":{"));
  Serial.print(F("\"flash\":\"32 KB\","));
  Serial.print(F("\"sram\":\"2 KB\","));
  Serial.print(F("\"eeprom\":\"1 KB\","));
  Serial.print(F("\"free_ram\":"));
  Serial.print(getFreeRam());
  Serial.print(F("},"));

  Serial.print(F("\"capabilities\":["));

  Serial.print(
    F("{\"id\":\"led1\","
      "\"type\":\"led\","
      "\"name\":\"LED Vermelho 1\","
      "\"pin\":4,"
      "\"state\":")
  );
  Serial.print(
    led1State ? F("true") : F("false")
  );
  Serial.print(F("},"));

  Serial.print(
    F("{\"id\":\"led2\","
      "\"type\":\"led\","
      "\"name\":\"LED Vermelho 2\","
      "\"pin\":5,"
      "\"state\":")
  );
  Serial.print(
    led2State ? F("true") : F("false")
  );
  Serial.print(F("},"));

  Serial.print(
    F("{\"id\":\"led3\","
      "\"type\":\"led\","
      "\"name\":\"LED Vermelho 3\","
      "\"pin\":6,"
      "\"state\":")
  );
  Serial.print(
    led3State ? F("true") : F("false")
  );
  Serial.print(F("},"));

  Serial.print(
    F("{\"id\":\"relay1\","
      "\"type\":\"relay\","
      "\"name\":\"Rele Principal\","
      "\"pin\":8,"
      "\"state\":")
  );
  Serial.print(
    relayState ? F("true") : F("false")
  );
  Serial.print(F("},"));

  Serial.print(
    F("{\"id\":\"buzzer1\","
      "\"type\":\"buzzer\","
      "\"name\":\"Buzzer\","
      "\"pin\":9,"
      "\"state\":")
  );
  Serial.print(
    buzzerState ? F("true") : F("false")
  );

  Serial.println(F("}]}"));
}

void sendStatus()
{
  Serial.print(F("{\"type\":\"status\","));
  Serial.print(F("\"device_id\":\"TSX-UNO-001\","));
  Serial.print(F("\"online\":true,"));

  Serial.print(F("\"uptime_ms\":"));
  Serial.print(millis());

  Serial.print(F(",\"free_ram\":"));
  Serial.print(getFreeRam());

  Serial.print(F(",\"capabilities\":["));

  Serial.print(F("{\"id\":\"led1\",\"state\":"));
  Serial.print(
    led1State ? F("true") : F("false")
  );
  Serial.print(F("},"));

  Serial.print(F("{\"id\":\"led2\",\"state\":"));
  Serial.print(
    led2State ? F("true") : F("false")
  );
  Serial.print(F("},"));

  Serial.print(F("{\"id\":\"led3\",\"state\":"));
  Serial.print(
    led3State ? F("true") : F("false")
  );
  Serial.print(F("},"));

  Serial.print(F("{\"id\":\"relay1\",\"state\":"));
  Serial.print(
    relayState ? F("true") : F("false")
  );
  Serial.print(F("},"));

  Serial.print(F("{\"id\":\"buzzer1\",\"state\":"));
  Serial.print(
    buzzerState ? F("true") : F("false")
  );

  Serial.println(F("}]}"));
}

void sendPong()
{
  Serial.print(
    F("{\"type\":\"pong\","
      "\"device\":\"TSX-UNO-001\","
      "\"uptime_ms\":")
  );

  Serial.print(millis());
  Serial.println(F("}"));
}

void sendResult(
  const char* target,
  bool value,
  bool success)
{
  Serial.print(
    F("{\"type\":\"result\","
      "\"target\":\"")
  );

  Serial.print(target);

  Serial.print(F("\",\"success\":"));
  Serial.print(
    success ? F("true") : F("false")
  );

  Serial.print(F(",\"value\":"));
  Serial.print(
    value ? F("true") : F("false")
  );

  Serial.println(F("}"));
}

void sendCommandReceived(const char* command)
{
  Serial.print(
    F("{\"type\":\"command_received\","
      "\"command\":\"")
  );

  Serial.print(command);

  Serial.println(F("\"}"));
}

void sendError(const char* message)
{
  Serial.print(
    F("{\"type\":\"error\","
      "\"message\":\"")
  );

  Serial.print(message);
  Serial.println(F("\"}"));
}

// =====================================================
// EXTRAÇÃO DO TARGET
// =====================================================

bool extractTarget(
  const char* json,
  char* target,
  size_t targetSize)
{
  const char* key =
    strstr(json, "\"target\"");

  if (key == nullptr)
  {
    return false;
  }

  const char* colon =
    strchr(key, ':');

  if (colon == nullptr)
  {
    return false;
  }

  const char* firstQuote =
    strchr(colon, '"');

  if (firstQuote == nullptr)
  {
    return false;
  }

  ++firstQuote;

  const char* secondQuote =
    strchr(firstQuote, '"');

  if (secondQuote == nullptr)
  {
    return false;
  }

  const size_t length =
    static_cast<size_t>(
      secondQuote - firstQuote
    );

  if (
    length == 0 ||
    length >= targetSize)
  {
    return false;
  }

  memcpy(
    target,
    firstQuote,
    length
  );

  target[length] = '\0';

  return true;
}

// =====================================================
// PROCESSAMENTO DOS COMANDOS
// =====================================================

void processCommand(const char* command)
{
  if (
    command == nullptr ||
    command[0] == '\0')
  {
    return;
  }

  sendCommandReceived(command);

  if (strcmp(command, "IDENTIFY") == 0)
  {
    hostDetected = true;
    sendIdentity();
    return;
  }

  if (strcmp(command, "STATUS") == 0)
  {
    hostDetected = true;
    sendStatus();
    return;
  }

  if (strcmp(command, "PING") == 0)
  {
    hostDetected = true;
    sendPong();
    return;
  }

  if (
    strstr(
      command,
      "\"command\":\"identify\""
    ) != nullptr)
  {
    hostDetected = true;
    sendIdentity();
    return;
  }

  if (
    strstr(
      command,
      "\"command\":\"status\""
    ) != nullptr)
  {
    hostDetected = true;
    sendStatus();
    return;
  }

  if (
    strstr(
      command,
      "\"command\":\"ping\""
    ) != nullptr)
  {
    hostDetected = true;
    sendPong();
    return;
  }

  if (
    strstr(
      command,
      "\"command\":\"set\""
    ) != nullptr)
  {
    char target[20];

    if (
      !extractTarget(
        command,
        target,
        sizeof(target)))
    {
      sendError(
        "Target ausente ou invalido"
      );

      return;
    }

    const bool valueIsTrue =
      strstr(
        command,
        "\"value\":true"
      ) != nullptr;

    const bool valueIsFalse =
      strstr(
        command,
        "\"value\":false"
      ) != nullptr;

    if (
      !valueIsTrue &&
      !valueIsFalse)
    {
      sendError(
        "Value deve ser true ou false"
      );

      return;
    }

    const bool value =
      valueIsTrue;

    const bool success =
      setCapabilityState(
        target,
        value
      );

    if (!success)
    {
      sendError(
        "Capability desconhecida"
      );

      return;
    }

    hostDetected = true;

    sendResult(
      target,
      value,
      true
    );

    return;
  }

  sendError("Comando desconhecido");
}

// =====================================================
// LEITURA SERIAL
// =====================================================

void readSerial()
{
  while (Serial.available() > 0)
  {
    const char received =
      static_cast<char>(
        Serial.read()
      );

    if (received == '\r')
    {
      continue;
    }

    if (received == '\n')
    {
      if (serialBufferIndex > 0)
      {
        serialBuffer[
          serialBufferIndex
        ] = '\0';

        processCommand(
          serialBuffer
        );

        serialBufferIndex = 0;
      }

      continue;
    }

    if (
      serialBufferIndex <
      SERIAL_BUFFER_SIZE - 1)
    {
      serialBuffer[
        serialBufferIndex++
      ] = received;
    }
    else
    {
      serialBufferIndex = 0;

      sendError(
        "Comando excedeu o limite"
      );
    }
  }
}

// =====================================================
// SETUP
// =====================================================

void setup()
{
  pinMode(PIN_LED_1, OUTPUT);
  pinMode(PIN_LED_2, OUTPUT);
  pinMode(PIN_LED_3, OUTPUT);
  pinMode(PIN_RELAY, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  digitalWrite(PIN_LED_1, LOW);
  digitalWrite(PIN_LED_2, LOW);
  digitalWrite(PIN_LED_3, LOW);

  writeRelay(false);
  writeBuzzer(false);

  Serial.begin(115200);

  // Aguarda a estabilização depois do reset causado
  // pela abertura da porta serial.
  delay(1200);

  sendBoot();
  sendIdentity();
  sendStatus();

  lastStatusTime = millis();
  lastIdentityTime = millis();
}

// =====================================================
// LOOP
// =====================================================

void loop()
{
  readSerial();

  const unsigned long now =
    millis();

  if (
    now - lastStatusTime >=
    STATUS_INTERVAL_MS)
  {
    lastStatusTime = now;
    sendStatus();
  }

  // Enquanto o C++Builder ainda não respondeu,
  // repete a identificação por alguns segundos.
  if (
    !hostDetected &&
    now <= IDENTITY_RETRY_DURATION_MS &&
    now - lastIdentityTime >=
      IDENTITY_RETRY_INTERVAL_MS)
  {
    lastIdentityTime = now;
    sendIdentity();
  }
}