# HAL2MID
HAL Laboratory (GB) to MIDI converter

This tool converts music (and sound effects) from Game Boy games using HAL Laboratory's sound engine to MIDI format. It is most notably used in four of the Kirby games.

It works with ROM images. To use it, you must specify the name of the ROM followed by the number of the bank containing the sound data (in hex).
For games that contain 2 separate banks for music and sound effects, you must run the program 2 times specifying where each different bank is located. However, in order to prevent files from being overwritten, the MIDI files from the previous bank must either be moved to a separate folder or renamed.

Examples:
* HAL2MID "Kirby's Dream Land (UE) [!].gb" 6
* HAL2MID "Ghostbusters II (UE) [!].gb" 8
* HAL2MID "Adventures of Lolo (U) [S][!].gb" 10

This tool was based on my own reverse-engineering of the sound engine. As usual, a TXT "prototype" application, HAL2TXT, is also included.

Supported games:
  * Adventures of Lolo
  * Ghostbusters II
  * Kirby's Dream Land
  * Kirby's Dream Land 2
  * Kirby's Pinball Land
  * Kirby's Star Stacker
  * Pinball: Revenge of the 'Gator
  * Trax
  * Vegas Stakes

## To do:
  * Support for the NES version of the sound engine
  * GBS file support
