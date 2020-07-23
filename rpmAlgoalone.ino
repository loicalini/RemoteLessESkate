#include <buffer.h>
#include <crc.h>
#include <datatypes.h>
#include <VescUart.h>
#include <LiquidCrystal_I2C.h>


float averageCurrent(float array[], int numValues);
int averageRPM(int array[], int numValues);

const int buttonOne = 22;
const int buttonTwo = 23;

//1 millisecond = 1000 microseconds
unsigned long debounceGate; //current TIMER FOR GATE
unsigned long debounceGateThreshold = 1000; //  the delay for the debounce loop in microseconds 
int debounceLoopCounter; //the debounce loop will need to be engaged a certain amount of times
bool footFront;            
bool footBack;
int footFrontChangeCounter;
int footBackChangeCounter;
bool footFrontTempHolder;
bool footBackTempHolder;
int state;

bool footBothLCD;
bool footOneLCD;
bool footOffLCD;

const int numInputCurrentValues = 10; //number of previous input current values that will be averaged
float prevCoastingInputCurrent;
float currentCoastingInputCurrent;
float coastingInputCurrent[numInputCurrentValues] = {0};
int coastingInputCurrentCounter;

const int numMotorCurrentValues = 10; //number of previous motor current values that will be averaged
float prevCoastingMotorCurrent;
float currentCoastingMotorCurrent;
float coastingMotorCurrent[numMotorCurrentValues] = {0};
int coastingMotorCurrentCounter;

int tempHolderRPM;

const int numRPMValues = 10; //number of previous RPM values that will be averaged
int currentRPM;
int prevRPM;
int coastingRPM[numRPMValues] = {0};
int coastingRPMCounter;

float tempHolderDuty;

const int numDutyValues = 7; //number of previous motor current values that will be averaged
float prevDuty;
float currentDuty;
float coastingDuty[numDutyValues] = {0};
int coastingDutyCounter;



unsigned long warningTime;
bool warningState;
bool footOffState;

/** Initiate VescUart class */
VescUart UART;
LiquidCrystal_I2C lcd(0x27,20,4);

 
void setup() {

  Serial.begin(9600);
  Serial1.begin(115200);
  
  while (!Serial) {;}

  /** Define which ports to use as UART */
  UART.setSerialPort(&Serial1);

  pinMode(buttonOne, INPUT);
  pinMode(buttonTwo, INPUT);
  digitalWrite(buttonOne, HIGH);  
  digitalWrite(buttonTwo, HIGH);
  
  lcd.init();
  // Print a message to the LCD.
  lcd.backlight();
  lcd.setCursor(1,0);
  lcd.print("Welcome Onboard,");
  lcd.setCursor(1,1);
  lcd.print("Ride Safe! :P");
  delay(3000);
  lcd.clear();



  //lcd.setCursor(0,0);
  //lcd.print("State=");
  lcd.setCursor(6,0);
  lcd.print("Ready...");
  lcd.setCursor(0,1); 
  lcd.print(UART.data.rpm);
  lcd.setCursor(6,1);
  lcd.print("RPM"); 
//  lcd.setCursor(10,1);
//  lcd.print(UART.data.avgInputCurrent);
//  lcd.setCursor(14,1);
//  lcd.print(" i"); 
   


  while((digitalRead(buttonOne)+digitalRead(buttonTwo)) == 2){;}
  footFront = footBack = 1;
  // initialize timers
  debounceGate = micros();

  // SETUP OVER
}

void loop() {
  //DEBOUNCING - input boolean needs to be a constant for 100 milliseconds in order to change ------------------------------

  debounceLoopCounter = 0;
  while (debounceLoopCounter < 100){  
    if ((micros()-debounceGate)>debounceGateThreshold){
       
      debounceGate = micros();
      footFrontTempHolder = digitalRead(buttonOne);
      footBackTempHolder = digitalRead(buttonTwo);
        
          
     
      if (footFrontTempHolder != footFront){ //is there a state change?
        footFrontChangeCounter = footFrontChangeCounter + 1;
        
      }
      else{ //there is no definative state change
        footFrontChangeCounter = 0;
      }
    
      if (footFrontChangeCounter > 100){footFront = !footFront;} //change state
    
    
    
    
      if (footBackTempHolder != footBack){ //is there a state change?
        footBackChangeCounter = footBackChangeCounter + 1;
      }
      else{ //there is no definative state change
        footBackChangeCounter = 0;
      }
    
      if (footBackChangeCounter > 100){footBack = !footBack;} //change state
  
      debounceLoopCounter = debounceLoopCounter + 1;      
  
    }
   
  }
 
  //DEBOUNCING END------------------------------------------------------------
    
    //unsigned long VERYTEMPTIMER = micros();
    state = footFront+footBack;
    UART.getVescValues();
  

    
    //Averaging for Input Current
    if (coastingInputCurrentCounter == numInputCurrentValues){coastingInputCurrentCounter = 0;}
    coastingInputCurrent[coastingInputCurrentCounter] = UART.data.avgInputCurrent;
    coastingInputCurrentCounter = coastingInputCurrentCounter + 1;
    currentCoastingInputCurrent = averageCurrent(coastingInputCurrent, numInputCurrentValues);

    //Averaging for Motor Current
    if (coastingMotorCurrentCounter == numMotorCurrentValues){coastingMotorCurrentCounter = 0;}
    coastingMotorCurrent[coastingMotorCurrentCounter] = UART.data.avgMotorCurrent*(UART.data.avgMotorCurrent>0);
    coastingMotorCurrentCounter = coastingMotorCurrentCounter + 1;
    currentCoastingMotorCurrent = averageCurrent(coastingMotorCurrent, numMotorCurrentValues);

    //Averaging for RPM
    if (coastingRPMCounter == numRPMValues){coastingRPMCounter = 0;}
    coastingRPM[coastingRPMCounter] = abs(UART.data.rpm);
    coastingRPMCounter = coastingRPMCounter + 1;
    currentRPM = averageRPM(coastingRPM, numRPMValues);

    //Averaging for Motor Current
    if (coastingDutyCounter == numDutyValues){coastingDutyCounter = 0;}
    coastingDuty[coastingDutyCounter] = abs(UART.data.dutyCycleNow);
    coastingDutyCounter = coastingDutyCounter + 1;
    currentDuty = averageCurrent(coastingDuty, numDutyValues);

    
    footOffState = warningState && ((micros()-warningTime) > 1000000); //one second
      lcd.setCursor(0,1);
                  lcd.print("       "); 
                  lcd.setCursor(10,1);
                  lcd.print("       "); 
 
    switch(state)
      {
          case 0: //BOTH FEET ON
              UART.setDuty(tempHolderDuty, 70);
              UART.setDuty(tempHolderDuty);
              
              if (!footBothLCD){
                lcd.setCursor(6,0);
                lcd.print("Both     ");
                footBothLCD = 1;
                footOneLCD = footOffLCD = 0;              
              }
              
              warningState = 0;
              break;
  
          case 1: //ONE FOOT ON;
                // so far there will be three states with one foot on, speeding up, staying the same, slowing down..
             // implement maximum acceleration case
            
                // implement minimum rpm, or else it equals 0
                tempHolderDuty = currentDuty;
               
                    UART.setCurrent(0.00, 70);
                  UART.setCurrent(0.00);
                 
                
  
              
             if (!footOneLCD){
                lcd.setCursor(6,0);
                lcd.print("One     ");
                footOneLCD = 1;
                footBothLCD = footOffLCD = 0;              
              }      
              
              warningState = 0;
              break;
  
          case 2: //BOTH FEET OFF
              
              if (warningState == 0){  
                
                lcd.setCursor(6,0);
                lcd.print("Warning!!");
                footBothLCD = footOneLCD = 0;   
                warningTime = micros();
                
                warningState = 1;           
              }
              if (footOffState)  {
                
                if (!footOffLCD){
                  UART.setCurrent(0.00, 70);
                  UART.setCurrent(0.00);
                  lcd.setCursor(0,0);
                  lcd.print("0.00  ");
                  lcd.setCursor(6,0);
                  lcd.print("Off      ");
                  lcd.setCursor(0,1);
                  lcd.print("       "); 
                  lcd.setCursor(10,1);
                  lcd.print("       "); 
                  footOffLCD = 1;
                  footBothLCD = footOneLCD = prevRPM = prevCoastingMotorCurrent = tempHolderDuty = 0;
                  //setNewValue(coastingRPM, numRPMValues, 0);
                     
                }   
        
              }
              else{
               // UART.setDuty(tempHolderDuty, 70);
                //UART.setDuty(tempHolderDuty);  
              }

              break;
                  
    
              
      }
    
      
             lcd.setCursor(0,1);
            lcd.print(tempHolderDuty);
            lcd.setCursor(10,1);
            lcd.print(currentDuty);

              lcd.setCursor(0,0);
              lcd.print(currentCoastingMotorCurrent);
              
}



float averageCurrent(float array[], int numValues){
  float sum = 0;
  int i = 0;
  for (i = 0; i < numValues; i++) {
    sum = sum + array[i];
  }
  return sum/ (float) numValues;
}


int averageRPM(int array[], int numValues){
  int sum = 0;
  int i = 0;
  for (i = 0; i < numValues; i++) {
    sum = sum + array[i];
  }
  return sum / numValues;
}

void setNewValue(int *array, int numValues, int newValue){
  int i = 0;
  for (i = 0; i < numValues; i++) {
    array[i] = newValue;
  }
}


void setNewCurrent(float *array, int numValues, float newValue){
  int i = 0;
  for (i = 0; i < numValues; i++) {
    array[i] = newValue;
  }
}

