# MIDI switcher for Marshall DSL 40

## Introduction
This repository propose a DYI switcher for the Marshall DSL40 tube Amplificator. The switcher is capable to select channel 1 / 2 selection with switching between modes ( Clean/Crunch / OD1 /OD 2 ). It can select also Master 1 or 2 and the FX loop activation/deactivation.

The core of the circuit is an **Arduino Every**. 
The switcher is linked with the DSL40 through a MIDI cable ( DIN5 )  and is powered up with an external 9V source.
the switching command is provided by 4 momentary footswitches. 
- Footswitch 1 is for clean/crunch switching ( and implicitely for channel 1 activation): Two leds indicate the clean/crunch status
- Footswitch 2 is for OD1/OD2switching ( and implicitely for channel 2 activation ) : Two leds indicate the OD1/OD2 status
- Footswitch 3 is for Master 1 / Master 2 switching : Two leds indicate the Mastezr 1 / 2 status
- Footswitch 4 is FX loop activation/deactivation : one led indicates the FX loop status.

![switcher](https://github.com/user-attachments/assets/cbfd0721-648a-4c7e-84bd-f8c79d852f18)

***Short demo video at https://youtu.be/Z-kX9PDRV-0***

## Syncrhonisation function

The "issue" with DSL40 is that there is no MIDI messages sent back by the amp. to the switcher to give the current status of all presets.
Each modes ( clean/crunch/OD1/OD2) have their own presets regarding FX loop and Master. Combination of presets might become a bit complex and lead to a de-synchronisation.
So, each time the switcher is turned on, the amp. preset must be manually synchronised with the switcher preset state ( leds ). Also, if a preset is modified manually during a use session, the switcher is desyncronised with the amp.
With most of the DYI switcher that I've seen on the web, the manual (re)synchronisation is a must to keep both switcher and amp. on the same state.

This switcher program have an integrated syncrhonisation function. The switcher presets states are stored in the EEPROM. So, each time the switcher is turned on, its last state is restored. 
if the amp presets has been modified manually in the mean time, a synchronisation of the amps can be triggered by a long hold ( > 3 sec. ) on the clean/crunch footswitch.

In a first version of the program, the synchronisation was triggered each time the switcher was turned on. Finally, the synchro is only triggered by demand.

## Hardware 

In folder Eagle, you'll find schematic and board to build the circuit board. If you have a CNC machine, you'll find too the G code files to engrave the circuit and the case.

![circuit 1](https://github.com/user-attachments/assets/0fa2b67c-2956-4702-ac5b-690facca2e66)

![circuit 2](https://github.com/user-attachments/assets/ca58f346-0f38-45d3-85b7-abe8205e0c27)

