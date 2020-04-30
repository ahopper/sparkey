# sparkey
midi interface to connect Morse keys or keyers to Spark SDR or any other software with midi keyer input.

All testing has been done on a pro micro Arduino clone.

A straight key or keyer can be connected to pin 2.

There is a very basic iambic A keyer. For this connect the dit paddle to pin 3 and the dah to pin 4. 
Alternatively the keyer will work with touch sensing on A0 and A1 just by connecting some thing conductive to these pins.

The wpm is currently hard coded, the plan is to set this from Spark via midi.
