#include <Adafruit_NeoPixel.h>

#define PIN 5

Adafruit_NeoPixel light = Adafruit_NeoPixel(1, PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  // put your setup code here, to run once:
    // initialize LED strip
  Serial.begin(9600);
  light.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  light.setPixelColor(1,204,51,255);
  light.show();
  Serial.print("hello");
}
