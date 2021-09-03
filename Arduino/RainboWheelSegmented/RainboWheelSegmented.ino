/*******************
* Program: RainboWheel
* Author:  Casey McGraw-Herdeg
* Inputs:  KY-040 Encoder knob (VCC, GND, DT [3], CLK [2])
* Outputs: LED addressible 'fairy lights' (VCC, GND, [5]) 
* Notes:   Rotates a rainbow pattern around the length of the LED string. 
*          Encoder wheel speeds up or changes direction of the fade.
*          
*          Customizable: 
*           - startLED: first LED in rainbow pattern
*           - total LEDs
*           - total Rainbows (uses click to adjust density)
*           - TBD: brightness
*******************/

// TODO: 5-clicks switches mode, between holding to adjust brightness or startPixel

#include <Encoder.h>
#include <Adafruit_NeoPixel.h>
#include <EncButton.h>

// Encoder Button - parse clicks
EncButton<EB_CALLBACK, 12, 13, 14> enc;   // энкодер с кнопкой <A, B, KEY>

// Basic encoder function for turns-- could simplify to only use EncButton
Encoder myEnc(12, 13);

// Which pin on the Arduino is connected to the NeoPixels?
#define LED_PIN    5

// How many NeoPixels are attached to the Arduino? (rainbow portion)
#define LED_COUNT 500

// Declare our NeoPixel strip object:
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// Internal and config vars
int wait = 100; 
long baseHue = 0;
int direction = 1;
int velocity = 1;
int vmax = 100;
float totalRainbows = .5;
int startPixel = 100;
int staticSaturation = 50;
long oldPosition = 0;

int brightness = 25;
int dimmerDirection = 1;
float dimmerFactor = .10; // Percent brighter or darker with each step
int maxBright = 50;
int minBright = 5;

int maxStartPixel = 100; // strip.numPixels();?

// WIP: modes for hold adjustment
int mode = 4;

void setup() {

  Serial.begin(9600);
  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(brightness); // Set BRIGHTNESS to about 1/5 (max = 255)


  // EncButton Setup
  enc.counter = 100;      // изменение счётчика
  //enc.setHoldTimeout(500);  // установка таймаута удержания кнопки
  enc.attach(HOLDED_HANDLER, myHolded);
  enc.attach(STEP_HANDLER, myStep);
  enc.attach(CLICKS_HANDLER, myClicks);
}

// Adjust rainbow density using click
void myClick() {
  totalRainbows /= 2;

  if(totalRainbows < .125){
    totalRainbows = 4; 
  }
}

// Switch dimmer direction when starting to hold
void myHolded() {
  dimmerDirection *= -1;
}

// Adjust brightness by holding button 
void myStep() {  
    Serial.print("Mode ");
    Serial.print(mode);
    Serial.print("step");

  // First mode: brightness
  if (mode < 5){
    // Linear Step
    brightness += dimmerDirection;
  
    // Proportional step
    if (dimmerDirection > 0){
      brightness += dimmerFactor * brightness;
    }
    else if (dimmerDirection < 0){
      brightness -= dimmerFactor * brightness;
    }
  
    if (brightness >= maxBright){
      brightness = maxBright;
      // Optionally switch directions and continue at peaks
      // dimmerDirection = -1;
    }
    else if (brightness <= minBright){
      brightness = minBright;
      // Optionally switch directions and continue at peaks
      // dimmerDirection = 1;
    }
    strip.setBrightness(brightness);
  
    // Logging
    Serial.print("\n");
    Serial.print("Brightness: ");
    Serial.print("\t");
    Serial.print(brightness);
    Serial.print("\t");
    Serial.print("dimmer: ");
    Serial.print("\t");
    Serial.print(dimmerDirection);
  }

  // Adjust startPixel
  else if (mode == 5){
    
    // Linear Step
    startPixel += dimmerDirection * 5;
  
    if (startPixel >= maxStartPixel){
      startPixel = maxStartPixel;
    }
    else if (startPixel <= 0){
      startPixel = 0;
    }

    // Logging
    Serial.print("\n");
    Serial.print("Dimmer: ");
    Serial.print("\t");
    Serial.print(dimmerDirection);
    Serial.print("\t");
    Serial.print("startPixel: ");
    Serial.print("\t");
    Serial.print(startPixel);
  }
  
}
void myClicks() {
  Serial.println(enc.clicks);

  if (enc.clicks < 2){
    myClick();
  }
  else if (enc.clicks == 4){
    mode = 4;
  }
  else if (enc.clicks == 5 ){
    mode = 5;
  }
  // TODO: Add mode 6 for num LEDs?
}

void loop() {

  // EncButton incrementing
  enc.tick();   // обработка всё равно здесь

  // Rainbow circuit
  for(int i=0; i< (strip.numPixels() - startPixel); i++) { // For each pixel in strip...

    // Color wheel has a range of 65536 Hues
    int pixelHue = baseHue + (i * 65536L / ((strip.numPixels() - startPixel) / totalRainbows));

    strip.setPixelColor(startPixel + i, strip.gamma32(strip.ColorHSV(pixelHue)));
  }

  // Check knob once per rotation
  long newPosition = myEnc.read();

//  // Logging
//  Serial.print("\n");
//  Serial.print("Encoder: ");
//  Serial.print("\t");
//  Serial.print(oldPosition);
//  Serial.print("\t");
//  Serial.print(newPosition);
//  Serial.print("\t");
//  Serial.print(newPosition - oldPosition);

  if (newPosition != oldPosition){
    velocity += (newPosition - oldPosition);
    oldPosition = newPosition;
  }

  if (velocity > 0){
    direction = 1;
  }
  else if(velocity < 0){
    direction = -1;
  }

  // Initial, less dynamic strand for scaffolding
  for (int i = 0; i < startPixel; i++){
    strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(baseHue, staticSaturation)));
  }
  
  strip.show(); // Update strip with new contents
  delay(wait);  // Pause for a moment
  
  // Apply gravity
  velocity -= (1 * direction);

  // Velocity min/max bounds
  if(-1 < velocity && velocity < 1){
    velocity = 1 * direction;
  }
  else if (velocity < (-1 * vmax)){
    velocity = (-1 * vmax);
  }
  else if (vmax < velocity){
    velocity = vmax;
  }

//  // Logging
//  Serial.print("\t\t");
//  Serial.print("Velocity: ");
//  Serial.print("\t");
//  Serial.print(velocity); 
//  Serial.print("\t\t");
//  Serial.print("Rainbows: ");
//  Serial.print(totalRainbows);

  baseHue += 256 * direction;
  baseHue += 256 * (velocity / 10);
}
