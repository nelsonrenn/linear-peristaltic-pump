#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

const int STEP_PIN = 2;
const int DIR_PIN = 3;
const int EN_PIN = 4;
const int BUTTON_PIN = 5;    
const int RATE_UP_PIN = 6;    
const int RATE_DOWN_PIN = 8; 
const int VOL_UP_PIN = 7;     
const int VOL_DOWN_PIN = 9;  

const float STEPS_PER_ML = 850;
const float RATE_MIN = 0.5, RATE_MAX = 60.0, RATE_STEP = 0.5; 
const int VOL_MIN = 1, VOL_MAX = 500, VOL_STEP = 1;           
float rate = 20;     
int target_volume = 10;  
bool running = false;

unsigned long stepCount = 0;    
unsigned long targetSteps = 0;   
unsigned long stepInterval = 0; 
unsigned long lastStepTime = 0;

char want[2][17];   
char shown[2][17];  

const int ADJ_PINS[4] = { RATE_UP_PIN, VOL_UP_PIN, RATE_DOWN_PIN, VOL_DOWN_PIN };
bool adjWas[4] = { false, false, false, false };

unsigned long adjSince[4] = { 0, 0, 0, 0 };
unsigned long adjRepeat[4] = { 0, 0, 0, 0 };

void recalc() {
  stepInterval = (unsigned long)(60000000.0 / (rate * STEPS_PER_ML));
  targetSteps = (unsigned long)(target_volume * STEPS_PER_ML);
}

bool finished() {
  return stepCount >= targetSteps;
}

void buildDisplay() {
  long r = lround(rate * 10);                        
  long v = (long)(stepCount * 10.0 / STEPS_PER_ML);
  const char* state = running ? "RUN" : (finished() ? "END" : "STP");
  snprintf(want[0], 17, "Rate:%3ld.%ldmL/min", r / 10, r % 10);
  snprintf(want[1], 17, "%4ld.%ld/%3dmL %s", v / 10, v % 10, target_volume, state);
}

void pushOneChar() {
  for (int row = 0; row < 2; row++) {
    for (int col = 0; col < 16; col++) {
      if (shown[row][col] != want[row][col]) {
        lcd.setCursor(col, row);
        lcd.write(want[row][col]);
        shown[row][col] = want[row][col];
        return;

      }
    }
  }
}

void changeRate(int dir) {
  rate = constrain(rate + dir * RATE_STEP, RATE_MIN, RATE_MAX);
  recalc();
}

void changeVolume(int dir) {
  target_volume = constrain(target_volume + dir * VOL_STEP, VOL_MIN, VOL_MAX);
  recalc();
}

bool clicked(int i) {
  unsigned long now = millis();
  bool pressed = digitalRead(ADJ_PINS[i]) == LOW;
  bool fire = false;
  if (pressed && !adjWas[i]) {
    fire = true;
    adjSince[i] = now;
    adjRepeat[i] = now;
  } else if (pressed && now - adjSince[i] >= 500) {
    unsigned long every = (now - adjSince[i] >= 3000) ? 20 : 80; 
    if (now - adjRepeat[i] >= every) {
      fire = true;
      adjRepeat[i] = now;
    }
  }
  adjWas[i] = pressed;
  return fire;
}

void checkButtons() {
  static bool lastStart = HIGH;
  static bool handled = false; 
  static unsigned long pressedAt = 0;
  unsigned long now = millis();
  bool start = digitalRead(BUTTON_PIN);
  if (start == LOW && lastStart == HIGH) { 
    pressedAt = now;
    handled = false;
    if (running) {  
      running = false;
      handled = true;
    }
  }
  if (start == LOW && !handled && now - pressedAt >= 1000) { 
    stepCount = 0;
    handled = true;
  }

  if (start == HIGH && lastStart == LOW && !handled) {  
    if (finished()) stepCount = 0;                     
    running = true;
    lastStepTime = micros();
  }

  lastStart = start;

  if (clicked(0)) changeRate(+1);    
  if (clicked(1)) changeVolume(+1); 
  if (clicked(2)) changeRate(-1);    
  if (clicked(3)) changeVolume(-1);  
}

void showRunning() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Running...");
  memset(shown, ' ', sizeof(shown));
  memcpy(shown[0], "Running...", 10);  
}

void setup() {
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(EN_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  for (int i = 0; i < 4; i++) pinMode(ADJ_PINS[i], INPUT_PULLUP);
  digitalWrite(EN_PIN, LOW);
  digitalWrite(DIR_PIN, HIGH);
  lcd.init();
  lcd.backlight();
  memset(shown, ' ', sizeof(shown)); 
  recalc();
  buildDisplay();

}

void loop() {
  static unsigned long lastButtons = 0, lastBuild = 0;
  if (running) {
    unsigned long now = micros();
    if (now - lastStepTime >= stepInterval) {
      lastStepTime += stepInterval;
      if (now - lastStepTime >= stepInterval) lastStepTime = now; 
      digitalWrite(STEP_PIN, HIGH);
      delayMicroseconds(5);
      digitalWrite(STEP_PIN, LOW);
      stepCount++;
      if (finished()) running = false;  
    }
  }

  unsigned long nowMs = millis();
  if (nowMs - lastButtons >= 20) {
    lastButtons = nowMs;
    checkButtons();
  }
  if (nowMs - lastBuild >= 100) {
    lastBuild = nowMs;
    buildDisplay();
  }
  static bool wasRunning = false;
if (running && !wasRunning) {
  showRunning();
}
if (!running) {
  pushOneChar();
}
wasRunning = running;
}