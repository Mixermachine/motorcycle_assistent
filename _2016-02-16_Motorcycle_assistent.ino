
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

// include for EEPROM memory
#include <EEPROM.h>
//*

// include for the power saving sleep library
#include <LowPower.h>
//*

//**** DEFINES
//* define power pin to switch the external chips
#define POWER_PIN_EXTERNAL 7 
//*

//* define for Screen
//#define OLED_RESET 4 // not used here/ nicht genutzt bei diesem Display
Adafruit_SSD1306 display(0); // the value is not important
//#define DRAW_DELAY 118
//#define D_NUM 47
//*

//* define for DS18B20 temperature sensor
#define DS18B20_PIN 4 // PIN for temp sensor
//*

//* define for the voltageDeviderPins and resistorValue
#define VOLTAGE_PIN A0
#define VOLTAGE_RESISTOR1 9400
#define VOLTAGE_RESISTOR2 4700
#define VOLTAGE_POWERDOWN 12
//*

//* define for the buttonPins
#define BUTTON_1 A1
#define BUTTON_2 A2

#define BUTTON_TRIGGER_VALUE 600 // if analogRead returns a value above this value, the button will be triggered
//*

//* define for the CHAIN_OILER_PIN
#define CHAIN_OILER_PIN 6
// standard values for chain_oiler
#define STD_CHAIN_OILER_ACTIVE true
#define STD_CHAIN_OILER_WAIT 600000
#define STD_CHAIN_OILER_PUMP 100
#define STD_CHAIN_OILER_VOLTAGE 13.2
//*

//* define for the logs
#define GBL_LOGLEVEL 1   // set log level here
#define VERBOUS_LOG 0
#define INFO_LOG 1
#define WARNING_LOG 2
#define ERROR_LOG 3
//*

//* defines for the memory management
#define RECORD_PRESENT_ADDR 0  // 1 byte for a byte
#define RECORD_PRESENT_VALUE_0 10 // this works as a versioning control for compactibably 
#define RECORD_PRESENT_VALUE_1 10 // Later versions might changed this number

#define RECORD_CHAIN_OILER_ADDR 2     // 7 bytes to save
//*

//**** GLOBAL VARIABLES
//* power state variables
bool currentExternalPower = false;
unsigned long timeFirstLowVoltage = 0;
uint16_t timeCranking = 0;  // about 65 seconds should be enough time ;)
float lowestCrankVoltage = 0;
float measuredVoltage = 0;
uint16_t timeToWaitPowerDown = 20000; // in milliseconds

//* timing variables
// Main display refresh
unsigned long timeSinceMainDisplayRefresh = 0;  //
#define INTERVAL_MAIN_DISPLAY_REFRESH 1000 // to keep it together, define is placed here. Using define to save ram

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
DS3231 DS3231rtc(A4,A5); // first SDA_PIN, second SCL_PIN on DS3231

//* chain oiler timings
boolean chain_oiler_active = STD_CHAIN_OILER_ACTIVE;
unsigned long chain_oiler_wait = STD_CHAIN_OILER_WAIT;  // unsigned long else overflow. standard 10 minutes (600000 ms)
unsigned int chain_oiler_pump = STD_CHAIN_OILER_PUMP;  // how long the pump should run in one intervall
double chain_oiler_voltage = STD_CHAIN_OILER_VOLTAGE; // needs to be tested. Determines if the engine is running
unsigned long timeSinceChainOiler = 0;

//**** setup code
void setup()   {

  //* initialize Serial connection
  Serial.begin(250000);
  //*

  powerManager(true);

  timeSinceChainOiler = millis(); // skip the first run of the chain oiler.
                                  // Nobody wants to spill oil in the garage

  readSettings();  // read settings if saved previously
}

//**** loop code
void loop() {

// meassuring voltage and checking for low voltage power down or cranking

  measuredVoltage = readVoltage();
  if(measuredVoltage <= VOLTAGE_POWERDOWN) {
    // detected low voltage. Can be low battery or cranking
    unsigned long timeFirstCrank = millis();

    if (INFO_LOG >= GBL_LOGLEVEL) {
          Serial.println(F("I: Low voltage detected"));
    }
    // think of the cranking first
    // Crank assistent
    lowestCrankVoltage = measuredVoltage;
    while (measuredVoltage < VOLTAGE_POWERDOWN && (millis() - timeFirstCrank) < timeToWaitPowerDown) {
      if (measuredVoltage < lowestCrankVoltage){ // loop till voltage is higher or voltage is low for a longer time
          lowestCrankVoltage = measuredVoltage;

          // notify user of a new lowest value
          // this takes about 30 ms
          display.clearDisplay();
          display.setTextColor(WHITE);
          display.setTextSize(2);
          display.setCursor(0,0);
          display.println(F("Starten"));
          
          display.setTextSize(1);
          display.println(F("Niedrigste Spannung"));
          
          display.print(measuredVoltage,1);
          display.print('V');
          display.display();

        }
        measuredVoltage = readVoltage();
    }
    
    
    // if the low voltage is still present, go to sleep
    if ((millis() - timeFirstCrank) >= timeToWaitPowerDown) {
      if (INFO_LOG >= GBL_LOGLEVEL) {
          Serial.println(F("I: Low voltage continues"));
          Serial.println(F("I: Entering power saving mode"));
      }
      
      // Power saving mode
      powerManager(false);

      while(readVoltage() < VOLTAGE_POWERDOWN) {

        // Power saving mode
        // wake up each two second to check the voltage.
        // sleep time could be lower but to remain a little responsive,
        // two seconds will do
        LowPower.powerDown(SLEEP_2S, ADC_OFF, BOD_OFF);
      }
      if (INFO_LOG >= GBL_LOGLEVEL) {
          Serial.println(F("I: Voltage is good again"));
      }
      powerManager(true);      
      
    } else {
      // if it was a simple crank, wait one second to give the user some time
      // to view the displayed value
      if (INFO_LOG >= GBL_LOGLEVEL) {
          Serial.println(F("I: Low voltage was cranking"));
          Serial.print(F("I: Crank lowest voltage="));
          Serial.println(lowestCrankVoltage,1);
          Serial.print(F("I: Crank durration="));
          Serial.println(millis() - timeFirstCrank);
          Serial.flush();  
          // we need to wait for the Serial data to be transfered
          // else the log will be distorted
      }
      
      LowPower.powerDown(SLEEP_1S,ADC_OFF,BOD_OFF);

      // display the time the cranking toke
      display.clearDisplay();
      display.setTextColor(WHITE);
      display.setTextSize(2);
      display.setCursor(0,0);
      display.println(F("Starten"));
      display.setTextSize(1);
      display.println(F("Dauer"));
      // the actual time isn't that accurate.
      // the crank duration can differ.
      // If the Arduino calcs the menu for example then the low voltage detection will be delayed
      // Still it's usefull
      display.print(millis() - timeFirstCrank -8); // it takes 8 ms from the end of the cranking loop till this is calced
      display.print(F("ms"));
      display.display();
      
      LowPower.powerDown(SLEEP_1S,ADC_OFF,BOD_OFF);
    }
  }

  // chain oiler routine
  if(timeSinceChainOiler + chain_oiler_wait <= millis()) {
    timeSinceChainOiler = millis();
    if ( chain_oiler_active) {
      if (readVoltage() >= chain_oiler_voltage) {
        triggerChainOiler(chain_oiler_pump);
      } else {
        if (INFO_LOG >= GBL_LOGLEVEL) {
          Serial.print(F("I: Chain oiler voltage ("));
          Serial.print(readVoltage());
          Serial.println(F(") to low"));
        }
      }
    } else {
      if (INFO_LOG >= GBL_LOGLEVEL) {
        Serial.println(F("I: Chain oiler timer triggered, but chain oiler is set inactive"));
      }
    }   
  }
  
  // main display update routine
  if (millis() - timeSinceMainDisplayRefresh >= INTERVAL_MAIN_DISPLAY_REFRESH) {
    timeSinceMainDisplayRefresh = millis();  // reset it here to avoid the 50 ms delay of the display
    
    display.clearDisplay();
    display.setTextColor(WHITE);
    
    // display temperatur
    display.setTextSize(2);
    display.setCursor(0,0);
    display.print(readTemperatur(),1);     // takes about 23 - 24 ms
    display.println('C');
  
    // display voltage
    display.setTextSize(2);
    String voltageTempString = String(readVoltage(),1)+"V";  //  takes about 3 - 4 ms
    // determining if the voltage value length is 4 or 5 digits long
    if (voltageTempString.length() == 4) {    
      display.setCursor(80,0);
    } else {
      display.setCursor(68,0);
    }                  
    display.println(voltageTempString);
  
    // display date
    display.setTextSize(1);
    display.setCursor(0,25);
    display.println(DS3231rtc.getDateStr());  // takes about 3 - 4 ms
  
    // display time
    display.setCursor(80,25);
    char* tempTimeString = DS3231rtc.getTimeStr();
    display.println(tempTimeString);  // takes about 3 - 4 ms
  
    if(tempTimeString[0] == 50 && tempTimeString[1] >= 48 && tempTimeString[1] <= 52) { 
      // if first digit of the current hour is 2
      // and second digit of the current hour is between 0 and 4 (keep it easy to edit)
      if (VERBOUS_LOG >= GBL_LOGLEVEL) {
        Serial.println(F("V: Display dimmed"));
      }
      display.dim(true); // dim display
  
    } else if(tempTimeString[0] == 48 && tempTimeString[1] >= 48 && tempTimeString[1] <= 56) {
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
    
    display.display();
  }
  
  // button check routine
  // counts up if button is presse
  // switches to menu if counts hit a certain level
  
  if (millis() - timeSinceMainButtonCheck >= INTERVAL_MAIN_BUTTON_CHECK) {
    timeSinceMainButtonCheck = millis();
    
    // the button needs to pressed for a certain time before a action is triggerd (to prevent false input)
    // this can't be done by any other method. The display should get updated even if the button is pressed
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

}

// MENU TASK
// this displays the menu for the user
// Nothing else will and should happen if the user is in the menu.
// Everything is packed into one methode to save space in RAM and Flash
// It takes quite a amount of extra stuff to draw and control the menu
// So, not that pretty stuff here, but it's functional
void enterMenuPage() {
  boolean exitMenu = false;

  // is needed to wait for the user
  boolean firstPageDisplay = false;
  
  // Menu defines.
  // Not at the top, because a user does not really needs to change this values
  // menuTyps
  // 0 first page
  #define MENU_FIRST_PAGE_ITEMS_COUNT 4
  // 10 chain oiler
  #define MENU_CHAIN_OILER_ITEMS_COUNT 6
  // 20 clock 
  #define MENU_CLOCK_ITEMS_COUNT 13
  // 90 reset all settings
  #define MENU_RESET_SETTINGS_ITEMS_COUNT 2

  byte menuTyp = 0;

  #define MENU_POINTER_GAB 8
  #define MENU_ENTRY_HEIGHT 8
  #define MENU_FIRST_ENTRY 8
  #define MENU_OFFSET_MOVE_TRIGGER 2  // defines when the menu should start to shift the text upwards

  byte menu_item_count = 3;
  byte menu_Pointer_Location = 0;
  byte menuOffset = 0;  // moves the hole menu up to display new stuff. Dealing with the small display



  // Print a reminder message, to not use the menu while driving
  display.clearDisplay();
  display.dim(false);

  display.setCursor(0,0);
  display.setTextSize(1);
  display.println(F("BEDIENUNG WAEHREND\nDER FAHRT VERBOTEN"));
  display.setCursor(0,17);
  display.println(F("Oeler im Menue \n deaktiviert"));

  display.display();

  getButtonHoldTime(BUTTON_1, false);  // simply wait for the user to release the button

  // printig the menu in a loop and handling the button
  while(!exitMenu){

  // printing section

  display.clearDisplay();
  
    switch(menuTyp) {
      case 0:   // main menu
      // found a bug in the compiler code maybe?
      // without the extra {} which creates an extra scope the sketch won't compile
      // i do not double use variables in different cases
      // the error appears when somewhere in a case a string is initialized (not matter what name it has)
      // the error always appears afte case 20
      { 
        menu_item_count = MENU_FIRST_PAGE_ITEMS_COUNT;  // dealing with the menu hops
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
        display.print(F("Zuruecksetzen"));
    
        display.setCursor(MENU_POINTER_GAB,MENU_FIRST_ENTRY + MENU_ENTRY_HEIGHT * 3 - menuOffset);
        display.print(F("Zurueck"));
      }
      break;

      case 10:  // chain oiler menu
      {
        menu_item_count = MENU_CHAIN_OILER_ITEMS_COUNT;
  
        // Headline
        display.setCursor(0,0 - menuOffset);
        display.setTextSize(1);
        display.print(F("Kettenoeler"));
  
        // display wait and pump time on the right side
        display.setCursor(96,0);  // no menuOffset here. We need to see what we change
        display.print(chain_oiler_wait/1000);
        display.print('s');
  
        display.setCursor(96,8);
        display.print(chain_oiler_pump);
        display.print(F("ms"));
    
         // Menu points
        display.setTextSize(1);
        display.setCursor(MENU_POINTER_GAB,MENU_FIRST_ENTRY + MENU_ENTRY_HEIGHT * 0 - menuOffset);
        display.print(F("Status:"));
        if(chain_oiler_active) {
          display.print(F("aktiv"));
        } else {
          display.print(F("inaktiv"));
        }
        
        display.setCursor(MENU_POINTER_GAB,MENU_FIRST_ENTRY + MENU_ENTRY_HEIGHT * 1 - menuOffset);
        display.print(F("Wartezeit +30s"));
      
        display.setCursor(MENU_POINTER_GAB,MENU_FIRST_ENTRY + MENU_ENTRY_HEIGHT * 2 - menuOffset);
        display.print(F("Wartezeit -30s"));
  
        display.setCursor(MENU_POINTER_GAB,MENU_FIRST_ENTRY + MENU_ENTRY_HEIGHT * 3 - menuOffset);
        display.print(F("Interval +10ms"));
  
        display.setCursor(MENU_POINTER_GAB,MENU_FIRST_ENTRY + MENU_ENTRY_HEIGHT * 4 - menuOffset);
        display.print(F("Interval -10ms"));
      
        display.setCursor(MENU_POINTER_GAB,MENU_FIRST_ENTRY + MENU_ENTRY_HEIGHT * 5 - menuOffset);
        display.print(F("Zurueck"));
      }
      break;

      case 20:  // clock menu 
      {
        menu_item_count = MENU_CLOCK_ITEMS_COUNT;
        
        // Headline
        display.setCursor(0,0 - menuOffset);
        display.print(F("Uhrzeit"));

        if(menu_Pointer_Location < 6) {
          // display time on the right side. Horizontal
          char* tempSettingsTime = DS3231rtc.getTimeStr();
          display.setCursor(100,8);
          display.print(tempSettingsTime[0]);
          display.print(tempSettingsTime[1]);
          display.print('H');
          display.setCursor(100,16);
          display.print(tempSettingsTime[3]);
          display.print(tempSettingsTime[4]);
          display.print('M');
          display.setCursor(100,24);
          display.print(tempSettingsTime[6]);
          display.print(tempSettingsTime[7]);
          display.print('S');
        } else {
          char* tempSettingsDate = DS3231rtc.getDateStr();
          
          display.setCursor(100,8);
          display.print(tempSettingsDate[0]);
          display.print(tempSettingsDate[1]);
          display.print('D');
          display.setCursor(100,16);
          display.print(tempSettingsDate[3]);
          display.print(tempSettingsDate[4]);
          display.print('M');
          display.setCursor(90,24);
          display.print(tempSettingsDate[6]);
          display.print(tempSettingsDate[7]);
          display.print(tempSettingsDate[8]);
          display.print(tempSettingsDate[9]);
          display.print('Y');
        }
        

        display.setCursor(MENU_POINTER_GAB,MENU_FIRST_ENTRY + MENU_ENTRY_HEIGHT * 0 - menuOffset);
        display.print(F("+5 sek"));

        display.setCursor(MENU_POINTER_GAB,MENU_FIRST_ENTRY + MENU_ENTRY_HEIGHT * 1 - menuOffset);
        display.print(F("-5 sek"));
      
        display.setCursor(MENU_POINTER_GAB,MENU_FIRST_ENTRY + MENU_ENTRY_HEIGHT * 2 - menuOffset);
        display.print(F("+1 min"));
  
        display.setCursor(MENU_POINTER_GAB,MENU_FIRST_ENTRY + MENU_ENTRY_HEIGHT * 3 - menuOffset);
        display.print(F("-1 min"));
  
        display.setCursor(MENU_POINTER_GAB,MENU_FIRST_ENTRY + MENU_ENTRY_HEIGHT * 4 - menuOffset);
        display.print(F("+1 std"));
      
        display.setCursor(MENU_POINTER_GAB,MENU_FIRST_ENTRY + MENU_ENTRY_HEIGHT * 5 - menuOffset);
        display.print(F("-1 std"));
        
        display.setCursor(MENU_POINTER_GAB,MENU_FIRST_ENTRY + MENU_ENTRY_HEIGHT * 6 - menuOffset);
        display.print(F("+1 tag"));
        
        display.setCursor(MENU_POINTER_GAB,MENU_FIRST_ENTRY + MENU_ENTRY_HEIGHT * 7 - menuOffset);
        display.print(F("-1 tag"));
        
        display.setCursor(MENU_POINTER_GAB,MENU_FIRST_ENTRY + MENU_ENTRY_HEIGHT * 8 - menuOffset);
        display.print(F("+1 mon"));
        
        display.setCursor(MENU_POINTER_GAB,MENU_FIRST_ENTRY + MENU_ENTRY_HEIGHT * 9 - menuOffset);
        display.print(F("-1 mon"));
        
        display.setCursor(MENU_POINTER_GAB,MENU_FIRST_ENTRY + MENU_ENTRY_HEIGHT * 10 - menuOffset);
        display.print(F("+1 jahr"));
        
        display.setCursor(MENU_POINTER_GAB,MENU_FIRST_ENTRY + MENU_ENTRY_HEIGHT * 11 - menuOffset);
        display.print(F("-1 jahr"));
        
        display.setCursor(MENU_POINTER_GAB,MENU_FIRST_ENTRY + MENU_ENTRY_HEIGHT * 12 - menuOffset);
        display.print(F("Zurueck"));
        
      }
      break;

      case 90: // reset settings menu
      {
        menu_item_count = MENU_RESET_SETTINGS_ITEMS_COUNT;

        // Headline
        display.setCursor(0,0 - menuOffset);
        display.print(F("Einstellungen zurücksetzen"));
    
        // to be sure the user really wants to reset all data
        display.setCursor(MENU_POINTER_GAB,MENU_FIRST_ENTRY + MENU_ENTRY_HEIGHT * 0 - menuOffset);
        display.print(F("Wirklich zuruecksetzen?"));
    
        display.setCursor(MENU_POINTER_GAB,MENU_FIRST_ENTRY + MENU_ENTRY_HEIGHT * 1 - menuOffset);
        display.print(F("Zurueck"));
      }
      break;
    }

    // print the menu pointer
    display.setCursor(0, MENU_FIRST_ENTRY + menu_Pointer_Location * MENU_ENTRY_HEIGHT - menuOffset);
    display.print((char)175);
    

    display.display();

  // action section
  // get the input from the user
  // if this is the first display of a new menuPage then wait for the user to release the button
    unsigned int button_1_hold = 0;
    if (firstPageDisplay) {
      getButtonHoldTime(BUTTON_1, false);
      firstPageDisplay = false;
    } else {
      button_1_hold = getButtonHoldTime(BUTTON_1, true);
    }
    
    // first find out if the button was a long press
    // reusing the predefined value for the button long press trigger (100ms * 10ms) = 1000 ms
    if(button_1_hold >= INTERVAL_MAIN_BUTTON_CHECK * MAIN_BUTTON_COUNTER_TRIGGER) {
      switch(menuTyp) {
        case 0:   // main menu
        switch(menu_Pointer_Location) {
          case 0:  // enter chain oiler menu
          menuTyp = 10;
          menu_Pointer_Location = 0;
          firstPageDisplay = true;
          break;
          case 1:  // enter clock menu
          menuTyp = 20;
          menu_Pointer_Location = 0;
          firstPageDisplay = true;
          break;
          case 2:  // reset settings
          menuTyp = 90;
          menu_Pointer_Location = 0;
          firstPageDisplay = true;
          break;
          case 3:
          exitMenu = true;
          break;
          
        }        
        break;

        case 10:   // chain oiler menu
        switch(menu_Pointer_Location) {
          case 0:
          if(chain_oiler_active) {
            chain_oiler_active = false;
          } else {
            chain_oiler_active = true;
          }
          break;
          case 1:  // inc wait time
          chain_oiler_wait += 30000;
          break;
          case 2:  // dec wait time
          if(chain_oiler_wait > 30000) {
            chain_oiler_wait -= 30000;
          }
          break;
          case 3:  // inc pump time
          chain_oiler_pump += 10;
          break;
          case 4:  // dec pump time
          if(chain_oiler_pump > 10) {
            chain_oiler_pump -= 10;
          }
          break;
          case 5:  // back to menu
          menuTyp = 0;
          menu_Pointer_Location = 0;
          saveSettings();
          firstPageDisplay = true;
          break;
        }
        break;

        case 20:  // clock menu
          switch(menu_Pointer_Location) {
            case 0:
              alterTime(0,0,0,0,0,+5);
            break;
            case 1:
              alterTime(0,0,0,0,0,-5);
            break;
            case 2:
              alterTime(0,0,0,0,+1,0);
            break;
            case 3:
              alterTime(0,0,0,0,-1,0);
            break;
            case 4:
              alterTime(0,0,0,+1,0,0);
            break;
            case 5:
              alterTime(0,0,0,-1,0,0);
            break;
            case 6:
              alterTime(0,0,+1,0,0,0);
            break;
            case 7:
              alterTime(0,0,-1,0,0,0);
            break;
            case 8:
              alterTime(0,+1,0,0,0,0);
            break;
            case 9:
              alterTime(0,-1,0,0,0,0);
            break;
            case 10:
              alterTime(+1,0,0,0,0,0);
            break;
            case 11:
              alterTime(-1,0,0,0,0,0);
            break;
            case 12:
              menuTyp = 0;
              menu_Pointer_Location = 0;
              firstPageDisplay = true;
            break;            
          }
        break;

        case 90:  // reset settings menu
        switch(menu_Pointer_Location) {
            case 0:
            chain_oiler_active = STD_CHAIN_OILER_ACTIVE;
            chain_oiler_wait = STD_CHAIN_OILER_WAIT;
            chain_oiler_pump = STD_CHAIN_OILER_PUMP;
            chain_oiler_voltage = STD_CHAIN_OILER_VOLTAGE;
            for (uint16_t i = 0; i < EEPROM.length(); i++) {
              EEPROM.update(i,255);
            }
            saveSettings();
            break;
            case 1:
            menuTyp = 0;
            menu_Pointer_Location = 0;
            firstPageDisplay = true;
            break;
          }
        break;
      }

      // else check for a short press
    } else if (button_1_hold > 50 && button_1_hold < 200) {
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
  display.print(F("Gute\nFahrt"));
  

  display.display();

  getButtonHoldTime(BUTTON_1, false);  // simply wait for the user to release the button
}

// POWER MANAGER
// this methode manages to power or power down the external chips
// DS3231 clock
// SSD1306 display
// DS18B20 temperatur
// true turns power one and initializes the components, false turns power off
void powerManager(boolean wantedState){

  if(wantedState && !currentExternalPower) {
    if(INFO_LOG >= GBL_LOGLEVEL) {
      Serial.print(F("I: powering up external chips"));
    }
    pinMode(POWER_PIN_EXTERNAL,OUTPUT);
    digitalWrite(POWER_PIN_EXTERNAL,HIGH);
  
    delay(10);
    //* initialize Display
    // initialize with the I2C addr 0x3C / mit I2C-Adresse 0x3c initialisieren
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    //*
  
    //* initialize DS18B20 temperature sensor
    if (!DS18B20ds.search(DS18B20addr)) {
      if(ERROR_LOG >= GBL_LOGLEVEL) {  
        // if this runs more then one time it will print an error
        // but the temperatur sensor will still work fine
        Serial.println(F("E: Tempsensor not found"));
      }
    } else {
      if(INFO_LOG >= GBL_LOGLEVEL) {
        Serial.println(F("temp sensor is good addr: "));
        for(byte i = 0; i < 8; i++) {
          Serial.print(DS18B20addr[i],HEX);
          Serial.print(' ');
        }
        Serial.print('\n');
      }
    }
    //* initialize DS3231 clock chip
    DS3231rtc.begin();
    //*
    currentExternalPower = true;
  } else if(!wantedState && currentExternalPower) {
    
    if(INFO_LOG >=GBL_LOGLEVEL) {
      Serial.println(F("I: powering down external sensors"));
    }
    
    pinMode(POWER_PIN_EXTERNAL,OUTPUT);
    digitalWrite(POWER_PIN_EXTERNAL,LOW);
    
    currentExternalPower = false;
  }
}

// READ TEMPERATUR TASK
// adapted from https://edwardmallon.wordpress.com/2014/03/12/using-a-ds18b20-temp-sensor-without-a-dedicated-library/
// this version does NOT work with the parasite mod. Big benefit here: no delay
// reduced code to a minimum. No options. Save space. Do the necessary.
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
// reads the raw voltage and does the voltage divider math
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
//the chain oiler keeps pumping oil (which is NO good ;)  )
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
// the boolean decides if the method returns after a long press is
// recorded or continues to count (wait for the user to release
unsigned int getButtonHoldTime(byte buttonPIN, bool returnAfterLongPress) {
  unsigned long startTime = millis();
  while (analogRead(buttonPIN) >= BUTTON_TRIGGER_VALUE) {   
    // wait for the user to release button
    // or return after the time it takes for a long press
    // This way the user can simply hold the button and trigger multiple long presses
    if (returnAfterLongPress && millis() - startTime > INTERVAL_MAIN_BUTTON_CHECK * MAIN_BUTTON_COUNTER_TRIGGER) { // calculating time for a long click
      return millis() - startTime;  // if the time is already over, why should we count on?
    }
  }

  return millis() - startTime;
}

// ALTER THE TIME ON THE RTC-CHIP
// this changes the time according to the values it gets
// a value for yearA of -1 will decress the year by one
// this methode will handle the "flip" of numbers (59 + 1 = 0 seconds -> +1 minute)
// and the different amount of days in a month
// Yes, this methode can fail. Putting 120 in secondA will produce nonsense
// But this case will never happen in this sketch
// Additional checks cost flash memory and CPU cycles, so lets save those
// Limit for the input values are the "normal" limits.
// There are not more than 59 seconds for example. 60 seconds are 1 minute
void alterTime(int8_t yearA, int8_t monthA, int8_t dayA, int8_t hourA, int8_t minuteA, int8_t secondA){

  // get current time + date
  char* tempHourTime = DS3231rtc.getTimeStr();
  char* tempDateTime = DS3231rtc.getDateStr();
  
  int16_t yearN = yearA + (tempDateTime[6]-48) * 1000 + (tempDateTime[7]-48) * 100 + (tempDateTime[8]-48) * 10 + (tempDateTime[9]-48);
  int8_t monthN = monthA + (tempDateTime[3]-48) * 10 + (tempDateTime[4]-48);
  int8_t dayN = dayA + (tempDateTime[0]-48) * 10 + (tempDateTime[1]-48);
  int8_t hourN = hourA + (tempHourTime[0]-48) * 10 + (tempHourTime[1]-48);
  int8_t minuteN = minuteA + (tempHourTime[3]-48) * 10 + (tempHourTime[4]-48);
  int8_t secondN = secondA + (tempHourTime[6]-48) * 10 + (tempHourTime[7]-48);
  int8_t allowedDays[] = {31,28,31,30,31,30,31,31,30,31,30,31};

  
  
  // find logic "flips" of time variables
  if(secondN>59) {
    secondN = secondN - 60;
    minuteN++;
  } else if (secondN < 0) {
    secondN = secondN + 60;
    minuteN--;
  }

  if(minuteN>59) {
    minuteN = minuteN - 60;
    hourN ++;
  } else if (minuteN < 0) {
    minuteN = minuteN + 60;
    hourN--;
  }

  if (hourN>23) {
    hourN -= 24;
    dayN++;
  } else if (hourN<0) {
    hourN += 24;
    dayN--;
  }

  // the month defines the amount of days
  // if there are 12 monthN and 40 dayN,
  // then monthN will be increased, resulting in 13 monthN
  // -> rerole
  // if we rerole with a new month the limit of days can be different
  // so we need a do while loop
  bool dayCountValid;
  do {
    
    if (monthN<1) {
      monthN+=12;
      yearN--;
    } else if (monthN>12) {
      monthN-=12;
      yearN++;
    }
  
    if (dayN > allowedDays[monthN-1]) {
      dayN -= allowedDays[monthN-1];
      monthN++;
      dayCountValid = false;
    } else if (dayN <1) {
      // prevent array outOfBounds
      if(monthN == 1) {
        dayN += allowedDays[11];
      } else {
        dayN += allowedDays[monthN-2];
      }
      monthN--;
      dayCountValid = false;
    } else {
          dayCountValid = true;
    }
  } while (!dayCountValid); 

  DS3231rtc.setTime(hourN,minuteN,secondN);
  DS3231rtc.setDate(dayN,monthN,yearN);

  if (INFO_LOG >= GBL_LOGLEVEL) {
    Serial.println(F("I: Time set to"));
    Serial.print(F("time: "));
    Serial.print(hourN);
    Serial.print(':');
    Serial.print(minuteN);
    Serial.print(':');
    Serial.println(secondN);
  
    Serial.print(F("date: "));
    Serial.print(dayN);
    Serial.print('.');
    Serial.print(monthN);
    Serial.print('.');
    Serial.println(yearN);
  }
}



struct chain_Oiler_data_t {
  boolean active;
  unsigned long wait_Time;
  unsigned int pump_Time;
};

union chain_Oiler_Save_t {
  chain_Oiler_data_t data;
  byte byteArray[7];
};

// SAVE THE SETTINGS
// all settings which are made in the menu are saved to EEPROM
// clock settings do not need to be saved separately
// the EEPROM can only be written bytewise
void saveSettings() {
  // EEPROM.update to save some write cycles

  // save the record_present check some for versioning control
  EEPROM.update(RECORD_PRESENT_ADDR, RECORD_PRESENT_VALUE_0);
  EEPROM.update(RECORD_PRESENT_ADDR+1, RECORD_PRESENT_VALUE_1);

  chain_Oiler_Save_t chain_Oiler_Save;
  chain_Oiler_Save.data.active = chain_oiler_active;
  chain_Oiler_Save.data.wait_Time = chain_oiler_wait;
  chain_Oiler_Save.data.pump_Time = chain_oiler_pump;
   
  for (byte i = 0; i<7; i++) {
    EEPROM.update(RECORD_CHAIN_OILER_ADDR + i, chain_Oiler_Save.byteArray[i]);
    Serial.print(i);
    Serial.print('.');
    Serial.println(chain_Oiler_Save.byteArray[i], BIN);
  }

  Serial.println('*');

  Serial.println(chain_Oiler_Save.data.active, BIN);
  Serial.println(chain_Oiler_Save.data.wait_Time, BIN);
  Serial.println(chain_Oiler_Save.data.pump_Time, BIN);
}

// READ THE SETTINGS
// performs a check if there is usefull data in the EEPROM
// and reads the data from EEPROM
void readSettings() {
  if (INFO_LOG >= GBL_LOGLEVEL) {
    Serial.println(F("I: Trying to read settings from EEPROM"));
  }
  
  if(EEPROM.read(RECORD_PRESENT_ADDR) == RECORD_PRESENT_VALUE_0 && EEPROM.read(RECORD_PRESENT_ADDR) == RECORD_PRESENT_VALUE_1) {
    chain_Oiler_Save_t chain_Oiler_Save;

    for (byte i = 0; i<7; i++) {
      chain_Oiler_Save.byteArray[i] = EEPROM.read(RECORD_CHAIN_OILER_ADDR + i);
    }
    
    chain_oiler_active = chain_Oiler_Save.data.active;
    chain_oiler_wait = chain_Oiler_Save.data.wait_Time;
    chain_oiler_pump = chain_Oiler_Save.data.pump_Time;
    
    //Serial.println(temp_chain_oiler_wait);
    if (INFO_LOG >= GBL_LOGLEVEL) {
      Serial.println(F("I: Settings read successfully from EEPROM"));
      Serial.print(F("I: Read pump_active= "));
      Serial.println(chain_Oiler_Save.data.active);
      Serial.print(F("I: Read wait_time= "));
      Serial.println(chain_Oiler_Save.data.wait_Time);
      Serial.print(F("I: Read pump_time= "));
      Serial.println(chain_Oiler_Save.data.pump_Time);
    }

  } else {
    if (ERROR_LOG >= GBL_LOGLEVEL) {
      Serial.print(F("E: No valid settings in EEPROM"));
    }
  }
}


