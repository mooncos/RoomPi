/*
 * lcd1602.c
 *
 *  Created on: 9 mar. 2021
 *      Author: Victoria M. Gullon and Marcos Gomez
 */

#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>

/************************/

#include "lcd1602vars.h"
#include "lcd1602.h"

LCD1602Display* LCD1602Display__create(int id, int rs, int rw, int enable,
		int fourbitmode, int d0, int d1, int d2, int d3, int d4, int d5, int d6,
		int d7) {
	LCD1602Display *result = (LCD1602Display*) malloc(sizeof(LCD1602Display));
	result->id = id;
	result->rs_pin = rs;
	result->rw_pin = rw;
	result->enable_pin = enable;

	result->data_pins[0] = d0;
	result->data_pins[1] = d1;
	result->data_pins[2] = d2;
	result->data_pins[3] = d3;
	result->data_pins[4] = d4;
	result->data_pins[5] = d5;
	result->data_pins[6] = d6;
	result->data_pins[7] = d7;

	if (fourbitmode) {
		result->_displayfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;
	} else {
		result->_displayfunction = LCD_8BITMODE | LCD_1LINE | LCD_5x8DOTS;
	}

	return result;
}

void LCD1602Display__destroy(LCD1602Display *display) {
	if (display) {
		free(display);
	}
}

void LCD1602Display__begin(LCD1602Display *display, int cols, int lines,
		int dotsize) {
	if (lines > 1) {
		display->_displayfunction |= LCD_2LINE;
	}

	display->_numlines = lines;

	LCD1602Display__set_row_offsets(display, 0x00, 0x40, 0x00 + cols,
			0x40 + cols);

	if ((dotsize != LCD_5x8DOTS) && (lines == 1)) {
		display->_displayfunction |= LCD_5x10DOTS;
	}

	pinMode(display->rs_pin, OUTPUT);
	// we can save 1 pin by not using RW. Indicate by passing 255 instead of pin# !!!PUT PIN TO GROUND!!!
	if (display->rw_pin != 255) {
		pinMode(display->rw_pin, OUTPUT);
	}
	pinMode(display->enable_pin, OUTPUT);

	int i = 0;
	if (!(display->_displayfunction & LCD_8BITMODE)) {
		i = 4;
	}

	for (; i < 8; ++i) {
		pinMode(display->data_pins[i], OUTPUT);
	}

	delayMicroseconds(50000);

	// Now we pull both RS and R/W low to begin commands
	digitalWrite(display->rs_pin, LOW);
	digitalWrite(display->enable_pin, LOW);
	if (display->rw_pin != 255) {
		digitalWrite(display->rw_pin, LOW);
	}

	//put the LCD into 4 bit or 8 bit mode
	if (!(display->_displayfunction & LCD_8BITMODE)) {
		// we start in 8bit mode, try to set 4 bit mode
		LCD1602Display__write4bits(display, 0x03);
		delayMicroseconds(4500); // wait min 4.1ms

		// second try
		LCD1602Display__write4bits(display, 0x03);
		delayMicroseconds(4500); // wait min 4.1ms

		// third go!
		LCD1602Display__write4bits(display, 0x03);
		delayMicroseconds(150);

		// finally, set to 4-bit interface
		LCD1602Display__write4bits(display, 0x02);
	} else {
		// this is according to the hitachi HD44780 datasheet
		// page 45 figure 23

		// Send function set command sequence
		LCD1602Display__command(display,
		LCD_FUNCTIONSET | display->_displayfunction);
		delayMicroseconds(4500);  // wait more than 4.1ms

		// second try
		LCD1602Display__command(display,
		LCD_FUNCTIONSET | display->_displayfunction);
		delayMicroseconds(150);

		// third go
		LCD1602Display__command(display,
		LCD_FUNCTIONSET | display->_displayfunction);
	}

	// finally, set # lines, font size, etc.
	LCD1602Display__command(display,
	LCD_FUNCTIONSET | display->_displayfunction);

	// turn the display on with no cursor or blinking default
	display->_displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
	LCD1602Display__display(display);

	// clear it off
	LCD1602Display__clear(display);

	// Initialize to default text direction (for romance languages)
	display->_displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
	// set the entry mode
	LCD1602Display__command(display, LCD_ENTRYMODESET | display->_displaymode);

}

void LCD1602Display__set_row_offsets(LCD1602Display *display, int row0,
		int row1, int row2, int row3) {
	display->_row_offsets[0] = row0;
	display->_row_offsets[1] = row1;
	display->_row_offsets[2] = row2;
	display->_row_offsets[3] = row3;
}

void LCD1602Display__clear(LCD1602Display *display) {
	LCD1602Display__command(display, LCD_CLEARDISPLAY);
	delayMicroseconds(2000);  // this command takes a long time!
}

void LCD1602Display__home(LCD1602Display *display) {
	LCD1602Display__command(display, LCD_RETURNHOME);
	delayMicroseconds(2000);  // this command takes a long time!
}

void LCD1602Display__set_cursor(LCD1602Display *display, int col, int row) {
	const size_t max_lines = sizeof(display->_row_offsets)
			/ sizeof(*(display->_row_offsets));
	if (row >= max_lines) {
		row = max_lines - 1;    // we count rows starting w/0
	}
	if (row >= display->_numlines) {
		row = display->_numlines - 1;    // we count rows starting w/0
	}
	LCD1602Display__command(display,
	LCD_SETDDRAMADDR | (col + display->_row_offsets[row]));
}

void LCD1602Display__no_display(LCD1602Display *display) {
	display->_displaycontrol &= ~LCD_DISPLAYON;
	LCD1602Display__command(display,
	LCD_DISPLAYCONTROL | display->_displaycontrol);
}

void LCD1602Display__display(LCD1602Display *display) {
	display->_displaycontrol |= LCD_DISPLAYON;
	LCD1602Display__command(display,
	LCD_DISPLAYCONTROL | display->_displaycontrol);
}

void LCD1602Display__no_cursor(LCD1602Display *display) {
	display->_displaycontrol &= ~LCD_CURSORON;
	LCD1602Display__command(display,
	LCD_DISPLAYCONTROL | display->_displaycontrol);
}

void LCD1602Display__cursor(LCD1602Display *display) {
	display->_displaycontrol |= LCD_CURSORON;
	LCD1602Display__command(display,
	LCD_DISPLAYCONTROL | display->_displaycontrol);
}

void LCD1602Display__no_blink(LCD1602Display *display) {
	display->_displaycontrol &= ~LCD_BLINKON;
	LCD1602Display__command(display,
	LCD_DISPLAYCONTROL | display->_displaycontrol);
}

void LCD1602Display__blink(LCD1602Display *display) {
	display->_displaycontrol |= LCD_BLINKON;
	LCD1602Display__command(display,
	LCD_DISPLAYCONTROL | display->_displaycontrol);
}

void LCD1602Display__scroll_display_left(LCD1602Display *display) {
	LCD1602Display__command(display,
	LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}

void LCD1602Display__scroll_display_right(LCD1602Display *display) {
	LCD1602Display__command(display,
	LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

void LCD1602Display__left_to_right(LCD1602Display *display) {
	display->_displaymode |= LCD_ENTRYLEFT;
	LCD1602Display__command(display, LCD_ENTRYMODESET | display->_displaymode);
}

void LCD1602Display__right_to_left(LCD1602Display *display) {
	display->_displaymode &= ~LCD_ENTRYLEFT;
	LCD1602Display__command(display, LCD_ENTRYMODESET | display->_displaymode);
}

void LCD1602Display__autoscroll(LCD1602Display *display) {
	display->_displaymode |= LCD_ENTRYSHIFTINCREMENT;
	LCD1602Display__command(display, LCD_ENTRYMODESET | display->_displaymode);
}

void LCD1602Display__no_autoscroll(LCD1602Display *display) {
	display->_displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
	LCD1602Display__command(display, LCD_ENTRYMODESET | display->_displaymode);
}

/* Print function */

int LCD1602Display__print(LCD1602Display *display, char *fmt, ...) {
	char string[16];
	va_list lst;
	va_start(lst, fmt);
	vsprintf(string, fmt, lst);

	int i;
	int n = 0;
	for (i = 0; i < 16; i++) {
		if (string[i] == '\0') {
			break;
		} else {
			LCD1602Display__write(display, string[i]);
			n++;
		}
	}
	return n;
	va_end(lst);
}

void LCD1602Display__create_char(LCD1602Display *display, int addr,
		int charmap[]) {
	addr &= 0x7;
	LCD1602Display__command(display, LCD_SETCGRAMADDR | (addr << 3));
	for (int i = 0; i < 8; i++) {
		LCD1602Display__write(display, charmap[i]);
	}

}

void LCD1602Display__command(LCD1602Display *display, int value) {
	LCD1602Display__send(display, value, LOW);
}

void LCD1602Display__write(LCD1602Display *display, int value) {
	LCD1602Display__send(display, value, HIGH);
}

void LCD1602Display__send(LCD1602Display *display, int value, int mode) {
	digitalWrite(display->rs_pin, mode);
	if (display->rw_pin != 255) {
		digitalWrite(display->rw_pin, LOW);
	}
	if (display->_displayfunction & LCD_8BITMODE) {
		LCD1602Display__write8bits(display, value);
	} else {
		LCD1602Display__write4bits(display, value >> 4);
		LCD1602Display__write4bits(display, value);
	}
}

void LCD1602Display__pulse_enable(LCD1602Display *display) {
	digitalWrite(display->enable_pin, LOW);
	delayMicroseconds(1);
	digitalWrite(display->enable_pin, HIGH);
	delayMicroseconds(1);    // enable pulse must be >450ns
	digitalWrite(display->enable_pin, LOW);
	delayMicroseconds(100);   // commands need > 37us to settle
}

void LCD1602Display__write4bits(LCD1602Display *display, int value) {
	for (int i = 0; i < 4; i++) {
		digitalWrite(display->data_pins[i + 4], (value >> i) & 0x01);
	}

	LCD1602Display__pulse_enable(display);
}

void LCD1602Display__write8bits(LCD1602Display *display, int value) {
	for (int i = 0; i < 8; i++) {
		digitalWrite(display->data_pins[i], (value >> i) & 0x01);
	}

	LCD1602Display__pulse_enable(display);
}

