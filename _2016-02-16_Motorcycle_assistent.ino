
//**** INCLUDES
//* include for Screen
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
//*

//* include for DS18B20 temperature sensor
#include <OneWire.h>
//

//* include for DS3231 clock chip
#include <DS3231.h>
//*

//**** DEFINES
//* define for Screen
//#define OLED_RESET 4 // not used here/ nicht genutzt bei diesem Display
Adafruit_SSD1306 display(0); // the value is not important
//#define DRAW_DELAY 118
//#define D_NUM 47
//*

//* define for DS18B20 temperature sensor
#define DS18B20_PIN 2 // PIN for temp sensor
//*

//* define for the voltageDeviderPins and resistorValue
#define VOLTAGE_PIN A0
#define VOLTAGE_RESISTOR1 9400
#define VOLTAGE_RESISTOR2 4700
//*

//* define for the buttonPins
#define BUTTON_1 A1
#define BUTTON_2 A2

#define BUTTON_TRIGGER_VALUE 600 // if analogRead returns a value above this value, the button will be triggered
//*

//* define for the CHAIN_OILER_PIN
#define CHAIN_OILER_PIN 6
//*

//* Logsettings
#define GBL_LOGLEVEL 0
#define VERBOUS_LOG 0
#define INFO_LOG 1
#define WARNING_LOG 2
#define ERROR_LOG 3
//*

//**** GLOBAL VARIABLES
//* timing variables
// Main display refresh
unsigned long timeSinceMainDisplayRefresh = 0;  //
#define intervalMainDisplayRefresh 1000 // to keep it together, define is placed here. Using define to save ram

// Main button check
// The check is triggered every 100 ms and counts up a value if the button is pressed
// At for example 10 counts a action is triggered. So you need to hold the button down for 1 
unsigned long timeSinceMainButtonCheck = 0;
#define INTERVAL_MAIN_BUTTON_CHECK 100
byte mainButtonCounter = 0;
#define MAIN_BUTTON_COUNTER_TRIGGER 10

//* DS18B20 temperatur sensor
OneWire DS18B20ds(DS18B20_PIN);
byte DS18B20addr[8];

//* DS3231 clock chip
// define the PINS for the clock.
DS3231 DS3131rtc(A4,A5); // first SDA_PIN, second SCL_PIN on DS3231

//* chain oiler timings
unsigned long chain_oiler_wait = 600000;  // unsigned long else overflow. standard 10 minutes (600000 ms)
unsigned int chain_oiler_pump_time = 100;  // how long the pump should run in one intervall
unsigned long timeSinceChainOiler = 0;
double chain_oiler_trigger_voltage = 13.2; // needs to be tested. Determines if the engine is running

//**** setup code
void setup()   {

  //* initialize Serial connection
  Serial.begin(250000);
  //*

  //* initialize Display
  // initialize with the I2C addr 0x3C / mit I2C-Adresse 0x3c initialisieren
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  //*

  //* initialize DS18B20 temperature sensor
  if (!DS18B20ds.search(DS18B20addr)) {
    if(ERROR_LOG >= GBL_LOGLEVEL) {
      Serial.println(F("E: Tempsensor not found"));
    }
  } else {

    if(INFO_LOG >= GBL_LOGLEVEL) {
      Serial.print(F("temp sensor is good addr: "));
      for(byte i = 0; i < 8; i++) {
        Serial.print(DS18B20addr[i],HEX);
        Serial.print(' ');
      }
      Serial.print('\n');
    }
  }
  //* initialize DS2131 clock chip
  DS3131rtc.begin();
  //*

  timeSinceChainOiler = millis(); // skip the first run of the chain oiler.
                                  // Nobody wants to spill oil in the garage
}

//**** loop code
void loop() {

// chain oiler routine
if(timeSinceChainOiler + chain_oiler_wait <= millis()) {
  timeSinceChainOiler = millis();
  if (readVoltage() >= chain_oiler_trigger_voltage) {
    triggerChainOiler(chain_oiler_pump_time);
  } else {
    if (INFO_LOG >= GBL_LOGLEVEL) {
      Serial.print(F("I: Chain oiler, voltage ("));
      Serial.print(readVoltage());
      Serial.println(F(") to low"));
    }
  }
}

// main display update routine
if (millis() - timeSinceMainDisplayRefresh >= intervalMainDisplayRefresh) {
  timeSinceMainDisplayRefresh = millis();  // reset it here to avoid the 50 ms delay of the display
  
  display.clearDisplay();
  display.setTextColor(WHITE);
  
  // display temperatur
  display.setTextSize(2);
  display.setCursor(0,0);
  display.println(String(readTemperatur(),1) + "C");     // takes about 23 - 24 ms

  // display voltage
  display.setTextSize(2);
  String voltageTempString = String(readVoltage(),1)+"V";  //  takes about 3 - 4 ms
  if (voltageTempString.length() == 4) {
    display.setCursor(80,0);
  } else {
    display.setCursor(68,0);
  }                  
  display.println(voltageTempString);

  // display date
  display.setTextSize(1);
  display.setCursor(0,25);
  display.println(DS3131rtc.getDateStr());  // takes about 3 - 4 ms

  // display time
  display.setCursor(80,25);
  String tempTimeString = DS3131rtc.getTimeStr();
  display.println(tempTimeString);  // takes about 3 - 4 ms

  byte tempHourArray[2];
  tempTimeString.getBytes(tempHourArray,3); // this is somehow weird... with 2 instead of 3 the second digit does not get read

  // check if the display need to be dimmed
  // ASCII
  // 48 == 0
  // 49 == 1 ...
  if(tempHourArray[0] == 50 && tempHourArray[1] >= 48 && tempHourArray[1] <= 52) { 
    // if first digit of the current hour is 2
    // and second digit of the current hour is between 0 and 4 (keep it easy to edit)
    if (VERBOUS_LOG >= GBL_LOGLEVEL) {
      Serial.println(F("V: Display dimmed"));
    }
    display.dim(true); // dim display

  } else if(tempHourArray[0] == 48 && tempHourArray[1] >= 48 && tempHourArray[1] <= 56) {
     // if first digit of the current hour is 0
     // if second digit of the current hour is between 0 and 8 (keep it easy to edit)
     if(VERBOUS_LOG >= GBL_LOGLEVEL) {
      Serial.println(F("V: Display dimmed"));
     }
     display.dim(true); // dim display
  } else {
    if(VERBOUS_LOG >= GBL_LOGLEVEL) {
      Serial.println(F("V: Display not dimmed"));
    }
    display.dim(false);    
  }

  /*if(byte(tempTimeString.substring(0,1)) > 20 || byte(tempTimeString.substring(0,1)) < 8) {
    display.dim(true);
  } else {
    display.dim(false);
  } */
  
  display.display();
}

// button check routine
// counts up if button is presse
// switches to menu if counts hit a certain level

if (millis() - timeSinceMainButtonCheck >= INTERVAL_MAIN_BUTTON_CHECK) {
  timeSinceMainButtonCheck = millis();
  
  // the button needs to pressed for a certain time before a action is triggerd (to prevent false input)
  if(analogRead(BUTTON_1) > BUTTON_TRIGGER_VALUE) {
    mainButtonCounter++;

    // if trigger is reached enter menu
    if(mainButtonCounter >= MAIN_BUTTON_COUNTER_TRIGGER) {
      if(INFO_LOG >= GBL_LOGLEVEL) {
        Serial.println(F("I: Entering Menu"));
      }
      enterMenuPage();
      mainButtonCounter= 0;  // resseting the counter
    }
    
  } else {
    mainButtonCounter = 0;
  }
}

      //Serial.println(analogRead(BUTTON_1));

      //delay(1000);
  /*if (displayTimerCompareValue == displayTimer)
  {

      display.clearDisplay();
      display.setTextColor(WHITE);
      display.setTextSize(3);
      display.setCursor(1,0);
      display.dim(true);
      display.display();
      displayTimerCompareValue = 0;
  }
  else
  {
    displayTimerCompareValue++;
  }


  PWMPowerValue++;*/

}

// MENU TASK
// this displays the menu for the user
// Nothing else will and should happen if the user is in the menu.
// Everything is packed into one methode to save space in RAM and Flash
// It takes quite a amount of extra stuff to draw and control the menu
// So, not that pretty stuff here, but it's functional
void enterMenuPage() {
  boolean exitMenu = false;

  // 0 first page
  // 10 chain oiler
  // 20 clock 
  byte menuTyp = 0;
  byte menu_item_count = 3;
  byte menu_Pointer_Location = 0;


  // Menu defines.
  // Not at the top, because a user does not really needs to change this values
  #define MENU_POINTER_GAB 8
  #define MENU_ENTRY_HEIGHT 8
  #define MENU_FIRST_ENTRY 8
  #define MENU_OFFSET_MOVE_TRIGGER 2  // defines when the menu should start to shift the text upwards

  byte menuOffset = 0;  // moves the hole menu up to display new stuff. Dealing with the small display


  // Write a reminder message, to not use the menu while driving
  display.clearDisplay();
  display.dim(false);

  display.setCursor(0,0);
  display.setTextSize(1);
  display.println(F("BEDIENUNG WAEHREND\nDER FAHRT VERBOTEN"));
  display.setCursor(0,17);
  display.println(F("Oeler im Menue \n deaktiviert"));

  display.display();

  while(analogRead(BUTTON_1) >= BUTTON_TRIGGER_VALUE) {} // wait for user to release button

  // printig the main menu in a loop and checking buttons
  while(!exitMenu){

  // printing section
    switch(menuTyp) {
      case 0:   // main menu      
      display.clearDisplay();
      // Headline
      display.setCursor(0,0 - menuOffset);
      display.setTextSize(1);
      display.println(F("Hauptmenue"));
    
      // Menu points
      display.setTextSize(1);
      display.setCursor(MENU_POINTER_GAB,MENU_FIRST_ENTRY + MENU_ENTRY_HEIGHT * 0 - menuOffset);
      display.print(F("Kettenoeler"));
    
      display.setCursor(MENU_POINTER_GAB,MENU_FIRST_ENTRY + MENU_ENTRY_HEIGHT * 1 - menuOffset);
      display.print(F("Uhrzeit"));
    
      display.setCursor(MENU_POINTER_GAB,MENU_FIRST_ENTRY + MENU_ENTRY_HEIGHT * 2 - menuOffset);
      display.print(F("Zurueck"));
 
      break;

      case 10:  // chain oiler menu
      display.clearDisplay();

      // Headline
      display.setCursor(0,0 - menuOffset);
      display.setTextSize(1);
      display.print(F("Kettenoeler"));
  
      display.setCursor(84,0);
      display.print(chain_oiler_wait/1000);
      display.print('s');     
  
       // Menu points
      display.setTextSize(1);
      display.setCursor(MENU_POINTER_GAB,MENU_FIRST_ENTRY + MENU_ENTRY_HEIGHT * 0 - menuOffset);
      display.print(F("Erhoehen (30s)"));
    
      display.setCursor(MENU_POINTER_GAB,MENU_FIRST_ENTRY + MENU_ENTRY_HEIGHT * 1 - menuOffset);
      display.print(F("Absenken (30s)"));
    
      display.setCursor(MENU_POINTER_GAB,MENU_FIRST_ENTRY + MENU_ENTRY_HEIGHT * 2 - menuOffset);
      display.print(F("Zurueck"));
      break;
    }

    // print the menu pointer
    display.setCursor(0, MENU_FIRST_ENTRY + menu_Pointer_Location * MENU_ENTRY_HEIGHT - menuOffset);
    display.print((char)175);

    display.display();

  // action section
  // get the input from the user
    unsigned int button_1_hold = getButtonHoldTime(BUTTON_1);
    // first find out if the button was a long press
    // reusing the predefined value for the button long press trigger (100ms * 10ms) = 1000 ms
    if(button_1_hold >= INTERVAL_MAIN_BUTTON_CHECK * MAIN_BUTTON_COUNTER_TRIGGER) {
      switch(menuTyp) {
        case 0:   // main menu
        switch(menu_Pointer_Location) {
          case 0:
          menuTyp = 10;
          menu_item_count = 3;
          menu_Pointer_Location = 0;
          break;
          case 1:
          break;
          case 2:
          exitMenu = true;
          break;
        }        
        break;

        case 10:   // chain oiler menu
        switch(menu_Pointer_Location) {
          case 0:
          chain_oiler_wait += 30000;
          menu_item_count = 3;
          break;
          case 1:
          break;
          case 2:
          menuTyp = 0;
          menu_item_count = 3;
          menu_Pointer_Location = 0;
          break;
        }     
        break;
      }

      // else check for a short press
    } else if (button_1_hold >= 50) {
      menu_Pointer_Location++;    // increase the menupointer
      if (menu_Pointer_Location >= menu_item_count){  // check if the menuPointer is over the actual count of the menuItems
        // keep in mind that "human" counting starts at 1, but the menu_Pointer_Location starts at 0
        menu_Pointer_Location = 0;     
      }
    }

    // calculate the menuOffset to move the complete menu upwards, showing new stuff at the buttom
    if (menu_Pointer_Location >= MENU_OFFSET_MOVE_TRIGGER) {
        menuOffset = (menu_Pointer_Location - 1) *MENU_ENTRY_HEIGHT;
    } else {
      menuOffset = 0;
    }
  }

  // display a good bye message
  display.clearDisplay();
  display.setCursor(0,0);
  display.setTextSize(2);
  display.print(F("Komme gut\nan dein Ziel"));

  display.display();

  while(analogRead(BUTTON_1) >= BUTTON_TRIGGER_VALUE) { } // waiting for the user to release the button
}

// READ TEMPERATUR TASK
// adapted from https://edwardmallon.wordpress.com/2014/03/12/using-a-ds18b20-temp-sensor-without-a-dedicated-library/
// this version does NOT work with the parasite mod. Big benefit here: no delay
// reduced code to a minimum. No options.
// Just read the value and convert it to float. For what do we need the Dalas lib again ;)?
float readTemperatur() {
  byte data[12];
  DS18B20ds.reset();
  DS18B20ds.select(DS18B20addr);
  DS18B20ds.write(0x44); // start process, read and store temperatur

  // delay(750) is only needed if used in parasite mode
  DS18B20ds.reset();
  DS18B20ds.select(DS18B20addr);
  DS18B20ds.write(0xBE); // read stored value

  for(int i = 0; i < 12; i++) {
    data[i] = DS18B20ds.read();
  }

  unsigned int rawTemp = (data[1] << 8) | data [0];
  float floatTemp = (float) rawTemp / 16.0;

  if (VERBOUS_LOG >= GBL_LOGLEVEL) {
    Serial.print(F("V: read temp: "));
    Serial.println(floatTemp);
  }    
  return floatTemp;
}

// READ VOLTAGE TASK
// reads the voltage and does the voltage divider math
// Pin_Voltage / (R2 / (R1 + R2))  (there are no unnecessary brackets ;)  )
// connect voltagePin to point between R1 and R2
// R1 is connected to positiv and R2, R2 is connected to R1 and GND
float readVoltage() {
  
  float floatVoltage = (((float)analogRead(VOLTAGE_PIN)/1024)*5.0)
      /((float)VOLTAGE_RESISTOR2/(VOLTAGE_RESISTOR2+VOLTAGE_RESISTOR1));
  if(VERBOUS_LOG >= GBL_LOGLEVEL) {
    Serial.print(F("V: read voltage: "));
    Serial.println(floatVoltage);
  }

  return floatVoltage;
}

// CHAIN OILER TASK
//It's critical that NOTHING interupts this task.
//So lets do a delay here else this could be interupted and 
//the chain oiler keeps pumping oil (which is NO god ;)  )
void triggerChainOiler(unsigned int openTime) {
  if (INFO_LOG >= GBL_LOGLEVEL) {
    Serial.print(F("I: Chain oiler is on for "));
    Serial.print(openTime);
    Serial.println(F("ms"));
  }
  analogWrite(CHAIN_OILER_PIN, 255);

  delay(openTime);   

  if(INFO_LOG >= GBL_LOGLEVEL){
    Serial.println(F("Chain oiler is off"));
  }
  analogWrite(CHAIN_OILER_PIN,0);
}

// GET BUTTON HOLD TIME TASK
// loops while a given button at buttonPIN is pressed down
// on release it returns the time it looped
unsigned int getButtonHoldTime(byte buttonPIN) {
  unsigned long startTime = millis();
  while (analogRead(buttonPIN) >= BUTTON_TRIGGER_VALUE) {   
    // this could be done more efficient, but why?
    // The arduino always runs at the same speed with the same power usage
    // this loop gets passed VERY often. So no difference here
    if (millis() - startTime > INTERVAL_MAIN_BUTTON_CHECK * MAIN_BUTTON_COUNTER_TRIGGER) { // calculating time for a long click
      return millis() - startTime;  // if the time is already over, why should we count on?
    }
  }

  return millis() - startTime;
}


// LOGOUTPUT
// 0: Verbos
// 1: Info
// 2: Warning
// 3: Error
/*void logOutput(byte level, String msg) {
  if(level >= GBL_LOGLEVEL)
  {
    switch(level){
      case 0:
      msg = "V: " + msg;
      break;      
      case 1:
      msg = "I: " + msg;
      break;      
      case 2:
      msg = "W: " + msg;
      break;      
      case 3:
      msg = "E: " + msg;
      break;      
      default:
      break;
    }
      Serial.println(msg);
  }
}*/
  /*

  display.clearDisplay();

  // set text color / Textfarbe setzen
  display.setTextColor(WHITE);
  // set text size / Textgroesse setzen
  display.setTextSize(1);
  // set text cursor position / Textstartposition einstellen
  display.setCursor(1,0);
  // show text / Text anzeigen
  display.println("OLED - Display - Test");
  display.setCursor(14,56);
  display.println("blog.simtronyx.de");
  display.setTextSize(2);
  display.setCursor(34,15);
  display.println("Hello");
  display.setCursor(30,34);
  display.println("World!");
  display.display();
  delay(1000);


  // invert the display / Display invertieren
  display.invertDisplay(true);
  delay(2000);
  display.invertDisplay(false);
  delay(1000);

  // draw some random pixel / zufaellige Pixel anzeigen
  for(i=0;i<380;i++){
    display.drawPixel(random(128),random(64), WHITE);
    display.display();
  }
  delay(DRAW_DELAY);
  display.clearDisplay();

  // draw some random lines / zufaellige Linien anzeigen
  for(i=0;i<D_NUM;i++){
    display.drawLine(random(128),random(64),random(128),random(64), WHITE);
    display.display();
    delay(DRAW_DELAY);
    display.clearDisplay();
  }

  // draw some random triangles / zufaellige Dreiecke anzeigen
  for(i=0;i<D_NUM;i++){
    if(random(2))display.drawTriangle(random(128),random(64),random(128),random(64), random(128),random(64), WHITE); // normal
    else display.fillTriangle(random(128),random(64),random(128),random(64), random(128),random(64), WHITE);     // filled / ausgefuellt
    display.display();
    delay(DRAW_DELAY);
    display.clearDisplay();
  }

  // draw some random rectangles / zufaellige Rechtecke anzeigen
  for(i=0;i<D_NUM;i++){
    int rnd=random(4);
    if(rnd==0)display.drawRect(random(88),random(44),random(40),random(20), WHITE);               // normal
    else if(rnd==1)display.fillRect(random(88),random(44),random(40),random(20), WHITE);            // filled / ausgefuellt
    else if(rnd==2)display.drawRoundRect(random(88),random(44),random(30)+10,random(15)+5,random(5), WHITE);  // normal with rounded edges / normal mit abgerundeten Ecken
    else display.fillRoundRect(random(88),random(44),random(30)+10,random(15)+5,random(5), WHITE);        // filled with rounded edges / ausgefuellt mit  mit abgerundeten Ecken
    display.display();
    delay(DRAW_DELAY);
    display.clearDisplay();
  }

  // draw some random circles / zufaellige Kreise anzeigen
  for(i=0;i<D_NUM;i++){
    if(random(2))display.drawCircle(random(88)+20,random(44)+20,random(10), WHITE); // normal
    else display.fillCircle(random(88)+20,random(44)+20,random(10), WHITE);     // filled / ausgefuellt
    display.display();
    delay(DRAW_DELAY);
    display.clearDisplay();
  }*/



