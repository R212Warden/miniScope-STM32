# miniScope-stm32
Miniature oscilloscope that runs on STM32
The chip in question is STM32F103C6T6 with 32kb of flash.
This is now reuploaded for 3rd time due to various user errors...

# About

This code contains some simple oscilloscope code. It is designed to use small 0.96" LCD display. You can also measure capacitance and voltage.
The code uses afiskon ST7735 library:

	https://github.com/afiskon/stm32-st7735
	

# Wiring
There is no provided schematic. But you can wire it this way (using bluepill with STM32F103C6T6):

	+-----------+---------------+
	| BLUEPILL	|   Function 	|
	+-----------+---------------+
	|	PB7		|	lcd_res		|
	|	PB6		|	lcd_dc		|
	|	PB5		|	lcd_mosi	|
	|	PB4		|	lcd_cs		|
	|	PB3		|	lcd_sck		|
	|	PA10	|	RX			|
	|	PA09	|	TX			|
	|	PA08	|	PWM_IN		|
	|	PA0		|	btn_power	|
	|	PA1		|	btn_mode	|
	|	PA2		|	btn_dowm	|
	|	PA3		|	btn_up		|
	|	PB1		|	scope_in	|
	+-----------+---------------+

The PWM_IN connects to 555 timer in astable mode wth variable capacitor (thats how its measuring capacitance), R1 = 1.7k and R2 = 470 Ohm.
Buttons pull to ground when pressed.
LCD has no backlight control.
Power button does nothing.

# Building

Inside the repo (after you clone) hit make. Your executable is in build folder and you can upload it your way.
You need gcc-arm-none-eabi.







