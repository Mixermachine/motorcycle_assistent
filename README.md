# Motorcycle Assistent

## Describtion
I created this project to equipe my BMW F650 GS motorcycle with a digital chain oiler.
To control it I added a display, some other chips and one button.
This project is still in the alpha phase.
The program variables, log and comments are all written in englisch.
The user interface is designed for German users.
It works on the Arduino/Genuino Nano and Arduino/Genuino Uno

## Hardware:
+ Arduino nano/uno   (Uno for development, Nano for actual use in my motorcycle)
+ SSD1306 display
+ DS18B20 temperature sensor
+ DS3231 clock chip
+ 2 * 4,7k, 2 * 12k Ohm Resistor
+ Button
+ Energy-saving voltage regulator (not compulsory, MCP1703 recomended)
+ Wiring

## Libraries
+ SPI.h (screen)
+ Wire.h (screen and clock)
+ Adafruit_GFX.h (screen)
+ Adafruit_SSD1306.h (clock)
+ OneWire.h (temperature)
+ DS3231.h (clock)
+ EEPROM.h (EEPROM memory)
+ LowPower.h (power saving)

## Features:
+ Trigger the chain oiler pump after a given periode of time
+ Display temperature
+ Display battery voltage
+ Display date and time (dim display accourding to the time)
+ Provide a menu for the settings
+ Power saving mode

## Chain oiler
The chain oiler can be trimmed to you needs.
The pump duration time, interval time and the state (if it should work at all) can be easily set in the menu.
All settings are saved in the internal memory (EEPROM) and will be loaded at each power up.
The chain oiler will only work if the battery voltage is above 13,2V.
This is no technical limitation, but a safety one.
On my F650 I needed to grab power from the rear lights, so when I leave my bike with parking lights on, the Arduino is still powered.
If the engine is running, the generator will hold the voltage at 13,5V to 14,2V -> chain oiler will run if active.

## Typical display faces
![Alt text](/img/mainPage.jpg?raw=true "Main page")

After a long click on the button (1 second, state 2016-05-12) the menu will open

![Alt text](/img/mainMenu.jpg?raw=true "Main menu")

With a short click on the button you move the cursor one step down (on the last entry it will jump up again).
With a long click on the button you enter subMenus and select options.
As long you are in the menu the chain oiler will not run.

## Voltage regulator and power saving
I recommend to replace the onboard voltage regulator with one which has less quiescent current (current which the regulator needs for it self) if you really want to use the power saving mode. 
The standard one uses about 10 mA and requiers at least 7V to produce a good 5V line for the Arduino. 
The MCP1703 for example only uses only 0.003 mA for itself and requiers 0,1V to 0,6V extra above the 5V for the Arduino to work properly.
DC-DC converters do not really make sense here.
Although the converter would be more efficient in the normal work mode, it would be horrible in power saving mode (because it also has a fairly high quiescent current). We don't really need a high efficiency if the engine is running.
To decrease the comsumption even more, disconnect the power LED of the Arduino.

So if you use the MCP1703 and remove the power LED from the Arduino it will be much more efficient.
The current should be under 0,1 mA. (Needs testing)
The power saving mode will kick in if the voltage is under 12V for longer then 20 seconds.
It will turn of the power of the display, clock and temperatur sensor.
Every 2 seconds the Arduino will wake up for under 1 milliSecond to check the voltage.
If the voltage is equal or above 12V the Arduino will wake up and return to the normal work mode.


## Developer stuff
This project does focus on the highest efficiency possible.
The Atmel328 chip (Arduino Uno, Mini,...) provides 32kb of flash memory and 2 kb of ram.
I could not always focus on clean code, but tried to keep the code well sorted and well commented.

Please send me a message before you use this Arduino project for a commercial product.
