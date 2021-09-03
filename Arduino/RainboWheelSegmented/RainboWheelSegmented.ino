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
int LEDcount = strip.numPixels();
long oldPosition = 0;
bool logging = false;

int brightness = 20;
int dimmerDirection = 1;
float dimmerFactor = .10; // Percent brighter or darker with each step
int maxBright = 50;
int minBright = 5;


// Todo: switch modes to Enum type for readability
int mode = 1;

void setup() {

  Serial.begin(9600);
  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(brightness); // Set BRIGHTNESS to about 1/5 (max = 255)


  // EncButton Setup
  enc.counter = 100;      // изменение счётчика
  enc.attach(HOLDED_HANDLER, reverseDimmer);
  enc.attach(STEP_HANDLER, handleStep);
  enc.attach(CLICKS_HANDLER, handleClicks);
}

// Adjust rainbow density using click
void setDensity() {
  totalRainbows /= 2;

  if(totalRainbows < .125){
    totalRainbows = 4; 
  }
}

// Switch dimmer direction when starting to hold
void reverseDimmer() {
  dimmerDirection *= -1;
}

// Adjust brightness by holding button 
void handleStep() {  

  // First mode: brightness
  if (mode == 1){
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
  
    if(logging){
      Serial.print("\n");
      Serial.print(dimmerDirection);
      Serial.print("\t");
      Serial.print("Brightness: ");
      Serial.print("\t");
      Serial.print(brightness);
      Serial.print("\t");
    }
  }

  // Adjust startPixel
  else if (mode == 2){
    
    // Linear Step
    startPixel += dimmerDirection * 5;
  
    if (startPixel >= LEDcount){
      startPixel = LEDcount;
    }
    else if (startPixel <= 0){
      startPixel = 0;
    }

    // Logging
    if(logging){
      Serial.print("\n");
      Serial.print(dimmerDirection);
      Serial.print("\t");
      Serial.print("startPixel: ");
      Serial.print("\t");
      Serial.print(startPixel);
    }
  }

  // Adjust total LEDs
  else if (mode == 3){

    // Linear Step
    LEDcount += dimmerDirection * 10;
  
    if (LEDcount >= strip.numPixels()){
      LEDcount = strip.numPixels();
    }
    else if (LEDcount <= 0){
      LEDcount = 0;
    }

    // Clear removed pixels
    if(dimmerDirection < 0){
      strip.clear();
    }

    if(logging){
      Serial.print("\n");
      Serial.print(dimmerDirection);
      Serial.print("\t");
      Serial.print("LEDcount: ");
      Serial.print("\t");
      Serial.print(LEDcount);
    }
  }
  
}
void handleClicks() {
  // For single click, adjust density
  if (enc.clicks < 2){
    setDensity();
  }
  // 5 clicks switches hold-mode
  else if (enc.clicks == 5){
    mode++;

    if (mode > 3){
      mode = 1;
    }

    // Reset dimmer direction for consistency
    dimmerDirection = 1;

    // Display mode in first few LEDs
    strip.clear();
    for (int i = 0; i < mode; i++){
      strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(baseHue, 100)));
    }
    strip.show(); // Update strip with new contents
    delay(3000);  // Pause for a moment
    }
}

void loop() {

  // EncButton incrementing
  enc.tick();   // обработка всё равно здесь

  // Rainbow circuit
  for(int i=0; i< (LEDcount - startPixel); i++) { // For each pixel in strip...

    // Color wheel has a range of 65536 Hues
    int pixelHue = baseHue + (i * 65536L / ((LEDcount - startPixel) / totalRainbows));

    strip.setPixelColor(startPixel + i, strip.gamma32(strip.ColorHSV(pixelHue)));
  }

  // Check knob once per rotation
  long newPosition = myEnc.read();

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

  // Non-rainbow portion at start of strand
  for (int i = 0; i < startPixel && i < LEDcount; i++){
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

  baseHue += 256 * direction;
  baseHue += 256 * (velocity / 10);
}
