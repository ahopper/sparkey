// comment out this line to send midi over serial
#define USEMIDI

#ifdef USEMIDI
#include <midi_serialization.h>
#include <usbmidi.h>
#endif
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

Stream *dataStream;
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

// if both set true, the firat to detect a change will be used
bool useTouch = true;
bool useDigital = true;

paddle ditPaddle = {DIT_PIN, A0, 0, false};
paddle dahPaddle = {DAH_PIN, A1, 0, false};

void sendCC(uint8_t channel, uint8_t control, uint8_t value) {
  dataStream->write(0xB0 | (channel & 0xf));
  dataStream->write(control & 0x7f);
  dataStream->write(value & 0x7f);
}

void sendNoteDown(uint8_t channel, uint8_t note, uint8_t velocity) {
  dataStream->write( 0x90  | (channel & 0xf));
  dataStream->write(note & 0x7f);
  dataStream->write(velocity &0x7f);
}

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
    sendNoteDown(0, 64 , down ? 127:0);
    lastKeyChange = now;
}

void setup() {
 
  for (int i=0; i<BUTTON_PIN_COUNT; ++i) {
    pinMode(buttonPins[i], INPUT);
    digitalWrite(buttonPins[i], HIGH);
    buttonDown[i] = isButtonDown(buttonPins[i]);
  }
  lastKeyChange = millis();

  ditPaddle.analogOffset = ADCTouch.read(ditPaddle.analogPin);
  dahPaddle.analogOffset = ADCTouch.read(dahPaddle.analogPin);
#ifdef USEMIDI
  dataStream = &USBMIDI;
#else  
//  Serial.begin(31250);
    Serial.begin(115200);
  dataStream = &Serial;
  
#endif
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
 //     Serial.write("dah\n");
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
  // Serial.write("clear\n");
 
}

void loop() {
  //Handle USB communication
#ifdef USEMIDI
  USBMIDI.poll();
#endif

  while (dataStream->available()) {
    // We must read entire available data, so in case we receive incoming
    // MIDI data, the host wouldn't get stuck.
    u8 b = dataStream->read();
  }

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

  // Flush the output.
  dataStream->flush(); 
}
