// --- LED pins ---
const int buzzerLEDs[] = {3,4,5,6,7};  // red, white, yellow, blue, green
const int controllerLEDs[] = {10,11};  // 0=RED, 1=GREEN

// --- Button pins ---
const int buzzerButtons[] = {A0,A1,A2,A3,A4};  // red, white, yellow, blue, green
const int controllerRedButton = 8;
const int controllerGreenButton = 9;

// --- Timing ---
const int sweepDelay = 200;
const int waitBeforeReverse = 500;
const int debounceDelay = 20;

// --- State ---
enum State {IDLE, GAME, LOCKED};
State currentState = IDLE;

// --- Variables ---
bool buzzerEliminated[5] = {false,false,false,false,false};
bool buzzerActive = false;
int lastPressedBuzzer = -1;
bool gameActive = false; // buzzers kan bara tryckas när spelet är aktivt
bool buzzerLocked[5] = {false,false,false,false,false}; // lås tills knapp släpps

const char* buzzerNames[] = {"RED","WHITE","YELLOW","BLUE","GREEN"};

void setup() {
  Serial.begin(9600);
  Serial.println("=== SYSTEM START (5 NO Buzzers) ===");

  // LED-pinnar
  for(int i=0;i<5;i++){
    pinMode(buzzerLEDs[i], OUTPUT);
    digitalWrite(buzzerLEDs[i], LOW);
  }

  // Controller LED
  for(int i=0;i<2;i++){
    pinMode(controllerLEDs[i], OUTPUT);
    digitalWrite(controllerLEDs[i], LOW);
  }

  // Buzzerknappar (NO)
  for(int i=0;i<5;i++){
    pinMode(buzzerButtons[i], INPUT_PULLUP);
  }

  // Controllerknappar (NO)
  pinMode(controllerRedButton, INPUT_PULLUP);
  pinMode(controllerGreenButton, INPUT_PULLUP);
}

void loop() {
  updateControllerLED();

  switch(currentState){
    case IDLE:
      idleMode();
      break;
    case GAME:
      gameMode();
      break;
    case LOCKED:
      break;
  }

  // --- Controller GREEN ---
  if(digitalRead(controllerGreenButton) == LOW){
    delay(debounceDelay);
    while(digitalRead(controllerGreenButton)==LOW);
    Serial.println("[CONTROLLER] GREEN pressed");

    if(currentState == GAME || currentState == LOCKED){
      Serial.println("[STATE] Returning to IDLE");
      resetBuzzers();
      currentState = IDLE;
    } else if(currentState == IDLE){
      Serial.println("[STATE] Starting GAME");
      currentState = GAME;
      lightAllActiveBuzzers();
      gameActive = true;

      // Lås alla buzzers som redan är nedtryckta (ska släppas först)
      for(int i=0;i<5;i++){
        buzzerLocked[i] = (digitalRead(buzzerButtons[i])==LOW);
      }
    }
  }

  // --- Controller RED ---
  if(digitalRead(controllerRedButton) == LOW){
    delay(debounceDelay);
    while(digitalRead(controllerRedButton)==LOW);
    Serial.println("[CONTROLLER] RED pressed");

    if(currentState == LOCKED && lastPressedBuzzer != -1){
      Serial.print("[ROUND] Eliminating buzzer: ");
      Serial.println(buzzerNames[lastPressedBuzzer]);

      buzzerEliminated[lastPressedBuzzer] = true;
      digitalWrite(buzzerLEDs[lastPressedBuzzer], LOW);
      lastPressedBuzzer = -1;

      // Tänd alla kvarvarande buzzers igen
      lightAllActiveBuzzers();
      currentState = GAME;

      // Lås alla buzzers tills knapparna släpps
      for(int i=0;i<5;i++){
        buzzerLocked[i] = true;
      }
    }
  }
}

// --- IDLE Mode ---
void idleMode(){
  if(buzzerActive) return;

  // Enkel animation i idle
  for(int i=0;i<5;i++){
    if(!buzzerEliminated[i]) digitalWrite(buzzerLEDs[i], HIGH);
    delay(sweepDelay);
    if(!buzzerEliminated[i]) digitalWrite(buzzerLEDs[i], LOW);
  }
  delay(waitBeforeReverse);
  for(int i=4;i>=0;i--){
    if(!buzzerEliminated[i]) digitalWrite(buzzerLEDs[i], HIGH);
    delay(sweepDelay);
    if(!buzzerEliminated[i]) digitalWrite(buzzerLEDs[i], LOW);
  }
  delay(waitBeforeReverse);

  checkBuzzerPress();
}

// --- GAME Mode ---
void gameMode(){
  checkBuzzerPress();
}

// --- Check buzzer presses ---
void checkBuzzerPress(){
  if(!gameActive) return; // ignorera tryck innan spelet startar

  for(int i=0;i<5;i++){
    if(buzzerEliminated[i]) continue;

    // Ignorera knapp om den fortfarande är låst
    if(buzzerLocked[i]){
      if(digitalRead(buzzerButtons[i])==HIGH){
        buzzerLocked[i] = false; // knapp återställd
      }
      continue;
    }

    if(digitalRead(buzzerButtons[i])==LOW){ // NO: tryckt = LOW
      delay(debounceDelay);
      if(digitalRead(buzzerButtons[i])==LOW){
        Serial.print("[BUZZER] Pressed: ");
        Serial.println(buzzerNames[i]);

        // Släck alla andra
        for(int j=0;j<5;j++){
          if(j != i && !buzzerEliminated[j]) digitalWrite(buzzerLEDs[j], LOW);
        }

        digitalWrite(buzzerLEDs[i], HIGH);
        buzzerActive = true;
        lastPressedBuzzer = i;
        currentState = LOCKED;

        // 🔴🟢 controller LED när någon svarar
        digitalWrite(controllerLEDs[0], HIGH);
        digitalWrite(controllerLEDs[1], HIGH);
        return;
      }
    }
  }
}

// --- Controller LED logik ---
void updateControllerLED(){
  if(currentState == IDLE){
    digitalWrite(controllerLEDs[1], HIGH); // Grön konstant
    digitalWrite(controllerLEDs[0], LOW);  // Röd av
  } 
  else if(currentState == GAME && !buzzerActive){
    // båda släckta innan någon tryckt
    digitalWrite(controllerLEDs[0], LOW);
    digitalWrite(controllerLEDs[1], LOW);
  }
}

// --- Helper functions ---
void lightAllActiveBuzzers(){
  for(int i=0;i<5;i++){
    digitalWrite(buzzerLEDs[i], buzzerEliminated[i] ? LOW : HIGH);
  }
  buzzerActive = false;
}

void resetBuzzers(){
  for(int i=0;i<5;i++){
    buzzerEliminated[i] = false;
    buzzerLocked[i] = false;
    digitalWrite(buzzerLEDs[i], LOW);
  }
  buzzerActive = false;
  lastPressedBuzzer = -1;
  gameActive = false; // ingen tryckning i IDLE
  digitalWrite(controllerLEDs[0], LOW);
  digitalWrite(controllerLEDs[1], HIGH); // grön konstant i IDLE
  Serial.println("[STATE] Reset to IDLE");
}
