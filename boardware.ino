
#include <MsTimer2.h>

//Row Column pin numbers
#define NUM_ROWS 5
#define NUM_COLS 16
//number of iterations of identical keyscan values before we trigger a keypress
#define DEBOUNCE_ITER 5 
//milliseconds between each scan. SCAN_PERIOD * DEBOUNCE_ITER = minimum response time
#define SCAN_PERIOD 3

#define KEY_RELEASED 1
#define KEY_PRESSED 0

#define KEY_FUNCTION -1
#define NOK 0
#define KC_NONUS_BACKSLASH        ( 100  | 0xF000 )
#define KC_NONUS_HASH             (  50  | 0xF000 )

byte rowPins[NUM_ROWS] = {13,21,20,18,19};
byte colPins[NUM_COLS] = {12,11,10,9,8,7,6,5,4,3,2,1,17,16,15,14};
byte keyIterCount[NUM_ROWS][NUM_COLS];
byte keyState[NUM_ROWS][NUM_COLS];        
int keyMap[NUM_ROWS][NUM_COLS] = {
        {KEY_ESC,KEY_1,KEY_2,KEY_3,KEY_4,KEY_5,KEY_6,KEY_7,KEY_8,KEY_9,KEY_0,KEY_MINUS,KEY_EQUAL,KEY_BACKSPACE,KEY_INSERT,KEY_HOME},
        {KEY_TAB,KEY_Q,KEY_W,KEY_E,KEY_R,KEY_T,KEY_Y,KEY_U,KEY_I,KEY_O,KEY_P,KEY_LEFT_BRACE,KEY_RIGHT_BRACE,NOK,KEY_DELETE,KEY_END},
        {KEY_CAPS_LOCK,KEY_A,KEY_S,KEY_D,KEY_F,KEY_G,KEY_H,KEY_J,KEY_K,KEY_L,KEY_SEMICOLON,KEY_QUOTE,KC_NONUS_HASH,KEY_ENTER,KEY_FUNCTION,KEY_PAGE_UP},
        {MODIFIERKEY_SHIFT,KC_NONUS_BACKSLASH,KEY_Z,KEY_X,KEY_C,KEY_V,KEY_B,KEY_N,KEY_M,KEY_COMMA,KEY_PERIOD,KEY_SLASH,MODIFIERKEY_RIGHT_SHIFT,NOK,KEY_UP,KEY_PAGE_DOWN},
        {MODIFIERKEY_CTRL,MODIFIERKEY_GUI,MODIFIERKEY_ALT,NOK,NOK,NOK,KEY_SPACE,NOK,NOK,NOK,MODIFIERKEY_RIGHT_ALT,KEY_FUNCTION,MODIFIERKEY_RIGHT_CTRL,KEY_LEFT,KEY_DOWN,KEY_RIGHT}                                 
};

int funcKeyMap[NUM_ROWS][NUM_COLS] = {
        {KEY_TILDE,KEY_F1,KEY_F2,KEY_F3,KEY_F4,KEY_F5,KEY_F6,KEY_F7,KEY_F8,KEY_F9,KEY_F10,KEY_F11,KEY_F12,KEY_BACKSPACE,KEY_PRINTSCREEN,KEY_SCROLL_LOCK},
        {KEY_TAB,NOK,KEY_UP,NOK,NOK,NOK,NOK,KEY_INSERT,KEY_HOME,KEY_PAGE_UP,NOK,NOK,NOK,NOK,KEY_DELETE,KEY_END},
        {NOK,KEY_LEFT,KEY_DOWN,KEY_RIGHT,NOK,NOK,NOK,KEY_DELETE,KEY_END,KEY_PAGE_DOWN,NOK,NOK,NOK,KEY_ENTER,KEY_FUNCTION,KEY_PAGE_UP},
        {MODIFIERKEY_SHIFT,NOK,NOK,NOK,NOK,NOK,NOK,NOK,NOK,NOK,NOK,NOK,MODIFIERKEY_RIGHT_SHIFT,NOK,KEY_UP,KEY_PAGE_DOWN},
        {MODIFIERKEY_CTRL,MODIFIERKEY_GUI,MODIFIERKEY_ALT,NOK,NOK,NOK,KEY_SPACE,NOK,NOK,NOK,MODIFIERKEY_RIGHT_ALT,KEY_FUNCTION,MODIFIERKEY_RIGHT_CTRL,KEY_LEFT,KEY_DOWN,KEY_RIGHT}                                 
};

boolean funcMode = false;

void setup() {
  Serial.begin(9600);
  
  for(int row=0; row < NUM_ROWS; row++) {
        pinMode(rowPins[row], INPUT);
  } 
  
  for (int col=0; col < NUM_COLS; col++) {
        pinMode(colPins[col], INPUT_PULLUP);
  }

  //set the initial values on the iter count and state arrays.
  for (int row = 0; row < NUM_ROWS; row++) {
    for (int col = 0; col < NUM_COLS; col++) {
      //initial iter value is debounce + 1 so that a key transition isn't immediately detected on startup.
      keyIterCount[row][col] = DEBOUNCE_ITER + 1;
      keyState[row][col] = KEY_RELEASED;
    }
  }

  Keyboard.begin();
  
  MsTimer2::set(SCAN_PERIOD,keyScan);
  MsTimer2::start();
}

//Pressing the FN key could potentially shift the scan code between the key being pressed 
//and being released. If the FN key is hit then any pressed keys have to be reset to be in the 
//'released' state and their iter counts set to DEBOUNCE_ITER+1. 
//Quick improvement: Only do this if the scan codes are different in the two maps. This means that 
//any keys that are the same between the layers like the modifiers will remain pressed.

void resetKeyStates(bool funcMode) {
  //set the initial values on the iter count and state arrays.
  for (int row = 0; row < NUM_ROWS; row++) {
    for (int col = 0; col < NUM_COLS; col++) {
      //only reset if they're different on the two layers.
      if(keyMap[row][col] != funcKeyMap[row][col]) {
        keyIterCount[row][col] = DEBOUNCE_ITER + 1;
        //if it's PRESSED then we have to 'release' the relevent map code, and 'press' the other one
        //if we're switching TO the func mode then we want to release the keymap code, and press the func keymap code
        //if we're switching to normal mode FROM the func mode then release the func keymap code and press the keymap code.
        if(keyState[row][col] == KEY_PRESSED) {
            Keyboard.release(funcMode ? funcKeyMap[row][col] : keyMap[row][col]);
            Keyboard.press  (funcMode ? keyMap[row][col] : funcKeyMap[row][col]);
        }       
      }
    }
  }  
}

//we have a debounced key transition event, either pressed or released.
void transitionHandler(int state, boolean fnMode, int row, int col) {
  //pick which keyMap we're using based on whether we're in func mode
  int scanCode = fnMode ? funcKeyMap[row][col] : keyMap[row][col];
  if(state == KEY_PRESSED) {
    Keyboard.press(scanCode);
  } else if (state == KEY_RELEASED) {
    Keyboard.release(scanCode);
  }
}


//Scan handler, runs in interrupt context, triggered by msTimer
void keyScan() {
  //set the scanFuncMode to false so we can check for any of the FN keys being pressed
  bool scanFuncMode = false;
  //First loop runs through each of the rows,
  for (int row=0; row < NUM_ROWS; row++) {
        //for each row pin, set to output LOW 
        pinMode(rowPins[row], OUTPUT);
        digitalWrite(rowPins[row], LOW);
        
        //now iterate through each of the columns, set to input_pullup, 
        //the Row is output and low, and we have input pullup on the column pins,
        //so a '1' is an un pressed switch, and a '0' is a pressed switch.
        for (int col=0; col < NUM_COLS; col++) {
            byte value = digitalRead(colPins[col]);
            //if the value is different from the stored value, then reset the count and set the stored value.
            if(value == KEY_PRESSED && keyState[row][col] == KEY_RELEASED) {
              keyState[row][col] = KEY_PRESSED;
              keyIterCount[row][col] = 0;
            } else if (value == KEY_RELEASED && keyState[row][col] == KEY_PRESSED) {
              keyState[row][col] = KEY_RELEASED;
              keyIterCount[row][col] = 0;
            } else {
              //Stored value is the same as the current value, this is where our debounce magic happens.
              //if the keyIterCount < debounce iter then increment the keyIterCount and move on
              //if it's == debounce iter then trigger the key & increment it so the trigger doesn't happen again.
              //if it's > debounce iter then we do nothing, except check for the FN key being pressed.
              if(keyIterCount[row][col] < DEBOUNCE_ITER) {
                keyIterCount[row][col] ++;
              } else if (keyIterCount[row][col] == DEBOUNCE_ITER) {
                keyIterCount[row][col] ++;
                transitionHandler(keyState[row][col], funcMode, row, col);
              } else {
                //We'll check for the func activation here, we just want to check if
                //any of them are pressed. If any are then set the scan func var to 
                //true. It starts as false so if there aren't any func keys pressed
                //it'll remain as false. At the end of the scan we just set the global
                //func bool to whatever the scan local one is.

                //we'll just check the base layer to avoid in sticky situations
                if(keyMap[row][col] == KEY_FUNCTION && keyState[row][col] == KEY_PRESSED) {
                  scanFuncMode = true;
                }
              }
            }
        }
        //now just reset the pin mode (effectively disabling it)
        pinMode(rowPins[row], INPUT);
  }
  //lastly if the scanFuncMode is different from the global func mode, then do the keystate reset, 
  //and set the global func mode to the scan func mode.
  if(funcMode != scanFuncMode) {
    resetKeyStates(funcMode);
    funcMode = scanFuncMode;
  }
}  

void loop() {
}
