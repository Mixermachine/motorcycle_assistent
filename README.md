# Motorcycle Assistent

## Describtion
I created this project to equipe my BMW F650 GS motorcycle with a little assitent including a digital chain oiler.<br />

What is a chain oiler? <br />
A chain oiler drops a little amount of oil on the heavily stressed motorcycle chain.<br />
This way the chain lasts longer. <br />

Why a digital chain oiler? <br />
Regular chain oilers are often not precise, hard to adjust and finaly quite expensive.<br />
In this project I use verily cheap components with an easy-to-use interface.<br />

For controlling my Assistent I added a display, external chips and one button.<br />
The program variables, log and comments are all written in english.<br />
The user interface is designed for German users.<br />
I could provide a interface for Englisch users if there is a need for it.<br />

It runs on the Arduino/Genuino Nano and Arduino/Genuino Uno<br />

## Features:
+ Triggers the chain oiler pump after a given periode of time for a little interval
+ Displays temperature
+ Displays battery voltage
+ Displays date and time (dim display accourding to the time)
+ Provides a menu for the settings
+ Power saving mode
+ Crank assistent (shows lowest voltage and the duration)

## Hardware:
+ Arduino nano/uno   (Uno for development, Nano for the actual use in my motorcycle)
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



## Chain oiler
The chain oiler can be trimmed to your needs.<br />
The pump duration time, the interval time and the state (if it works at all) can be easily set in the menu.<br />
All settings are saved in the internal memory (EEPROM) and will be loaded with each power up.<br />
The chain oiler only works if the battery voltage is above 13,2V.<br />
This is no technical limitation, but a safety one.<br />
On my F650 I needed to grab power from the rear lights, which means if I leave the parking lights on my bike on, the Arduino is still powered.<br />
If the engine is running, the generator will hold the voltage at 13,5V to 14,2V -> which means the chain oiler will run if it is set active.<br />

## Typical display faces
![Alt text](/img/mainPage.jpg?raw=true "Main page")

After a long click on the button (1 second, state 2016-05-12) the menu will open

![Alt text](/img/mainMenu.jpg?raw=true "Main menu")

With a short click on the button you move the cursor one step further down (on the last entry it will jump back up again).<br />
With a long click on the button you enter submenus and select options.<br />
As long you are in the menu the chain oiler and all other components will remain in standby.<br />

## Voltage regulator and power saving
I recommend to replace the onboard voltage regulator with one which has less quiescent current (current which the regulator needs for itself) if you are planning to use the power saving mode. <br />
The standard one uses about 10 mA and requiers at least 7V to produce a good 5V line for the Arduino. <br />
The MCP1703 for example uses only 0.003 mA for itself and requiers 0,1V to 0,6V extra above the 5V for the Arduino to work properly. <br />
DC-DC converters do not really make sense here.<br />
Although the converter would be more efficient in the normal work mode, it would be horrible in power saving mode (because it also has a fairly high quiescent current). We don't really need a high efficiency if the engine is running (about 50mA in normal mode). <br />
To decrease the comsumption even more, you can disconnect the power LED of the Arduino.<br />

So if you use the MCP1703 and remove the power LED from the Arduino it will be much more efficient.<br />
The current should be under 0,01 mA. (Needs testing)<br />
The power saving mode will kick in if the voltage is under 12V for longer then 20 seconds.<br />
It will turn of the power of the display, clock and temperature sensor.<br />
Every 2 seconds the Arduino will wake up for under 1 millisecond to check the voltage.<br />
If the voltage is equal or above 12V the Arduino will wake up and return to the normal work mode.<br />

## Schematic

![Alt text](/img/fritzingScreenshot.jpg?raw=true "Fritzing Schematic") <br />


## Developer stuff
This project does focus on the highest efficiency possible.<br />
The Atmel328 chip (Arduino Uno, Mini,...) provides 32kb of flash memory and 2 kb of ram.<br />
I could not always focus on best practice code, but tried to keep the work well sorted and well commented.<br />
Flash memory and RAM efficiency was the main goal.<br />
The code offers the possibility of logging via the serial connection of the Arduino.<br />
Change GBL_LOGLEVEL to a value which you like. (0 verbous, 1 info, 2 warning, 3 error).<br />
This project nearly uses the hole flash memory.<br />
(state 2016-05-13, 27 148 Byte of the flash memory is used).<br />

Please send me a message before you use this Arduino project for a commercial product.<br />
-> dietz1993@gmail.com
