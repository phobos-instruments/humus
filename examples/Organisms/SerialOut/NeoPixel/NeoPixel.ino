#include <Adafruit_NeoPixel.h>

const int kRingPin = 6;
const int kPixels = 24;
const int kLineMax = 96;

Adafruit_NeoPixel ring(kPixels, kRingPin, NEO_GRB + NEO_KHZ800);

char line[kLineMax];
int lineLen = 0;

void setup() {
  Serial.begin(115200);
  ring.begin();
  ring.show();
}

float nextNumber(char** cursor) {
  char* end;
  float v = strtod(*cursor, &end);
  *cursor = end;
  return v;
}

int channel(float unit) {
  return (int) (constrain(unit, 0.0, 1.0) * 255.0 + 0.5);
}

uint32_t colourOf(float r, float g, float b) {
  return ring.Color(channel(r), channel(g), channel(b));
}

void runLine() {
  char* cursor = line + 1;
  switch (line[0]) {
    case 'P': {
      int i = (int) (nextNumber(&cursor) + 0.5);
      float r = nextNumber(&cursor), g = nextNumber(&cursor), b = nextNumber(&cursor);
      if (i >= 0 && i < kPixels) ring.setPixelColor(i, colourOf(r, g, b));
      break;
    }
    case 'F': {
      int n = (int) (nextNumber(&cursor) + 0.5);
      float r = nextNumber(&cursor), g = nextNumber(&cursor), b = nextNumber(&cursor);
      for (int i = 0; i < kPixels; i++) ring.setPixelColor(i, i < n ? colourOf(r, g, b) : 0);
      break;
    }
    case 'A': {
      float r = nextNumber(&cursor), g = nextNumber(&cursor), b = nextNumber(&cursor);
      ring.fill(colourOf(r, g, b));
      break;
    }
    case 'H': {
      float h = nextNumber(&cursor), s = nextNumber(&cursor), v = nextNumber(&cursor);
      ring.fill(ring.ColorHSV((uint16_t) (constrain(h, 0.0, 1.0) * 65535.0), channel(s), channel(v)));
      break;
    }
    case 'B':
      ring.setBrightness(channel(nextNumber(&cursor)));
      break;
    default:
      return;
  }
  ring.show();
}

void loop() {
  while (Serial.available()) {
    char c = (char) Serial.read();
    if (c == '\n' || c == '\r') {
      if (lineLen > 0) {
        line[lineLen] = 0;
        runLine();
      }
      lineLen = 0;
    } else if (lineLen < kLineMax - 1) {
      line[lineLen++] = c;
    }
  }
}
