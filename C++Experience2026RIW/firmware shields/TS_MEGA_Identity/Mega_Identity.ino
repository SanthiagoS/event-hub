// ============================================================
// RIW2026 - Arduino Mega 2560 + TFT LCD 2.4"
// Protocolo TSX-HW/1.0
//
// Funções atuais:
// - IDENTIFY
// - STATUS
// - PING
// - Exibir mensagem no TFT
// ============================================================

#include <MCUFRIEND_kbv.h>
#include <Adafruit_GFX.h>
#include <TouchScreen.h>

// ------------------------------------------------------------
// TFT
// ------------------------------------------------------------

MCUFRIEND_kbv tft;

// Cores RGB565
#define BLACK   0x0000
#define WHITE   0xFFFF
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define YELLOW  0xFFE0
#define ORANGE  0xFD20
#define GRAY    0x8410

const int XP = 8;
const int XM = A2;
const int YP = A3;
const int YM = 9;

const int TS_LEFT = 142;
const int TS_RT   = 908;
const int TS_TOP  = 148;
const int TS_BOT  = 906;

#define MINPRESSURE 200
#define MAXPRESSURE 1000

TouchScreen ts =
    TouchScreen(
        XP,
        YP,
        XM,
        YM,
        300
    );

// ------------------------------------------------------------
// Protocolo
// ------------------------------------------------------------

const unsigned long STATUS_INTERVAL = 3000;

unsigned long lastStatusTime = 0;
bool hostDetected = false;

//Game
bool gameRunning = false;

int gameScore = 0;

int targetX = 160;
int targetY = 120;

const int TARGET_RADIUS = 22;

bool signatureMode = false;
bool signatureDrawing = false;

bool photoSuccessScreen = false;

// ---------------------------------------------------------
// Preview de foto recebida pela Serial
// ---------------------------------------------------------

bool photoTransferMode = false;
bool photoPreviewScreen = false;

unsigned long photoPreviewStartTime = 0;

const unsigned long PHOTO_PREVIEW_DURATION =
  5000;

int photoSourceWidth = 80;
int photoSourceHeight = 60;

unsigned long photoSuccessStartTime = 0;
const unsigned long PHOTO_SUCCESS_DURATION = 5000;
const unsigned long SIGNATURE_SUCCESS_DURATION = 5000;


int signatureLastX = 0;
int signatureLastY = 0;

unsigned long signatureLastTouch = 0;

const unsigned long SIGNATURE_RELEASE_TIME = 150;

const unsigned long SIGNATURE_START_TIMEOUT =
  10000;

const unsigned long SIGNATURE_AUTO_FINISH_TIME =
  3000;

const unsigned long SIGNATURE_SUCCESS_TIME =
  2500;

unsigned long signatureSessionStartTime =
  0;

unsigned long signatureSuccessStartTime =
  0;

bool signatureHasContent =
  false;

bool signatureSuccessScreen =
  false;

// ------------------------------------------------------------
// Setup
// ------------------------------------------------------------

bool readTouchPoint(
  int& x,
  int& y)
{
  TSPoint p =
    ts.getPoint();

  pinMode(
    XM,
    OUTPUT
  );

  pinMode(
    YP,
    OUTPUT
  );

  if (
    p.z < MINPRESSURE ||
    p.z > MAXPRESSURE)
  {
    return false;
  }

  x =
    map(
      p.y,
      TS_TOP,
      TS_BOT,
      0,
      320
    );

  y =
    map(
      p.x,
      TS_RT,
      TS_LEFT,
      0,
      240
    );

  x =
    constrain(
      x,
      0,
      319
    );

  y =
    constrain(
      y,
      0,
      239
    );

  return true;
}




void setup()
{
  Serial.begin(115200);

  delay(1000);

  initializeDisplay();

  sendBoot();
}

// ------------------------------------------------------------
// Loop
// ------------------------------------------------------------

void loop()
{
  readSerialCommand();
  //processTouchGame();
  processSignatureTouch();

  // -------------------------------------------------------
  // Mensagem simples de foto salva
  // -------------------------------------------------------

  if (
    photoSuccessScreen &&
    (
      millis() -
      photoSuccessStartTime
    ) >=
      PHOTO_SUCCESS_DURATION
  )
  {
    photoSuccessScreen =
      false;

    showMegaHomeScreen();
  }


  // -------------------------------------------------------
  // Preview da foto recebida
  // -------------------------------------------------------

  if (
    photoPreviewScreen &&
    (
      millis() -
      photoPreviewStartTime
    ) >=
      PHOTO_PREVIEW_DURATION
  )
  {
    photoPreviewScreen =
      false;

    showMegaHomeScreen();
  }


  // -------------------------------------------------------
  // Sucesso da assinatura
  // -------------------------------------------------------

  if (
    signatureSuccessScreen &&
    (
      millis() -
      signatureSuccessStartTime
    ) >=
      SIGNATURE_SUCCESS_DURATION
  )
  {
    signatureSuccessScreen =
      false;

    showMegaHomeScreen();
  }


  // -------------------------------------------------------
  // Status periódico
  // -------------------------------------------------------

  if (
    !signatureMode &&
    !photoTransferMode &&
    millis() -
      lastStatusTime >=
        STATUS_INTERVAL
  )
  {
    lastStatusTime =
      millis();

    sendStatus();
  }

}

void showPhotoSuccess()
{
  photoSuccessScreen =
    true;

  photoSuccessStartTime =
    millis();

  tft.fillScreen(
    BLACK
  );

  int16_t x1;
  int16_t y1;

  uint16_t w;
  uint16_t h;


  // --------------------------------------------------
  // Título
  // --------------------------------------------------

  tft.setTextColor(
    GREEN
  );

  tft.setTextSize(
    2
  );

  const char* title =
    "FOTO REGISTRADA!";

  tft.getTextBounds(
    title,
    0,
    0,
    &x1,
    &y1,
    &w,
    &h
  );

  tft.setCursor(
    (tft.width() - w) / 2,
    65
  );

  tft.print(
    title
  );


  // --------------------------------------------------
  // Mensagem
  // --------------------------------------------------

  tft.setTextColor(
    WHITE
  );

  tft.setTextSize(
    1
  );

  const char* line1 =
    "Sua foto foi adicionada";

  tft.getTextBounds(
    line1,
    0,
    0,
    &x1,
    &y1,
    &w,
    &h
  );

  tft.setCursor(
    (tft.width() - w) / 2,
    115
  );

  tft.print(
    line1
  );


  const char* line2 =
    "ao mural do evento";

  tft.getTextBounds(
    line2,
    0,
    0,
    &x1,
    &y1,
    &w,
    &h
  );

  tft.setCursor(
    (tft.width() - w) / 2,
    135
  );

  tft.print(
    line2
  );


  // --------------------------------------------------
  // Rodapé
  // --------------------------------------------------

  tft.setTextColor(
    GRAY
  );

  const char* footer =
    "Obrigado por participar!";

  tft.getTextBounds(
    footer,
    0,
    0,
    &x1,
    &y1,
    &w,
    &h
  );

  tft.setCursor(
    (tft.width() - w) / 2,
    185
  );

  tft.print(
    footer
  );
}


void processTouchGame()
{
     if (!gameRunning)
    {
        return;
    }

    TSPoint tp =
        ts.getPoint();

    // Igual ao exemplo que funciona
    pinMode(XM, OUTPUT);
    pinMode(YP, OUTPUT);

    // Para o NOSSO sketch principal,
    // aceitamos a faixa de pressão observada nos testes.
    if (
        tp.z <= 50 ||
        tp.z >= 3000)
    {
        return;
    }

    const int xpos =
        map(
            tp.y,
            TS_TOP,
            TS_BOT,
            0,
            tft.width()
        );

    const int ypos =
        map(
            tp.x,
            TS_RT,
            TS_LEFT,
            0,
            tft.height()
        );

    Serial.print(F("[TEST] RAW X="));
    Serial.print(tp.x);

    Serial.print(F(" Y="));
    Serial.print(tp.y);

    Serial.print(F(" Z="));
    Serial.print(tp.z);

    Serial.print(F(" -> SCREEN X="));
    Serial.print(xpos);

    Serial.print(F(" Y="));
    Serial.println(ypos);

    // Evita desenhar fora da tela.
    if (
        xpos >= 0 &&
        xpos < tft.width() &&
        ypos >= 0 &&
        ypos < tft.height())
    {
        tft.fillCircle(
            xpos,
            ypos,
            4,
            YELLOW
        );
    }

    delay(30);
}

void sendGameEvent(
  const char* eventName)
{
  Serial.print(
    F(
      "{\"type\":\"game_event\","
      "\"event\":\""
    )
  );

  Serial.print(
    eventName
  );

  Serial.print(
    F(
      "\",\"score\":"
    )
  );

  Serial.print(
    gameScore
  );

  Serial.println(
    F("}")
  );
}

void sendGameScore()
{
  Serial.print(
    F(
      "{\"type\":\"game_score\","
      "\"score\":"
    )
  );

  Serial.print(
    gameScore
  );

  Serial.println(
    F("}")
  );
}

// ------------------------------------------------------------
// Inicialização do TFT
// ------------------------------------------------------------

void initializeDisplay()
{
  uint16_t id = tft.readID();

  Serial.print(F("[TFT] ID detectado: 0x"));
  Serial.println(id, HEX);

  // Como seu shield retornou 0xD3D3,
  // tentamos usar o valor detectado primeiro.
  tft.begin(id);

  tft.setRotation(1);

  tft.fillScreen(BLACK);

  drawStartupScreen();
}

// ------------------------------------------------------------
// Tela inicial
// ------------------------------------------------------------

void drawStartupScreen()
{
  tft.fillScreen(BLACK);

  tft.setTextColor(CYAN);
  tft.setTextSize(3);

  drawCenteredText(
    "RIW 2026",
    55,
    3,
    CYAN
  );

  drawCenteredText(
    "TSX - Arduino Mega",
    105,
    2,
    WHITE
  );

  drawCenteredText(
    "TFT Interactive Panel",
    145,
    2,
    GREEN
  );

  drawCenteredText(
    "Aguardando comando...",
    195,
    2,
    GRAY
  );
}

// ------------------------------------------------------------
// Centraliza texto no display
// ------------------------------------------------------------

void drawCenteredText(
  const char* text,
  int y,
  uint8_t textSize,
  uint16_t color)
{
  if (
    text == nullptr ||
    text[0] == '\0')
  {
    return;
  }

  tft.setTextSize(textSize);
  tft.setTextColor(color);

  int16_t x1;
  int16_t y1;

  uint16_t width;
  uint16_t height;

  tft.getTextBounds(
    text,
    0,
    y,
    &x1,
    &y1,
    &width,
    &height
  );

  int16_t x =
    (
      tft.width() -
      static_cast<int16_t>(width)
    ) / 2;

  if (x < 0)
  {
    x = 0;
  }

  tft.setCursor(
    x,
    y
  );

  tft.print(text);
}

void drawWrappedText(
  const char* text,
  int startY,
  uint8_t textSize,
  uint16_t color)
{
  if (
    text == nullptr ||
    text[0] == '\0')
  {
    return;
  }

  const int charWidth =
    6 * textSize;

  const int lineHeight =
    8 * textSize + 10;

  const int availableWidth =
    tft.width() - 40;

  const int maxCharsPerLine =
    availableWidth / charWidth;

  const int totalLength =
    strlen(text);

  // Se cabe em uma linha, desenha uma vez e sai.
  if (totalLength <= maxCharsPerLine)
  {
    drawCenteredText(
      text,
      startY,
      textSize,
      color
    );

    return;
  }

  char line[64];

  int position = 0;
  int y = startY;

  while (position < totalLength)
  {
    while (
      position < totalLength &&
      text[position] == ' ')
    {
      ++position;
    }

    if (position >= totalLength)
    {
      break;
    }

    int remaining =
      totalLength - position;

    int length =
      remaining > maxCharsPerLine
        ? maxCharsPerLine
        : remaining;

    // Se ainda sobra texto,
    // tenta quebrar no último espaço.
    if (
      position + length <
      totalLength)
    {
      int lastSpace = -1;

      for (
        int i = length;
        i > 0;
        --i)
      {
        if (
          text[position + i] ==
          ' ')
        {
          lastSpace = i;
          break;
        }
      }

      if (lastSpace > 0)
      {
        length =
          lastSpace;
      }
    }

    if (
      length >=
      static_cast<int>(
        sizeof(line)))
    {
      length =
        sizeof(line) - 1;
    }

    memcpy(
      line,
      text + position,
      length
    );

    line[length] =
      '\0';

    drawCenteredText(
      line,
      y,
      textSize,
      color
    );

    position +=
      length;

    while (
      position < totalLength &&
      text[position] == ' ')
    {
      ++position;
    }

    y +=
      lineHeight;

    if (
      y >
      tft.height() - 45)
    {
      break;
    }
  }
}

// ------------------------------------------------------------
// Tela de mensagem
// ------------------------------------------------------------

void showMessageOnDisplay(
  const char* message)
{
  tft.fillScreen(BLACK);

  // Título
  drawCenteredText(
    "MENSAGEM",
    25,
    2,
    CYAN
  );

  // Linha divisória
  tft.drawFastHLine(
    20,
    55,
    tft.width() - 40,
    BLUE
  );

  // Validação
  if (
    message == nullptr ||
    message[0] == '\0')
  {
    drawCenteredText(
      "Mensagem vazia",
      110,
      2,
      RED
    );

    return;
  }

  // Desenha SOMENTE UMA VEZ,
  // com quebra automática.
  drawWrappedText(
    message,
    80,
    2,
    WHITE
  );

  // Rodapé
  drawCenteredText(
    "RIW 2026",
    215,
    1,
    GREEN
  );
}

void showExpression(
  const char* expression)
{
  if (
    expression == nullptr ||
    expression[0] == '\0')
  {
    return;
  }

  if (
    strcmp(
      expression,
      "happy"
    ) == 0)
  {
    drawHappyFace();
    return;
  }

  if (
    strcmp(
      expression,
      "cool"
    ) == 0)
  {
    drawCoolFace();
    return;
  }

  if (
    strcmp(
      expression,
      "love"
    ) == 0)
  {
    drawLove();
    return;
  }

  if (
    strcmp(
      expression,
      "surprise"
    ) == 0)
  {
    drawSurpriseFace();
    return;
  }
}

void drawHappyFace()
{
  tft.fillScreen(BLACK);

  drawCenteredText(
    "FELIZ",
    15,
    2,
    CYAN
  );

  const int cx =
    tft.width() / 2;

  const int cy =
    125;

  // Rosto
  tft.fillCircle(
    cx,
    cy,
    70,
    YELLOW
  );

  // Olhos
  tft.fillCircle(
    cx - 25,
    cy - 20,
    8,
    BLACK
  );

  tft.fillCircle(
    cx + 25,
    cy - 20,
    8,
    BLACK
  );

  // Sorriso
  for (
    int r = 35;
    r <= 38;
    ++r)
  {
    tft.drawCircle(
      cx,
      cy + 5,
      r,
      BLACK
    );
  }

  // Apaga metade superior do círculo,
  // deixando apenas o sorriso.
  tft.fillRect(
    cx - 45,
    cy - 35,
    90,
    40,
    YELLOW
  );

  // Redesenha os olhos
  tft.fillCircle(
    cx - 25,
    cy - 20,
    8,
    BLACK
  );

  tft.fillCircle(
    cx + 25,
    cy - 20,
    8,
    BLACK
  );

  drawCenteredText(
    "RIW 2026",
    215,
    1,
    GREEN
  );
}

void drawCoolFace()
{
  tft.fillScreen(BLACK);

  drawCenteredText(
    "COOL",
    15,
    2,
    CYAN
  );

  const int cx =
    tft.width() / 2;

  const int cy =
    125;

  tft.fillCircle(
    cx,
    cy,
    70,
    YELLOW
  );

  // Óculos esquerdo
  tft.fillRoundRect(
    cx - 55,
    cy - 35,
    45,
    28,
    5,
    BLACK
  );

  // Óculos direito
  tft.fillRoundRect(
    cx + 10,
    cy - 35,
    45,
    28,
    5,
    BLACK
  );

  // Ponte dos óculos
  tft.fillRect(
    cx - 10,
    cy - 25,
    20,
    5,
    BLACK
  );

  // Hastes
  tft.drawFastHLine(
    cx - 70,
    cy - 27,
    15,
    BLACK
  );

  tft.drawFastHLine(
    cx + 55,
    cy - 27,
    15,
    BLACK
  );

  // Sorriso
  tft.drawFastHLine(
    cx - 25,
    cy + 30,
    50,
    BLACK
  );

  drawCenteredText(
    "RIW 2026",
    215,
    1,
    GREEN
  );
}

void drawLove()
{
  tft.fillScreen(BLACK);

  drawCenteredText(
    "LOVE",
    15,
    2,
    CYAN
  );

  const int cx =
    tft.width() / 2;

  const int cy =
    105;

  // Parte superior do coração
  tft.fillCircle(
    cx - 28,
    cy,
    32,
    RED
  );

  tft.fillCircle(
    cx + 28,
    cy,
    32,
    RED
  );

  // Parte inferior
  tft.fillTriangle(
    cx - 58,
    cy + 5,

    cx + 58,
    cy + 5,

    cx,
    cy + 85,

    RED
  );

  drawCenteredText(
    "LOVE RIW 2026",
    210,
    1,
    WHITE
  );
}

void drawSurpriseFace()
{
  tft.fillScreen(BLACK);

  drawCenteredText(
    "SURPRESA!",
    15,
    2,
    CYAN
  );

  const int cx =
    tft.width() / 2;

  const int cy =
    125;

  tft.fillCircle(
    cx,
    cy,
    70,
    YELLOW
  );

  // Olhos maiores
  tft.fillCircle(
    cx - 25,
    cy - 22,
    11,
    BLACK
  );

  tft.fillCircle(
    cx + 25,
    cy - 22,
    11,
    BLACK
  );

  // Boca surpresa
  tft.fillCircle(
    cx,
    cy + 30,
    18,
    BLACK
  );

  drawCenteredText(
    "RIW 2026",
    215,
    1,
    GREEN
  );
}

// ------------------------------------------------------------
// Leitura serial
// ------------------------------------------------------------

void readSerialCommand()
{
  static char commandBuffer[256];
  static size_t commandLength = 0;

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
      commandBuffer[commandLength] = '\0';

      if (commandLength > 0)
      {
        processCommand(
          commandBuffer
        );
      }

      commandLength = 0;
      commandBuffer[0] = '\0';

      continue;
    }

    if (
      commandLength <
      sizeof(commandBuffer) - 1)
    {
      commandBuffer[commandLength] =
        received;

      ++commandLength;
    }
    else
    {
      commandLength = 0;
      commandBuffer[0] = '\0';

      sendError(
        "Buffer de comando excedido"
      );
    }
  }
}
uint8_t hexToNibble(
  char value)
{
  if (
    value >= '0' &&
    value <= '9')
  {
    return value - '0';
  }

  if (
    value >= 'A' &&
    value <= 'F')
  {
    return value - 'A' + 10;
  }

  if (
    value >= 'a' &&
    value <= 'f')
  {
    return value - 'a' + 10;
  }

  return 0;
}

uint16_t hexToColor565(
  const char* value)
{
  uint16_t color = 0;

  for (
    int i = 0;
    i < 4;
    ++i)
  {
    color <<= 4;

    color |=
      hexToNibble(
        value[i]
      );
  }

  return color;
}

// ------------------------------------------------------------
// Processamento
// ------------------------------------------------------------
void processCommand(
  const char* command)
{
  if (
    command == nullptr ||
    command[0] == '\0')
  {
    return;
  }

  sendCommandReceived(
    command
  );


  // ---------------------------------------------------------
  // IDENTIFY
  // ---------------------------------------------------------

  if (
    strcmp(
      command,
      "IDENTIFY"
    ) == 0)
  {
    hostDetected = true;

    sendIdentity();

    return;
  }


  // ---------------------------------------------------------
  // STATUS
  // ---------------------------------------------------------

  if (
    strcmp(
      command,
      "STATUS"
    ) == 0)
  {
    hostDetected = true;

    sendStatus();

    return;
  }


  // ---------------------------------------------------------
  // PING
  // ---------------------------------------------------------

  if (
    strcmp(
      command,
      "PING"
    ) == 0)
  {
    hostDetected = true;

    sendPong();

    return;
  }


  // ---------------------------------------------------------
  // TFT_CLEAR
  // ---------------------------------------------------------

  if (
    strcmp(
      command,
      "TFT_CLEAR"
    ) == 0)
  {
    hostDetected = true;

    tft.fillScreen(
      BLACK
    );

    Serial.println(
      "{\"type\":\"tft\",\"action\":\"clear\",\"status\":\"ok\"}"
    );

    return;
  }


  // ---------------------------------------------------------
  // TFT_LINE
  // ---------------------------------------------------------

  if (
    strncmp(
      command,
      "TFT_LINE:",
      9
    ) == 0)
  {
    int x1 = 0;
    int y1 = 0;
    int x2 = 0;
    int y2 = 0;

    const int parsed =
      sscanf(
        command + 9,
        "%d,%d,%d,%d",
        &x1,
        &y1,
        &x2,
        &y2
      );

    if (parsed != 4)
    {
      sendError(
        "Coordenadas invalidas"
      );

      return;
    }

    hostDetected = true;

    tft.drawLine(
      x1,
      y1,
      x2,
      y2,
      WHITE
    );

    Serial.println(
      "{\"type\":\"tft\",\"action\":\"line\",\"status\":\"ok\"}"
    );

    return;
  }


  // ---------------------------------------------------------
  // SHOW MESSAGE
  // ---------------------------------------------------------

  if (
    strstr(
      command,
      "\"command\":\"show_message\""
    ) != nullptr)
  {
    char text[96];

    if (
      !extractJsonString(
        command,
        "\"text\":\"",
        text,
        sizeof(text)
      )
    )
    {
      sendError(
        "Texto ausente ou invalido"
      );

      return;
    }

    hostDetected = true;

    showMessageOnDisplay(
      text
    );

    sendDisplayResult(
      "message",
      text,
      true
    );

    return;
  }


  // ---------------------------------------------------------
  // SHOW EXPRESSION
  // ---------------------------------------------------------

  if (
    strstr(
      command,
      "\"command\":\"show_expression\""
    ) != nullptr)
  {
    char expression[20];

    if (
      !extractJsonString(
        command,
        "\"expression\":\"",
        expression,
        sizeof(expression)
      )
    )
    {
      sendError(
        "Expressao ausente ou invalida"
      );

      return;
    }

    hostDetected = true;

    showExpression(
      expression
    );

    sendDisplayResult(
      "expression",
      expression,
      true
    );

    return;
  }


  // ---------------------------------------------------------
  // START GAME
  // ---------------------------------------------------------

  if (
    strstr(
      command,
      "\"command\":\"start_game\""
    ) != nullptr)
  {
    hostDetected = true;

    startTapGame();

    sendDisplayResult(
      "game",
      "started",
      true
    );

    return;
  }


  // ---------------------------------------------------------
  // SIGNATURE_BEGIN
  // ---------------------------------------------------------

  if (
    strcmp(
      command,
      "SIGNATURE_BEGIN"
    ) == 0)
  {
    hostDetected = true;

    signatureMode = true;
    signatureDrawing = false;
    signatureHasContent = false;
    signatureSuccessScreen = false;

    signatureSessionStartTime =
      millis();

    tft.fillScreen(
      BLACK
    );

    Serial.println(
      "{\"type\":\"signature\",\"status\":\"ready\"}"
    );

    return;
  }


  // ---------------------------------------------------------
  // SIGNATURE_END
  // ---------------------------------------------------------

  if (
    strcmp(
      command,
      "SIGNATURE_END"
    ) == 0)
  {
    signatureMode = false;
    signatureDrawing = false;

    Serial.println(
      "{\"type\":\"signature_end\"}"
    );

    Serial.println(
      "{\"type\":\"signature\",\"status\":\"finished\"}"
    );

    showSignatureSuccess();

    return;
  }


  // ---------------------------------------------------------
  // PHOTO_SUCCESS
  // Mensagem simples de foto salva
  // ---------------------------------------------------------

  if (
    strcmp(
      command,
      "PHOTO_SUCCESS"
    ) == 0)
  {
    hostDetected = true;

    showPhotoSuccess();

    Serial.println(
      "{\"type\":\"photo\",\"status\":\"shown\"}"
    );

    return;
  }


  // ---------------------------------------------------------
  // PHOTO_BEGIN
  //
  // Exemplo:
  // PHOTO_BEGIN:80,60
  // ---------------------------------------------------------

  if (
    strncmp(
      command,
      "PHOTO_BEGIN:",
      12
    ) == 0)
  {
    int width = 0;
    int height = 0;

    const int parsed =
      sscanf(
        command + 12,
        "%d,%d",
        &width,
        &height
      );

    if (
      parsed != 2 ||
      width <= 0 ||
      height <= 0)
    {
      sendError(
        "Dimensoes da foto invalidas"
      );

      return;
    }

    // Para esta primeira versao,
    // aceitamos somente 80 x 60.
    if (
      width != 80 ||
      height != 60)
    {
      sendError(
        "Foto deve possuir 80x60"
      );

      return;
    }

    hostDetected = true;

    photoTransferMode =
      true;

    photoPreviewScreen =
      false;

    photoSourceWidth =
      width;

    photoSourceHeight =
      height;

    // Evita que a mensagem PHOTO_SUCCESS
    // interfira na foto real.
    photoSuccessScreen =
      false;

    tft.fillScreen(
      BLACK
    );

    Serial.println(
      "{\"type\":\"photo\",\"status\":\"receiving\"}"
    );

    return;
  }


  // ---------------------------------------------------------
  // PHOTO_DATA
  //
  // Formato:
  //
  // PHOTO_DATA:x,y:HEXHEXHEX...
  //
  // Cada pixel:
  // 4 caracteres HEX em RGB565.
  //
  // Exemplo:
  // PHOTO_DATA:0,0:F80007E0001FFFFF
  // ---------------------------------------------------------

  if (
    strncmp(
      command,
      "PHOTO_DATA:",
      11
    ) == 0)
  {
    if (!photoTransferMode)
    {
      sendError(
        "Transferencia de foto nao iniciada"
      );

      return;
    }

    const char* data =
      command + 11;

    int startX = 0;
    int startY = 0;

    const char* separator =
      strchr(
        data,
        ':'
      );

    if (
      separator == nullptr)
    {
      sendError(
        "PHOTO_DATA invalido"
      );

      return;
    }

    if (
      sscanf(
        data,
        "%d,%d",
        &startX,
        &startY
      ) != 2)
    {
      sendError(
        "Coordenadas PHOTO_DATA invalidas"
      );

      return;
    }

    if (
      startX < 0 ||
      startX >= photoSourceWidth ||
      startY < 0 ||
      startY >= photoSourceHeight)
    {
      sendError(
        "PHOTO_DATA fora da imagem"
      );

      return;
    }

    const char* pixels =
      separator + 1;

    const size_t hexLength =
      strlen(
        pixels
      );

    if (
      hexLength == 0 ||
      (hexLength % 4) != 0)
    {
      sendError(
        "Pixels PHOTO_DATA invalidos"
      );

      return;
    }

    const int pixelCount =
      hexLength / 4;

    // Evita ultrapassar a largura da imagem.
    if (
      startX +
      pixelCount >
      photoSourceWidth)
    {
      sendError(
        "PHOTO_DATA excede largura"
      );

      return;
    }


    // -------------------------------------------------------
    // Desenha cada pixel recebido
    //
    // Imagem fonte:
    // 80 x 60
    //
    // TFT:
    // 320 x 240
    //
    // 1 pixel fonte vira bloco 4 x 4.
    // -------------------------------------------------------

    for (
      int i = 0;
      i < pixelCount;
      ++i)
    {
      const char* pixelHex =
        pixels +
        (
          i * 4
        );

      const uint16_t color =
        hexToColor565(
          pixelHex
        );

      const int sourceX =
        startX + i;

      const int sourceY =
        startY;

      const int screenX =
        sourceX * 4;

      const int screenY =
        sourceY * 4;

      tft.fillRect(
        screenX,
        screenY,
        4,
        4,
        color
      );
    }

    return;
  }


  // ---------------------------------------------------------
  // PHOTO_END
  //
  // Finaliza a transferencia.
  // A imagem permanece no TFT durante 5 segundos.
  // ---------------------------------------------------------

  if (
    strcmp(
      command,
      "PHOTO_END"
    ) == 0)
  {
    if (!photoTransferMode)
    {
      sendError(
        "Nenhuma foto em transferencia"
      );

      return;
    }

    photoTransferMode =
      false;

    photoPreviewScreen =
      true;

    photoPreviewStartTime =
      millis();

    Serial.println(
      "{\"type\":\"photo\",\"status\":\"displayed\"}"
    );

    return;
  }


  // ---------------------------------------------------------
  // COMANDO DESCONHECIDO
  // Deve permanecer SEMPRE por ultimo.
  // ---------------------------------------------------------

  sendError(
    "Comando desconhecido"
  );
}


bool extractJsonString(
  const char* source,
  const char* key,
  char* output,
  size_t outputSize)
{
  if (
    source == nullptr ||
    key == nullptr ||
    output == nullptr ||
    outputSize == 0)
  {
    return false;
  }

  const char* start =
    strstr(
      source,
      key
    );

  if (start == nullptr)
  {
    return false;
  }

  start += strlen(key);

  const char* end =
    strchr(
      start,
      '"'
    );

  if (end == nullptr)
  {
    return false;
  }

  const size_t length =
    static_cast<size_t>(
      end - start
    );

  if (
    length == 0 ||
    length >= outputSize)
  {
    return false;
  }

  memcpy(
    output,
    start,
    length
  );

  output[length] = '\0';

  return true;
}

// ------------------------------------------------------------
// Boot
// ------------------------------------------------------------

void sendBoot()
{
  Serial.println(
    F(
      "{\"type\":\"boot\","
      "\"device\":\"TSX-MEGA-001\","
      "\"message\":\"Arduino Mega 2560 iniciado\","
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
      "\"name\":\"Arduino Mega 2560\","
      "\"id\":\"TSX-MEGA-001\","
      "\"category\":\"Interactive HMI\","
      "\"mcu\":\"ATmega2560\","
      "\"clock\":\"16 MHz\","
      "\"firmware\":\"1.1.0\","
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
      "\"flash\":\"256 KB\","
      "\"sram\":\"8 KB\","
      "\"eeprom\":\"4 KB\""
      "},"
      "\"capabilities\":["
    )
  );

  Serial.print(
    F(
      "{"
      "\"id\":\"tft24\","
      "\"type\":\"display\","
      "\"name\":\"TFT LCD 2.4\","
      "\"mode\":\"interactive\""
      "}"
    )
  );

  Serial.println(
    F(
      "]}"
    )
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
      "\"device_id\":\"TSX-MEGA-001\","
      "\"online\":true,"
      "\"uptime_ms\":"
    )
  );

  Serial.print(
    millis()
  );

  Serial.println(
    F(
      ",\"capabilities\":["
      "{"
      "\"id\":\"tft24\","
      "\"state\":true"
      "}"
      "]}"
    )
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
      "\"device\":\"TSX-MEGA-001\","
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
// Comando recebido
// ------------------------------------------------------------

void sendCommandReceived(
  const char* command)
{
  Serial.print(
    F(
      "{\"type\":\"command_received\","
      "\"command\":\""
    )
  );

  Serial.print(command);

  Serial.println(
    F("\"}")
  );
}

// ------------------------------------------------------------
// Resultado do display
// ------------------------------------------------------------

void sendDisplayResult(
  const char* mode,
  const char* value,
  bool success)
{
  Serial.print(
    F(
      "{\"type\":\"display_result\","
      "\"mode\":\""
    )
  );

  Serial.print(mode);

  Serial.print(
    F(
      "\",\"value\":\""
    )
  );

  Serial.print(value);

  Serial.print(
    F(
      "\",\"success\":"
    )
  );

  Serial.print(
    success
      ? F("true")
      : F("false")
  );

  Serial.println(
    F("}")
  );
}

void drawTapGame()
{
  tft.fillScreen(BLACK);

    drawCenteredText(
        "TOUCH TEST",
        10,
        2,
        CYAN
    );

    tft.drawFastHLine(
        15,
        38,
        tft.width() - 30,
        BLUE
    );

    gameScore = 0;
}

void drawGameScore()
{
  tft.fillRect(
    0,
    45,
    tft.width(),
    30,
    BLACK
  );

  tft.setTextColor(
    WHITE,
    BLACK
  );

  tft.setTextSize(2);

  tft.setCursor(
    15,
    50
  );

  tft.print(
    "SCORE: "
  );

  tft.print(
    gameScore
  );
}

void drawGameTarget()
{
  tft.fillCircle(
    targetX,
    targetY,
    TARGET_RADIUS,
    RED
  );

  tft.fillCircle(
    targetX,
    targetY,
    TARGET_RADIUS - 7,
    YELLOW
  );

  tft.fillCircle(
    targetX,
    targetY,
    5,
    RED
  );
}

void moveGameTarget()
{
  targetX =
    random(
      35,
      tft.width() - 35
    );

  targetY =
    random(
      95,
      tft.height() - 35
    );
}

void startTapGame()
{
  gameRunning = true;

  gameScore = 0;

  randomSeed(
    analogRead(A15)
  );

  moveGameTarget();

  drawTapGame();

  sendGameEvent(
    "started"
  );
}

bool Touch_getXY(
    int &pixelX,
    int &pixelY)
{
    TSPoint tp =
        ts.getPoint();

    // Exatamente como Touch_shield_new
    pinMode(XM, OUTPUT);
    pinMode(YP, OUTPUT);

    if (
        tp.z <= MINPRESSURE ||
        tp.z >= MAXPRESSURE)
    {
        return false;
    }

    // Orientation = 1
    pixelX =
        map(
            tp.y,
            TS_TOP,
            TS_BOT,
            0,
            tft.width()
        );

    pixelY =
        map(
            tp.x,
            TS_RT,
            TS_LEFT,
            0,
            tft.height()
        );

    return true;
}

// ------------------------------------------------------------
// Erro
// ------------------------------------------------------------

void sendError(
  const char* message)
{
  Serial.print(
    F(
      "{\"type\":\"error\","
      "\"message\":\""
    )
  );

  Serial.print(message);

  Serial.println(
    F("\"}")
  );
}
void processSignatureTouch()
{
  if (!signatureMode)
  {
    return;
  }

  int x = 0;
  int y = 0;

  const bool touched =
    readTouchPoint(
      x,
      y
    );

  if (touched)
  {
    signatureLastTouch =
      millis();

    if (!signatureDrawing)
    {
      signatureDrawing = true;

      signatureLastX = x;
      signatureLastY = y;

      Serial.println(
        "{\"type\":\"signature_start\"}"
      );

      return;
    }

    const int dx =
      abs(
        x - signatureLastX
      );

    const int dy =
      abs(
        y - signatureLastY
      );

    if (
      dx >= 3 ||
      dy >= 3)
    {
      signatureHasContent =
      true; 

      tft.drawLine(
        signatureLastX,
        signatureLastY,
        x,
        y,
        WHITE
      );

      Serial.print(
        "{\"type\":\"signature_line\",\"x1\":"
      );

      Serial.print(
        signatureLastX
      );

      Serial.print(
        ",\"y1\":"
      );

      Serial.print(
        signatureLastY
      );

      Serial.print(
        ",\"x2\":"
      );

      Serial.print(
        x
      );

      Serial.print(
        ",\"y2\":"
      );

      Serial.print(
        y
      );

      Serial.println(
        "}"
      );

      signatureLastX = x;
      signatureLastY = y;
    }

    return;
  }

  if (
    signatureDrawing &&
    millis() - signatureLastTouch >
      SIGNATURE_RELEASE_TIME)
  {
    signatureDrawing = false;
  }
  if (
  signatureMode &&
  signatureHasContent &&
  !signatureDrawing &&
  millis() - signatureLastTouch >=
    SIGNATURE_AUTO_FINISH_TIME)
{
  signatureMode = false;
  signatureHasContent = false;

  Serial.println(
    "{\"type\":\"signature_end\"}"
  );

  Serial.println(
    "{\"type\":\"signature\",\"status\":\"finished\"}"
  );

  showSignatureSuccess();

  signatureSuccessScreen =
    true;

  signatureSuccessStartTime =
    millis();

  return;
}
  if (
  signatureMode &&
  !signatureHasContent &&
  millis() - signatureSessionStartTime >=
    SIGNATURE_START_TIMEOUT)
{
  signatureMode = false;
  signatureDrawing = false;

  Serial.println(
    "{\"type\":\"signature\",\"status\":\"cancelled\",\"reason\":\"timeout\"}"
  );

  showMegaHomeScreen();

  return;
}

}

void showSignatureSuccess()
{
  signatureSuccessScreen =
    true;

  signatureSuccessStartTime =
    millis();

  tft.fillScreen(
    BLACK
  );

  int16_t x1;
  int16_t y1;

  uint16_t w;
  uint16_t h;


  // --------------------------------------------------
  // Título
  // --------------------------------------------------

  tft.setTextColor(
    GREEN
  );

  tft.setTextSize(
    2
  );

  const char* title =
    "ASSINATURA GRAVADA!";

  tft.getTextBounds(
    title,
    0,
    0,
    &x1,
    &y1,
    &w,
    &h
  );

  tft.setCursor(
    (tft.width() - w) / 2,
    65
  );

  tft.print(
    title
  );


  // --------------------------------------------------
  // Mensagem
  // --------------------------------------------------

  tft.setTextColor(
    WHITE
  );

  tft.setTextSize(
    1
  );

  const char* line1 =
    "Sua assinatura foi";

  tft.getTextBounds(
    line1,
    0,
    0,
    &x1,
    &y1,
    &w,
    &h
  );

  tft.setCursor(
    (tft.width() - w) / 2,
    115
  );

  tft.print(
    line1
  );


  const char* line2 =
    "adicionada ao mural";

  tft.getTextBounds(
    line2,
    0,
    0,
    &x1,
    &y1,
    &w,
    &h
  );

  tft.setCursor(
    (tft.width() - w) / 2,
    135
  );

  tft.print(
    line2
  );


  // --------------------------------------------------
  // Rodapé
  // --------------------------------------------------

  tft.setTextColor(
    GRAY
  );

  const char* footer =
    "Obrigado por participar!";

  tft.getTextBounds(
    footer,
    0,
    0,
    &x1,
    &y1,
    &w,
    &h
  );

  tft.setCursor(
    (tft.width() - w) / 2,
    185
  );

  tft.print(
    footer
  );
}


void showMegaHomeScreen()
{
  tft.fillScreen(
    BLACK
  );

  tft.setTextColor(
    CYAN
  );

  tft.setTextSize(
    2
  );

  const char* title =
    "C++ EXPERIENCE";

  int16_t x1;
  int16_t y1;

  uint16_t w;
  uint16_t h;

  tft.getTextBounds(
    title,
    0,
    0,
    &x1,
    &y1,
    &w,
    &h
  );

  tft.setCursor(
    (tft.width() - w) / 2,
    80
  );

  tft.print(
    title
  );

  tft.setTextColor(
    WHITE
  );

  tft.setTextSize(
    1
  );

  const char* line =
    "Selecione uma experiencia no painel";

  tft.getTextBounds(
    line,
    0,
    0,
    &x1,
    &y1,
    &w,
    &h
  );

  tft.setCursor(
    (tft.width() - w) / 2,
    125
  );

  tft.print(
    line
  );
}
