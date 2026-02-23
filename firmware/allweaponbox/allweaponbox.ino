//===========================================================================//
//                                                                           //
//  Desc:    Arduino Code to implement a fencing scoring apparatus           //
//  Dev:     Wnew                                                            //
//  Date:    Nov  2012                                                       //
//  Updated: Sept 2015                                                       //
//  Notes:   1. Basis of algorithm from digitalwestie on github. Thanks Mate //
//           2. Used uint8_t instead of int where possible to optimise       //
//           3. Set ADC prescaler to 16 faster ADC reads                     //
//                                                                           //
//  To do:   1. Could use shift reg on lights and mode LEDs to save pins     //
//           2. Implement short circuit LEDs (already provision for it)      //
//           3. Set up debug levels correctly                                //
//                                                                           //
//===========================================================================//

//============
// #defines
//============
//TODO: set up debug levels correctly
#define DEBUG 0
//#define TEST_LIGHTS       // turns on lights for a second on start up
//#define TEST_ADC_SPEED    // used to test sample rate of ADCs
//#define REPORT_TIMING     // prints timings over serial interface
#define BUZZERTIME  1000  // length of time the buzzer is kept on after a hit (ms)
#define LIGHTTIME   3000  // length of time the lights are kept on after a hit (ms)
#define BAUDRATE    57600  // baudrate of the serial debug interface

//============
// Pin Setup
//============
const uint8_t shortLEDA  =  8;    // Short Circuit A Light
const uint8_t onTargetA  =  9;    // On Target A Light
const uint8_t offTargetA = 10;    // Off Target A Light
const uint8_t offTargetB = 11;    // Off Target B Light
const uint8_t onTargetB  = 12;    // On Target B Light
const uint8_t shortLEDB  = 13;    // Short Circuit A Light

const uint8_t groundPinA = A0;    // Ground A pin - Analog
const uint8_t weaponPinA = A1;    // Weapon A pin - Analog
const uint8_t lamePinA   = A2;    // Lame   A pin - Analog (Epee return path)
const uint8_t lamePinB   = A3;    // Lame   B pin - Analog (Epee return path)
const uint8_t weaponPinB = A4;    // Weapon B pin - Analog
const uint8_t groundPinB = A5;    // Ground B pin - Analog

const uint8_t modePin    =  2;        // Mode change button interrupt pin 0 (digital pin 2)
const uint8_t buzzerPin  =  3;        // buzzer pin
const uint8_t modeLeds[] = {4, 5, 6}; // LED pins to indicate weapon mode selected {f e s}

//=========================
// values of analog reads
//=========================
int weaponA = 0;
int weaponB = 0;
int lameA   = 0;
int lameB   = 0;
int groundA = 0;
int groundB = 0;

//=======================
// depress and timeouts
//=======================
long depressAtime = 0;
long depressBtime = 0;
bool lockedOut    = false;

//==========================
// Lockout & Depress Times
//==========================
// the lockout time between hits for foil is 300ms +/-25ms
// the minimum amount of time the tip needs to be depressed for foil 14ms +/-1ms
// the lockout time between hits for epee is 45ms +/-5ms (40ms -> 50ms)
// the minimum amount of time the tip needs to be depressed for epee 2ms
// the lockout time between hits for sabre is 170ms +/-10ms
// the minimum amount of time the tip needs to be depressed (in contact) for sabre 0.1ms -> 1ms
// These values are stored as micro seconds for more accuracy
//                         foil   epee   sabre
long lockout [] = {300000,  45000, 170000};  // the lockout time between hits
long depress [] = { 14000,   2000,   1000};  // the minimum amount of time the tip needs to be depressed

//=================
// mode constants
//=================
const uint8_t FOIL_MODE  = 0;
const uint8_t EPEE_MODE  = 1;
const uint8_t SABRE_MODE = 2;
const String WEAPON_NAMES[3] = { "FLEURET", "EPEE", "SABRE" };
const uint8_t BLINKING[3][4] = {
  { HIGH, HIGH, HIGH, LOW },
  { HIGH, LOW, HIGH, LOW },
  { HIGH, HIGH, HIGH, HIGH }
  
};
uint8_t currentMode = SABRE_MODE;

bool modeJustChangedFlag = false;

//=========
// states
//=========
boolean depressedRed  = false;
boolean depressedGreen  = false;
boolean hitOnTargRed  = false;
boolean hitOffTargRed = false;
boolean hitOnTargGreen  = false;
boolean hitOffTargGreen = false;

#ifdef TEST_ADC_SPEED
long now;
long loopCount = 0;
bool done = false;
#endif


//================
// Configuration
//================
void setup() {
   // set the internal pullup resistor on modePin
   pinMode(modePin, INPUT_PULLUP);

   // add the interrupt to the mode pin (interrupt is pin 0)
   attachInterrupt(digitalPinToInterrupt(modePin), changeMode, RISING);
   pinMode(modeLeds[0], OUTPUT);
   pinMode(modeLeds[1], OUTPUT);
   pinMode(modeLeds[2], OUTPUT);
   pinMode(LED_BUILTIN, OUTPUT);

   // set the light pins to outputs
   pinMode(offTargetA, OUTPUT);
   pinMode(offTargetB, OUTPUT);
   pinMode(onTargetA,  OUTPUT);
   pinMode(onTargetB,  OUTPUT);
   pinMode(shortLEDA,  OUTPUT);
   pinMode(shortLEDB,  OUTPUT);
   pinMode(buzzerPin,  OUTPUT);

   digitalWrite(modeLeds[currentMode], HIGH);

#ifdef TEST_LIGHTS
   testLights();
#endif

   // this optimises the ADC to make the sampling rate quicker
   //adcOpt();

   Serial.begin(BAUDRATE);
   tellMode();
   resetValues();
}

void tellMode() {
   Serial.print("Mode changed to: ");
   Serial.println(WEAPON_NAMES[currentMode]);
}

//=============
// ADC config
//=============
void adcOpt() {

   // the ADC only needs a couple of bits, the atmega is an 8 bit micro
   // so sampling only 8 bits makes the values easy/quicker to process
   // unfortunately this method only works on the Due.
   //analogReadResolution(8);

   // Data Input Disable Register
   // disconnects the digital inputs from which ever ADC channels you are using
   // an analog input will be float and cause the digital input to constantly
   // toggle high and low, this creates noise near the ADC, and uses extra 
   // power Secondly, the digital input and associated DIDR switch have a
   // capacitance associated with them which will slow down your input signal
   // if you’re sampling a highly resistive load 
   DIDR0 = 0x7F;

   // set the prescaler for the ADCs to 16 this allowes the fastest sampling
   bitClear(ADCSRA, ADPS0);
   bitClear(ADCSRA, ADPS1);
   bitSet  (ADCSRA, ADPS2);
}


//============
// Main Loop
//============
void loop() {
   // use a while as a main loop as the loop() has too much overhead for fast analogReads
   // we get a 3-4% speed up on the loop this way
   while(1) {
	  readSerial();
      updateBlinkingModeLed();
      checkIfModeChanged();
      // read analog pins
      weaponA = analogRead(weaponPinA);
      weaponB = analogRead(weaponPinB);
      lameA   = analogRead(lamePinA);
      lameB   = analogRead(lamePinB);
      if      (currentMode == FOIL_MODE)
         foil();
      else if (currentMode == EPEE_MODE)
         epee();
      else if (currentMode == SABRE_MODE)
         sabre();

#ifdef TEST_ADC_SPEED
      if (loopCount == 0) {
         now = micros();
      }
      loopCount++;
      if ((micros()-now >= 1000000) && done == false) {
         Serial.print(loopCount);
         Serial.println(" readings in 1 sec");
         done = true;
      }
#endif
   }
}

int findWeaponId(String weaponName) {
	for (int i = 0; i < 3; i++) {
		if (WEAPON_NAMES[i].compareTo(weaponName) == 0) {
			return i;
		}
	}
	return -1;
}

void readSerial() {
	if (Serial.available()) {
		String input = Serial.readString();
      if (input.endsWith("\n")) {
         input = input.substring(0, input.length() - 1);
      }
      if (input.equals("?")) {
         tellMode();
         // tellConf
         return;
      }
		int weaponId = findWeaponId(input);
		if (weaponId >= 0) {
			currentMode = weaponId;
			setModeLeds();
			tellMode();
         return;
		}
      int indexOfDot = input.lastIndexOf(".");
      int indexOfEquals = input.lastIndexOf("=");
      if (indexOfDot > 0 && indexOfEquals > 0) {
         String weaponName = input.substring(0, indexOfDot);
         weaponId = findWeaponId(weaponName);
         if (weaponId >= 0) {
            String configure = input.substring(indexOfDot + 1, indexOfEquals);
            long* tableToSet = 0;
            if (configure.equals("depress")) {
               tableToSet = depress;
            } else if (configure.equals("lockout")) {
               tableToSet = lockout;
            }
            if (tableToSet > 0) {
               long value = input.substring(indexOfEquals + 1, input.length()).toInt();
               if (value > 0) {
                  tableToSet[weaponId] = value;
               }
            }
         }
      }
	}
}

//=====================
// Mode pin interrupt
//=====================
void changeMode() {
   // set a flag to keep the time in the ISR to a min
   modeJustChangedFlag = true;
}

void updateBlinkingModeLed() {
   long now = micros();
   // span feeback blinking by 300ms per hit
   now /= 300000; // to hit number
   now %= sizeof(BLINKING[currentMode]) / sizeof(BLINKING[currentMode][0]);
   digitalWrite(LED_BUILTIN, BLINKING[currentMode][now]);
}

//============================
// Sets the correct mode led
//============================
void setModeLeds() {
   digitalWrite(modeLeds[FOIL_MODE],  LOW);
   digitalWrite(modeLeds[EPEE_MODE],  LOW);
   digitalWrite(modeLeds[SABRE_MODE], LOW);
   digitalWrite(modeLeds[currentMode], HIGH);
   if (currentMode == FOIL_MODE) {
      digitalWrite(onTargetA, HIGH);
   } else {
      if (currentMode == EPEE_MODE) {
        digitalWrite(onTargetB, HIGH);
      } else {
         if (currentMode == SABRE_MODE){
            digitalWrite(onTargetA, HIGH);
            digitalWrite(onTargetB, HIGH);
         }
      }
   }
   delay(500);
   digitalWrite(onTargetA, LOW);
   digitalWrite(onTargetB, LOW);
}


//========================
// Run when mode changed
//========================
void checkIfModeChanged() {
 if (modeJustChangedFlag) {
      if (digitalRead(modePin)) {
    	 currentMode = (currentMode + 1) % 3;
      }
      delay(500); // avoid pressure to cause more changes than expected
      setModeLeds();
      tellMode();
      modeJustChangedFlag = false;
   }
}


//===================
// Main foil method
//===================
void foil() {

   long now = micros();
   if (((hitOnTargRed || hitOffTargRed) && (depressAtime + lockout[0] < now)) ||
       ((hitOnTargGreen || hitOffTargGreen) && (depressBtime + lockout[0] < now))) {
      resetValues();
   }

   // weapon A
   if (hitOnTargRed == false && hitOffTargRed == false) { // ignore if A has already hit
      // off target
      if (900 < weaponA && lameB < 100) {
         if (!depressedRed) {
            depressAtime = micros();
            depressedRed   = true;
         } else {
            if (depressAtime + depress[0] <= micros()) {
               signalHits(false, true, false, false);
            }
         }
      } else {
      // on target
         if (400 < weaponA && weaponA < 600 && 400 < lameB && lameB < 600) {
            if (!depressedRed) {
               depressAtime = micros();
               depressedRed   = true;
            } else {
               if (depressAtime + depress[0] <= micros()) {
                  signalHits(true, false, false, false);
               }
            }
         } else {
            // reset these values if the depress time is short.
            depressAtime = 0;
            depressedRed   = 0;
         }
      }
   }

   // weapon B
   if (hitOnTargGreen == false && hitOffTargGreen == false) { // ignore if B has already hit
      // off target
      if (900 < weaponB && lameA < 100) {
         if (!depressedGreen) {
            depressBtime = micros();
            depressedGreen   = true;
         } else {
            if (depressBtime + depress[0] <= micros()) {
            	signalHits(false, false, true, false);
            }
         }
      } else {
      // on target
         if (400 < weaponB && weaponB < 600 && 400 < lameA && lameA < 600) {
            if (!depressedGreen) {
               depressBtime = micros();
               depressedGreen   = true;
            } else {
               if (depressBtime + depress[0] <= micros()) {
                  signalHits(false, false, false, true);
               }
            }
         } else {
            // reset these values if the depress time is short.
            depressBtime = 0;
            depressedGreen   = 0;
         }
      }
   }
}


//===================
// Main epee method
//===================
void epee() {
   long now = micros();
   if ((hitOnTargRed && (depressAtime + lockout[1] < now)) || (hitOnTargGreen && (depressBtime + lockout[1] < now))) {
      resetValues();
   }

   // weapon A
   //  no hit for A yet    && weapon depress    && opponent lame touched
   if (hitOnTargRed == false) {
      if (400 < weaponA && weaponA < 600 && 400 < lameA && lameA < 600) {
         if (!depressedRed) {
            depressAtime = micros();
            depressedRed   = true;
         } else {
            if (depressAtime + depress[1] <= micros()) {
               signalHits(true, false, false, false);
            }
         }
      } else {
         // reset these values if the depress time is short.
         if (depressedRed == true) {
            depressAtime = 0;
            depressedRed   = 0;
         }
      }
   }

   // weapon B
   //  no hit for B yet    && weapon depress    && opponent lame touched
   if (hitOnTargGreen == false) {
      if (400 < weaponB && weaponB < 600 && 400 < lameB && lameB < 600) {
         if (!depressedGreen) {
            depressBtime = micros();
            depressedGreen   = true;
         } else {
            if (depressBtime + depress[1] <= micros()) {
            	signalHits(false, false, false, true);
            }
         }
      } else {
         // reset these values if the depress time is short.
         if (depressedGreen == true) {
            depressBtime = 0;
            depressedGreen   = 0;
         }
      }
   }
}


bool isAbout(unsigned int millivolts, unsigned int capturedADC) {
   unsigned int measuredVoltage = ((capturedADC * 40) /* no term must be > 2^16 ~= 60000 */ / 1024) * 125;
   return abs(millivolts - measuredVoltage) < 500; // tolerance of 500mV, which is 10%
}

//===================
// Main sabre method
//===================
void sabre() {

	// Still failing cases: both fencers hit themselves with their weapon simulataneously
	// EXPECTED: no light
	// GOT: both lights
	// analysis: Checking for the state if identical circuits is not enough, sabre seems to
	// require a way to "sign" which weapon(s) are causing the hit. This could probably be
	// achieved by using different resistances on each weapon to capute different voltages
	// and deduce what weapon caused the hit.

   long now = micros();
   if (((hitOnTargRed || hitOffTargRed) && (depressAtime + lockout[2] < now)) ||
       ((hitOnTargGreen || hitOffTargGreen) && (depressBtime + lockout[2] < now))) {
      resetValues();
   }

   // weapon A
   if (hitOnTargRed == false && hitOffTargRed == false) { // ignore if A has already hit
	   bool atRestWeaponA = isAbout(5000, weaponA);
	   bool atRestLameB = isAbout(0, lameB);
      // on target
      if (!atRestWeaponA && !atRestLameB) {
         if (!depressedRed) {
            depressAtime = micros();
            depressedRed   = true;
         } else {
            if (depressAtime + depress[2] <= micros()) {
               signalHits(true, false, false, false);
            }
         }
      } else {
         // reset these values if the depress time is short.
         depressAtime = 0;
         depressedRed   = 0;
      }
   }

   // weapon B
   if (hitOnTargGreen == false && hitOffTargGreen == false) { // ignore if B has already hit
	   bool atRestWeaponB = isAbout(5000, weaponB);
	   bool atRestLameA = isAbout(0, lameA);
      // on target
      if (!atRestWeaponB && !atRestLameA) {
         if (!depressedGreen) {
            depressBtime = micros();
            depressedGreen   = true;
         } else {
            if (depressBtime + depress[2] <= micros()) {
               signalHits(false, false, false, true);
            }
         }
      } else {
         // reset these values if the depress time is short.
         depressBtime = 0;
         depressedGreen   = 0;
      }
   }
}


//==============
// Signal Hits
//==============
void signalHits(bool justHitOnTargRed, bool justHitOffTargRed, bool justHitOffTargGreen, bool justHitOnTargGreen) {
   // non time critical, this is run after a hit has been detected
   if (justHitOnTargRed || justHitOffTargRed || justHitOffTargGreen || justHitOnTargGreen) {
      digitalWrite(buzzerPin,  HIGH);
   }
   if (justHitOnTargRed) {
      hitOnTargRed = true;
	   digitalWrite(onTargetA, true);
	   Serial.println("R");
   }
   if (justHitOffTargRed) {
      hitOffTargRed = true;
	   digitalWrite(offTargetA, true);
	   Serial.println("r");
   }
   if (justHitOffTargGreen) {
      hitOffTargGreen = true;
	   digitalWrite(offTargetB, true);
	   Serial.println("g");
   }
   if (justHitOnTargGreen) {
      hitOnTargGreen = true;
      digitalWrite(onTargetB,  true);
      Serial.println("G");
   }
}


//======================
// Reset all variables
//======================
void resetValues() {
   delay(BUZZERTIME);             // wait before turning off the buzzer
   digitalWrite(buzzerPin,  LOW);
   delay(max(LIGHTTIME, lockout[currentMode] / 1000) - BUZZERTIME);   // wait before turning off the lights
   digitalWrite(onTargetA,  LOW);
   digitalWrite(offTargetA, LOW);
   digitalWrite(offTargetB, LOW);
   digitalWrite(onTargetB,  LOW);
   digitalWrite(shortLEDA,  LOW);
   digitalWrite(shortLEDB,  LOW);
   Serial.println("0");

   lockedOut    = false;
   depressAtime = 0;
   depressedRed   = false;
   depressBtime = 0;
   depressedGreen   = false;

   hitOnTargRed  = false;
   hitOffTargRed = false;
   hitOnTargGreen  = false;
   hitOffTargGreen = false;

   delay(100);
}


//==============
// Test lights
//==============
void testLights() {
   digitalWrite(offTargetA, HIGH);
   digitalWrite(onTargetA,  HIGH);
   digitalWrite(offTargetB, HIGH);
   digitalWrite(onTargetB,  HIGH);
   digitalWrite(shortLEDA,  HIGH);
   digitalWrite(shortLEDB,  HIGH);
   delay(1000);
   resetValues();
}
