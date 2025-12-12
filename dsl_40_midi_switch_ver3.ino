#include <MIDI.h>
#include <EEPROM.h>

// MIDI switcher for Marshall DSL 40
// the hrdware is composed of 4 momentary footswitches. 
// Footswitch 1 is for clean/crunch switching ( and implicitely for channel 1 activation): Two leds indicate the clean/crunch status
// Footswitch 2 is for OD1/OD2switching ( and implicitely for channel 2 activation ) : Two leds indicate the OD1/OD2 status
// Footswitch 3 is for Master 1 / Master 2 switching : Two leds indicate the Mastezr 1 / 2 status
// Footswitch 4 is FX loop activation/deactivation : one led indicates the FX loop status

// The "issue" with DSL40 is that there is no MIDI messages sent back by the amp. to the switcher to give the current status of all presets.
// Each modes ( clean/crunch/OD1/OD2) have their own presets regarding FX loop and Master. Combination of presets might become a bit complex and lead to a de-synchronisation.
// So, each time the switcher is turned on, the amp. preset must be manually synchronised with the switcher preset state ( leds ). Also, if a preset is modified manually during a use session, the switcher is desyncronised with the amp.
// With most of the DYI switcher that I've seen on the web, the manual (re)synchronisation is a must to keep both switcher and amp. on the same state.

// This switcher program have an integrated syncrhonisation function. The switcher presets states are stored in the EEPROM. So, each time the switcher is turned on, its last state is restored. 
// if the amp presets has been modified manually in the mean time, a synchronisation of the amps can be triggered by a long hold ( > 3 sec. ) on the clean/crunch footswitch.

// In a first version of the program, the synchronisation was triggered each time the switcher was activated. Finally, the synchro is only triggered by demand.


MIDI_CREATE_DEFAULT_INSTANCE();
byte patchNum = 0;
byte cc14Num = 0;

struct feature {
  bool fxloop = false;
  int master = LOW;  // LOW = Master 1 / HIGH = Master 2;
};

struct allStatesStruct {
  int channelState = LOW;  //LOW = channel1 / HIGH = Channel 2
  struct {
    int buttonState = LOW;  // LOW = clean / HIGH = Crunch
    feature cleanFeature, crunchFeature;
  } channel1State;

  struct {
    int buttonState = LOW;  // LOW = OD1 / HIGH = OD2
    feature od1Feature, od2Feature;
  } channel2State;
  int fxlButtonState = LOW;
  int masterButtonState = LOW;
  int EEpromInitiazlized = 99;
};

allStatesStruct allStates;

const int cleanCrunchButton = 2;
const int cleanLed = 4;
const int crunchLed = 3;

const int odButton = 5;
const int od1Led = 7;
const int od2Led = 6;

const int fxlButton = 12;
const int fxlLed = 11;

const int masterButton = 8;
const int master1Led = 10;
const int master2Led = 9;

unsigned long debounceDelay = 50;  // the debounce time; increase if the output flickers

unsigned long cleanCrunchLastDebounceTime = 0;  // the last time the output pin was toggled
int cleanCrunchButtonState;
int cleanCrunchLastButtonState = LOW;

unsigned long odLastDebounceTime = 0;  // the last time the output pin was toggled
int odButtonState;
int odLastButtonState = LOW;

unsigned long fxlLastDebounceTime = 0;  // the last time the output pin was toggled
int fxlButtonState;
int fxlLastButtonState = LOW;

unsigned long masterLastDebounceTime = 0;  // the last time the output pin was toggled
int masterButtonState;
int masterLastButtonState = LOW;

unsigned long resetStartPressed = 0;
int resetButtonState;
int resetLastButtonState = LOW;
unsigned long resetDelay = 3000;  // the delay to consider a amp. syncro is requested
unsigned long midiDelay = 200;    // the delay between two MIDI message


void synchroniseLed() {

  if (allStates.channelState == LOW) {
    // channel 1 is active
    digitalWrite(cleanLed, !allStates.channel1State.buttonState);  //if button state is LOW --> clean is active --> led is HIGH
    digitalWrite(crunchLed, allStates.channel1State.buttonState);  //if button state is HIGH --> crunch is active --> led is HIGH
    digitalWrite(od1Led, LOW);                                     //channel 2 leds are LOW
    digitalWrite(od2Led, LOW);
    // detrmine now the fx loop led state
    if (allStates.channel1State.buttonState == LOW) {  //if button state is LOW --> clean is active

      digitalWrite(fxlLed, allStates.channel1State.cleanFeature.fxloop);  // if fxloop is active for clean --> fxled is active too
    } else {                                                              //if button state is LOW --> clean is active

      digitalWrite(fxlLed, allStates.channel1State.crunchFeature.fxloop);  // if fxloop is active for crunch --> fxled is active too
    }
    // detrmine now the master led state
    if (allStates.channel1State.buttonState == LOW) {                          //if button state is LOW --> clean is active
      digitalWrite(master1Led, !allStates.channel1State.cleanFeature.master);  // if master clean is low --> master 1 selected --> led master 1 HIGH
      digitalWrite(master2Led, allStates.channel1State.cleanFeature.master);   // if master clean is HIGH --> master 2 selected --> led master 2 HIGH
    } else {
      digitalWrite(master1Led, !allStates.channel1State.crunchFeature.master);  // if master crunch is low --> master 1 selected --> led master 1 HIGH
      digitalWrite(master2Led, allStates.channel1State.crunchFeature.master);   // if master crunch is HIGH --> master 2 selected --> led master 2 HIGH
    }

  } else {
    // channel 2 is active
    digitalWrite(od1Led, !allStates.channel2State.buttonState);  //if button state is LOW --> od1 is active --> led is HIGH
    digitalWrite(od2Led, allStates.channel2State.buttonState);   //if button state is HIGH -->  od2 is active --> led is HIGH
    digitalWrite(cleanLed, LOW);                                 //channel 1 leds are LOW
    digitalWrite(crunchLed, LOW);
    // detrmine now the fx loop led state
    if (allStates.channel2State.buttonState == LOW) {                   //if button state is LOW -->od1 is active
      digitalWrite(fxlLed, allStates.channel2State.od1Feature.fxloop);  // if fxloop is active for od1 --> fxled is active too
    } else {
      digitalWrite(fxlLed, allStates.channel2State.od2Feature.fxloop);  // if fxloop is active for od2 --> fxled is active too
    }
    // determine now the master led state
    if (allStates.channel2State.buttonState == LOW) {                        //if button state is LOW --> od1 is active
      digitalWrite(master1Led, !allStates.channel2State.od1Feature.master);  // if master OD1 is low --> master 1 selected --> led master 1 HIGH
      digitalWrite(master2Led, allStates.channel2State.od1Feature.master);   //if master OD1 is HIGH --> master 2 selected --> led master 2 HIGH
    } else {                                                                 //if button state is LOW --> od2 is active
      digitalWrite(master1Led, !allStates.channel2State.od2Feature.master);  // if master OD2 is low --> master 1 selected --> led master 1 HIGH
      digitalWrite(master2Led, allStates.channel2State.od2Feature.master);   //if master OD2 is HIGH --> master 2 selected --> led master 2 HIGH
    }
  }
}

void synchronise() {

  // blink clean/crunch leds to indicate syncro is started
  for (int i = 0; i <= 10; i++) {
    digitalWrite(cleanLed, LOW);
    digitalWrite(crunchLed, HIGH);
    delay(50);
    digitalWrite(cleanLed, HIGH);
    digitalWrite(crunchLed, LOW);
    delay(50);
  }

  // set all leds to low
  digitalWrite(cleanLed, LOW);
  digitalWrite(crunchLed, LOW);
  digitalWrite(od1Led, LOW);
  digitalWrite(od2Led, LOW);
  digitalWrite(master1Led, LOW);
  digitalWrite(master2Led, LOW);
  digitalWrite(fxlLed, LOW);

  //preset channel 1 / clean with fx and master position
  digitalWrite(cleanLed, HIGH);
  checkAndPC(0);
  checkAndCC(14, allStates.channel1State.cleanFeature.master);
  checkAndCC(13, allStates.channel1State.cleanFeature.fxloop == true ? HIGH : LOW);

  // preset channel 1 / crunch with fx and master position
  digitalWrite(crunchLed, HIGH);
  checkAndPC(1);
  checkAndCC(14, allStates.channel1State.crunchFeature.master);
  checkAndCC(13, allStates.channel1State.crunchFeature.fxloop == true ? HIGH : LOW);

  // preset channel 2 / od1 with fx and master position
  digitalWrite(od1Led, HIGH);
  checkAndPC(2);
  checkAndCC(14, allStates.channel2State.od1Feature.master);
  checkAndCC(13, allStates.channel2State.od1Feature.fxloop == true ? HIGH : LOW);

  // preset channel 2 / od2 with fx and master position
  digitalWrite(od2Led, HIGH);
  checkAndPC(3);
  checkAndCC(14, allStates.channel2State.od2Feature.master);
  checkAndCC(13, allStates.channel2State.od2Feature.fxloop == true ? HIGH : LOW);
  digitalWrite(master1Led, HIGH);

  // set active channel recorded in memory
  if (allStates.channelState == LOW) {
    checkAndPC(allStates.channel1State.buttonState == LOW ? 0 : 1);
  } else {
    checkAndPC(allStates.channel2State.buttonState == LOW ? 2 : 3);
  }
  // the following to get the same delay between last two led "bargraph" effect during synchro
  delay(midiDelay * 2);
  digitalWrite(master2Led, HIGH);
  delay(midiDelay * 3);
  digitalWrite(fxlLed, HIGH);

  // syncronise all led with current state
  synchroniseLed();
}

void setup() {
 
  pinMode(cleanCrunchButton, INPUT);
  pinMode(cleanLed, OUTPUT);
  pinMode(crunchLed, OUTPUT);

  pinMode(odButton, INPUT);
  pinMode(od1Led, OUTPUT);
  pinMode(od2Led, OUTPUT);

  pinMode(fxlButton, INPUT);
  pinMode(fxlLed, OUTPUT);

  pinMode(masterButton, INPUT);
  pinMode(master1Led, OUTPUT);
  pinMode(master2Led, OUTPUT);

  MIDI.begin(MIDI_CHANNEL_OMNI);
  
// Get state buffer from EEprom
  EEPROM.get(0, allStates);
  while (allStates.EEpromInitiazlized != 99) {
    //if we come here, there is a big chance that is it the first time the EEprom is used for this program
    // --> format the eeprom with a clear buffer
    allStatesStruct initStates;
    // set the 99 tag to indicate the next tilme that the eeprom has been alredy initialized
    initStates.EEpromInitiazlized = 99;
    // write eeprom with a fresh buffer
    EEPROM.put(0, initStates);
    // move the fresh buffer from memory to state buffer
    EEPROM.get(0, allStates);
  }

  synchroniseLed();
}

void loop() {

  // detect a brief push on button clean crunch
  int reading = digitalRead(cleanCrunchButton);
  if (reading != cleanCrunchLastButtonState) {
    if (reading == HIGH) {
      // push button is high : start to count debouncing
      cleanCrunchLastDebounceTime = millis();
    }
  }
  unsigned long elapsedTime = millis() - cleanCrunchLastDebounceTime;
  if (elapsedTime > debounceDelay and elapsedTime < resetDelay) {
    // we are in the autorised time frame to detect a clean crunch command request ( between debounce andf restet delay)
    if (reading != cleanCrunchButtonState) {
      cleanCrunchButtonState = reading;
      // react only if we pass from HIGH to LOW
      if (cleanCrunchButtonState == LOW) {
        //switch clean/crunch
        cleanCrunchSwitch();
      }
    }
  }
  cleanCrunchLastButtonState = reading;

  // detect a long push on button clean crunch ( > 3 sec. ) to trigger Amp. synchronisation
  if (reading != resetLastButtonState and reading == HIGH) {
    resetStartPressed = millis();
  }
  if ((millis() - resetStartPressed) > resetDelay) {
    if (reading != resetButtonState) {
      resetButtonState = reading;
      if (resetButtonState == HIGH) {
        //trigger an amp syncronisation with the state buffer
        synchronise();
      }
    }
  }
  resetLastButtonState = reading;

  // detect switch command request on OD1/OD2 button
  reading = digitalRead(odButton);
  if (reading != odLastButtonState) {
    odLastDebounceTime = millis();
  }
  if ((millis() - odLastDebounceTime) > debounceDelay) {
    if (reading != odButtonState) {
      odButtonState = reading;
      if (odButtonState == HIGH) {
        //switch OD1/OD2
        odSwitch();
      }
    }
  }
  odLastButtonState = reading;

  // detect switch command request on FX loop button
  reading = digitalRead(fxlButton);
  if (reading != fxlLastButtonState) {
    fxlLastDebounceTime = millis();
  }
  if ((millis() - fxlLastDebounceTime) > debounceDelay) {
    if (reading != fxlButtonState) {
      fxlButtonState = reading;
      if (fxlButtonState == HIGH) {
        //switch Fx loop
        fxlSwitch();
      }
    }
  }
  fxlLastButtonState = reading;

  // detect switch command request on master 1/2 button
  reading = digitalRead(masterButton);
  if (reading != masterLastButtonState) {
    masterLastDebounceTime = millis();
  }
  if ((millis() - masterLastDebounceTime) > debounceDelay) {
    if (reading != masterButtonState) {
      masterButtonState = reading;
      if (masterButtonState == HIGH) {
        //switch master 1 / 2
        masterSwitch();
      }
    }
  }
  masterLastButtonState = reading;
}
// trigger a PC Midi command toward amp.
int checkAndPC(int pc) {
  MIDI.sendProgramChange(pc, 1);
  delay(midiDelay);
}
// trigger a CC Midi command toward amp.
int checkAndCC(int cc, int value) {
  MIDI.sendControlChange(cc, value, 1);
  delay(midiDelay);
}
// Save current state ( button/Led )
void saveState() {
  EEPROM.put(0, allStates);
}
// trigger a clean/crunch switch
void cleanCrunchSwitch() {
  // inverse clean/crunch button state
  allStates.channel1State.buttonState = !allStates.channel1State.buttonState;
  // Send a PC MIDI message to Amp.
  checkAndPC(allStates.channel1State.buttonState == LOW ? 0 : 1);
  // if we are here it means that implicitely the channel 1 must be active --> set state buffer with Channel 1 selected
  allStates.channelState = LOW;
  //syncrhonise LED Vs. button change
  synchroniseLed();
  //Save change in memory
  saveState();
}
// trigger a OD1/OD2 switch
void odSwitch() {
  // inverse OD1/OD2 button state
  allStates.channel2State.buttonState = !allStates.channel2State.buttonState;
  // Send a PC MIDI message to Amp.
  checkAndPC(allStates.channel2State.buttonState == LOW ? 2 : 3);
  // if we are here it means that implicitely the channel 2 must be active --> set state buffer with Channel 2 selected
  allStates.channelState = HIGH;
  //syncrhonise LED Vs. button change
  synchroniseLed();
  //Save change in memory
  saveState();
}
// trigger a FX loop state switch
void fxlSwitch() {
  // inverse fx loop  button state
  allStates.fxlButtonState = !allStates.fxlButtonState;
  // Send a PC MIDI message to Amp.
  checkAndCC(13, allStates.fxlButtonState);
  //set LED state
  digitalWrite(fxlLed, allStates.fxlButtonState);
  // record state of Fx loop for the current channel/mode in state buffer
  if (allStates.channelState == LOW) {
    // channel 1 is active
    if (allStates.channel1State.buttonState == LOW) {
      // Clean mode is selected
      allStates.channel1State.cleanFeature.fxloop = allStates.fxlButtonState;  //new fx loop state recorded for clean
    } else {
      // crunch mode is selected
      allStates.channel1State.crunchFeature.fxloop = allStates.fxlButtonState;  //new fx loop state recorded for crunch
    }
  } else {
    // channel 2 is selected
    if (allStates.channel2State.buttonState == LOW) {
      // OD1 mode is selected
      allStates.channel2State.od1Feature.fxloop = allStates.fxlButtonState;  //new fx loop state recorded for OD1
    } else {
      // OD2 mode is selected
      allStates.channel2State.od2Feature.fxloop = allStates.fxlButtonState;  //new fx loop state recorded for OD2
    }
  }
  //Save state buffer in memory
  saveState();
}
// trigger a Master state switch
void masterSwitch() {
  // inverse Master button state
  allStates.masterButtonState = !allStates.masterButtonState;
  //send a MIDI message to amp
  checkAndCC(14, allStates.masterButtonState);
  digitalWrite(master1Led, !allStates.masterButtonState);  //if master button state is LOW -> Master 1 selected --> Led master 1 HIGH
  digitalWrite(master2Led, allStates.masterButtonState);   //if master button state is HIGH -> Master 2 selected --> Led master 1 HIGH
  // record state of master for the current channel/mode in state buffer
  if (allStates.channelState == LOW) {
    // channel 1 is active
    if (allStates.channel1State.buttonState == LOW) {
      // Clean mode is selected
      allStates.channel1State.cleanFeature.master = allStates.masterButtonState;  //new master state recorded for clean
    } else {
      // crunch mode is selected
      allStates.channel1State.crunchFeature.master = allStates.masterButtonState;  //new master state recorded for crunch
    }
  } else {
    // channel 2 is selected
    if (allStates.channel2State.buttonState == LOW) {
      // OD1 mode is selected
      allStates.channel2State.od1Feature.master = allStates.masterButtonState;  //new master state recorded for OD1
    } else {
      // OD2 mode is selected
      allStates.channel2State.od2Feature.master = allStates.masterButtonState;  //new master state recorded for OD2
    }
  }
  //Save state buffer in memory
  saveState();
}
