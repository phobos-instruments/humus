const int kSwitchPin = 13;
const int kDimPin = 11;
const unsigned long kPrintEveryMs = 30;

unsigned long lastPrint = 0;

void setup() {
  Serial.begin(115200);
  pinMode(kSwitchPin, OUTPUT);
  pinMode(kDimPin, OUTPUT);
}

void printSensors() {
  float first = analogRead(A0) / 1023.0;
  float second = analogRead(A1) / 1023.0;
  Serial.print(first, 3);
  Serial.print(' ');
  Serial.println(second, 3);
}

void readCommands() {
  if (!Serial.available()) return;
  float gate = Serial.parseFloat();
  float level = Serial.parseFloat();
  digitalWrite(kSwitchPin, gate > 0.5 ? HIGH : LOW);
  analogWrite(kDimPin, (int) (constrain(level, 0.0, 1.0) * 255));
  while (Serial.available() && Serial.read() != '\n') {
  }
}

void loop() {
  unsigned long now = millis();
  if (now - lastPrint >= kPrintEveryMs) {
    lastPrint = now;
    printSensors();
  }
  readCommands();
}
