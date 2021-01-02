//Rows and Columns, and pin mapping
#define NUM_ROWS 5
#define NUM_COLS 16

unsigned char rowPins[NUM_ROWS] = {13,21,20,18,19};
unsigned char colPins[NUM_COLS] = {12,11,10,9,8,7,6,5,4,3,2,1,17,16,15,14};


void setup() {
  Serial.begin(9600);

  //set the row pins to 'input' which is the default 'disabled' state
  for(int row=0; row < NUM_ROWS; row++) {
        pinMode(rowPins[row], INPUT);
  } 
  
  for (int col=0; col < NUM_COLS; col++) {
        pinMode(colPins[col], INPUT_PULLUP);
  }
}

void loop() {
  delay(100);                  // wait for a 1/10th second
  
  //First loop runs through each of the rows,
  for (int row=0; row < NUM_ROWS; row++) {
        //for each row pin, set to OUTPUT and LOW 
        pinMode(rowPins[row], OUTPUT);
        digitalWrite(rowPins[row], LOW);

        //now iterate through each of the columns, set to input_pullup, 
        //the Row is output and low, and we have input pullup on the column pins,
        //so a on a digital read, 
        //'1' is an unpressed switch, and a '0' is a pressed switch.        
        for (int col=0; col < NUM_COLS; col++) {
            byte value = digitalRead(colPins[col]);
            if (value != 1) {
              //if it's pressed, write it out to the serial link
              Serial.print("[");
              Serial.print(col);
              Serial.print(":");
              Serial.print(row);
              Serial.print("]");
            }
        }
        //now just reset the pin mode (effectively disabling it)
        pinMode(rowPins[row], INPUT);
  }
  //one complete scan done, write out on serial...
  Serial.println("Scanned");  
}