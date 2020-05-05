// use one of below for serial or real usb midi

//#include <MIDI.h> // Include MIDI Library
//MIDI_CREATE_DEFAULT_INSTANCE();

#include <USB-MIDI.h>
USBMIDI_CREATE_DEFAULT_INSTANCE();

#include <ADCTouch.h>



enum playState{
  playdit,
  playdah,
  playspace,
  playletterSpace,
  playwordSpace,
  playidle
};

enum keyState{
  idle,
  dit,
  dah,
  ditdah,
  dahdit
};

enum iambicMode{
  iambicA,
  iambicB
};

struct paddle
{
  int digitalPin;
  int analogPin;
  int analogOffset;
  bool pressed;
};

unsigned long ditTime = 100;
iambicMode iMode = iambicA;

const int BUTTON_PIN_COUNT = 3;

const int KEY_PIN = 2;
const int DIT_PIN = 3;
const int DAH_PIN = 4;

bool lastKey = false;

int buttonPins[BUTTON_PIN_COUNT] = { KEY_PIN, DIT_PIN, DAH_PIN };
int buttonDown[BUTTON_PIN_COUNT];

unsigned long lastKeyChange;
unsigned long now;

int dtime=0;

playState pState = playidle;
playState lastPlayed = playidle;
keyState kState = idle;
bool keyStateUsed = false;


bool extraChar = false;

// if both set true, the first to detect a change will be used
bool useTouch = true;
bool useDigital = true;

paddle ditPaddle = {DIT_PIN, A0, 0, false};
paddle dahPaddle = {DAH_PIN, A1, 0, false};


bool isButtonDown(int pin) {
  return digitalRead(pin) == 0;
}

//return true if changed
bool readPaddle(paddle* pad){
  bool last = pad->pressed;
  if(useTouch)
  {
    int ai = ADCTouch.read(pad->analogPin,3) - pad->analogOffset;
    pad->pressed = ai > 80; 
    if(pad->pressed)useDigital = false;
  }
  if(useDigital)
  {
    pad->pressed = digitalRead(pad->digitalPin) == 0;
    if(pad->pressed)useTouch = false;
  }
  return pad->pressed != last;
}

void setKey(bool down)
{
    MIDI.sendNoteOn( 64 , down ? 127:0,1);
    lastKeyChange = now;
}

bool getNextState(playState* state)
{  
  switch(kState)
  {
    case idle:
      *state=playidle;
      break;
    case dit:
      *state=playdit;
      break; 
    case dah:
      *state=playdah;
      break; 
    case ditdah:
      *state = lastPlayed == playdah ? playdit : playdah;
      break;
    case dahdit:
      *state = lastPlayed == playdit ? playdah : playdit;
      break;     
   }
      
   if(*state == playdit || *state == playdah){
      setKey(true);
      keyStateUsed = true;
   }
   else
   {
    lastPlayed = playidle;
   }
   /*
// clear state if paddles released
  if(kState!=idle && !ditPaddle.pressed && !dahPaddle.pressed){
    if((iMode == iambicB) && extraChar){
      extraChar = false;
    }
    else{
      clearKeyState();
      
    }
  }
  */
  return (*state != playidle);
}
void clearKeyState()
{
   kState = idle;
   lastPlayed = playidle;
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  for (int i=0; i<BUTTON_PIN_COUNT; ++i) {
    pinMode(buttonPins[i], INPUT);
    digitalWrite(buttonPins[i], HIGH);
    buttonDown[i] = isButtonDown(buttonPins[i]);
  }
  lastKeyChange = millis();

  ditPaddle.analogOffset = ADCTouch.read(ditPaddle.analogPin);
  dahPaddle.analogOffset = ADCTouch.read(dahPaddle.analogPin);
  MIDI.begin(MIDI_CHANNEL_OMNI); 

}

void loop() {
  //Handle USB communication
  
  MIDI.read();

  // straight key or keyer input sent directly to midi
  bool down = isButtonDown(KEY_PIN);
  if(down != lastKey)
  {
    setKey(down);
    lastKey=down;
  }

  now = millis();
  
  unsigned long tdelta = now - lastKeyChange;

  // play keyer output
  switch(pState){
    case playdit:
      if(tdelta>= ditTime){
          pState = playspace;
          setKey(false);
          lastPlayed = playdit;
      }
      break;
    case playdah:
      if(tdelta>= ditTime*3){
          pState = playspace;
          setKey(false);
          lastPlayed = playdah;
      }
      break;
    case playspace:
      if(tdelta>= ditTime){
          if(!getNextState(&pState)){
            pState=playletterSpace;
          }
      }
      break;
    case playletterSpace:
      if(tdelta>= ditTime*3){
        if(!getNextState(&pState)){
          pState=playwordSpace;
        }
      }
      break;
    case playwordSpace:
      if(tdelta>= ditTime*7){
        if(!getNextState(&pState)){
          pState=playidle;
        }
      }
      break;
    case playidle:
      getNextState(&pState);
      break;
  }
  
  // check for paddle changes
  if(readPaddle(&ditPaddle))
  {
    if(ditPaddle.pressed){
      if(dahPaddle.pressed){
        kState = dahdit;
        extraChar = true;
      }
      else kState = dit;
      keyStateUsed = false;
    }
  }
  
  if(readPaddle(&dahPaddle))
  {
    if(dahPaddle.pressed){
      if(ditPaddle.pressed){
        kState = ditdah;
        extraChar = true;
      }
      else kState = dah;
      keyStateUsed = false;
    }
  }
  if((!ditPaddle.pressed)  && (!dahPaddle.pressed) && keyStateUsed && kState != idle){
    clearKeyState();
  }

}
