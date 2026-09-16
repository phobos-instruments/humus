const unsigned long kPrintEveryMs = 50;

unsigned long lastPrint = 0;
float echo = 0.0;

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
}

void printReport() {
  float ramp = (millis() % 1000) / 1000.0;
  Serial.print(ramp, 3);
  Serial.print(' ');
  Serial.print(echo, 3);
  Serial.print(' ');
  Serial.println(analogRead(A0) / 1023.0, 3);
}

void readCommands() {
  if (!Serial.available()) return;
  echo = Serial.parseFloat();
  float light = Serial.parseFloat();
  digitalWrite(LED_BUILTIN, light > 0.5 ? HIGH : LOW);
  while (Serial.available() && Serial.read() != '\n') {
  }
}

void loop() {
  unsigned long now = millis();
  if (now - lastPrint >= kPrintEveryMs) {
    lastPrint = now;
    printReport();
  }
  readCommands();
}
