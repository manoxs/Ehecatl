/*********************************************************
  Flauta Magica
**********************************************************/
// M16 Sinewave example
#include "M16.h" 
#include "Osc.h"
#include "Bob.h"

int16_t waveTable[TABLE_SIZE]; // empty wavetable
Osc aOsc1(waveTable);
Bob filter;
int16_t vol = 0; // 0 - 1024, 10 bit
unsigned long msNow = millis();
unsigned long breathTime = msNow;
unsigned long buttonTime = msNow;
int breathDelta = 5;
int buttonDelta = 30;
bool pinTouched [] = {false, false, false, false, false, false};
int pitches [] = {60, 62, 64, 67, 69, 72};
float prevAdc = 0;
bool touchingSomething = false;

#include "MultiControl.h"
MultiControl touchPin[6] {
  MultiControl(1,0),
  MultiControl(2,0),
  MultiControl(3,0),
  MultiControl(4,0),
  MultiControl(5,0),
  MultiControl(6,0)
};

#include <Adafruit_ADS1X15.h>
Adafruit_ADS1015 ads;
 
void setup() {
  Serial.begin(115200);
  delay(200);

  ads.setGain(GAIN_EIGHT);      // 8x gain   +/- 0.512V  1 bit = 0.25mV   0.015625mV
  if (!ads.begin()) {
    Serial.println("Failed to initialize ADS.");
    while (1);
  }

  Osc::sawGen(waveTable); // fill the wavetable
  aOsc1.setPitch(69);
  // seti2sPins(25, 27, 12, 21); // bck, ws, data_out, data_in // change ESP32 defaults
  audioStart();
}

void loop() {
  msNow = millis();

  if ((unsigned long)(msNow - breathTime) >= breathDelta) {
    breathTime += breathDelta;
    int16_t adc0 = ads.readADC_SingleEnded(0);
    if (!touchingSomething) adc0 = 0;
    // float adcF = min(1.0f, adc0 / 700.0f); // 0.0 - 1.0
    float adcF = (float)adc0 / 1000.0;
    adcF = min(1.0f, max(adcF, float(0.0)));
    Serial.print("adcF: "); Serial.println(adcF);
    if (abs(adcF - prevAdc) > 0.03) {
      if (adcF > prevAdc) {
        adcF = pow(adcF, 0.7);
      } else adcF = pow(adcF, 6);
    }
    adcF = slew(prevAdc, adcF, 0.2);
    filter.setCutoff(adcF); // filter cutoff
    prevAdc = adcF;
    vol =  adcF * 1024;
  }

  if ((unsigned long)(msNow - buttonTime) >= buttonDelta) {
    buttonTime += buttonDelta;
    touchingSomething = false;
    for (int i=0; i<6; i++) {
      int t = touchPin[i].isTouched();
      if (t) touchingSomething = true;
      if (t && !pinTouched[i]) {
        int pitch = pitches[i];
        aOsc1.setPitch(pitch);
        Serial.println(pitch);
        pinTouched[i] = true;
      }
      if (!t) {
        pinTouched[i] = false;
      }
    }
  }
}

/* The audioUpdate function is required in all M16 programs 
* to specify the audio sample values to be played.
* Always finish with i2s_write_samples()
*/
void audioUpdate() {
  int32_t leftVal = (aOsc1.next() * vol)>>10;
  int32_t rightVal = leftVal;
  i2s_write_samples(leftVal, rightVal);
}
