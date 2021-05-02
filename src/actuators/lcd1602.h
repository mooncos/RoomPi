/*
 * lcd1602.h
 *
 *  Created on: 9 mar. 2021
 *      Author: Victoria M. Gullon and Marcos Gomez
 */

#ifndef LCD1602_H_
#define LCD1602_H_

typedef struct {
	int id; // identificador del display
	int rs_pin; // Register Select Pin
	int rw_pin; // Read Write Pin
	int enable_pin; // Enable Pin
	int data_pins[8]; // Data pins (8 bit)
	int _displayfunction; // Variable to store the display functioning mode
	int _displaycontrol; // Variable to store the display control mode
	int _displaymode; // Variable to store the display mode
	int _initialized; // is initialized?
	int _numlines; // number of lines used
	int _row_offsets[4]; // Display row offsets
	int _fourbitmode; // is 4 or 8 bit mode
} LCD1602Display;

LCD1602Display* LCD1602Display__create(int id, int rs, int rw, int enable,
		int fourbitmode, int d0, int d1, int d2, int d3, int d4, int d5, int d6,
		int d7);
void LCD1602Display__destroy(LCD1602Display *display);
void LCD1602Display__begin(LCD1602Display *display, int cols, int lines,
		int dotsize);
void LCD1602Display__set_row_offsets(LCD1602Display *display, int row0,
		int row1, int row2, int row3);

// high level commands
void LCD1602Display__clear(LCD1602Display *display);
void LCD1602Display__home(LCD1602Display *display);
void LCD1602Display__set_cursor(LCD1602Display *display, int col, int row);
void LCD1602Display__no_display(LCD1602Display *display);
void LCD1602Display__display(LCD1602Display *display);
void LCD1602Display__no_cursor(LCD1602Display *display);
void LCD1602Display__cursor(LCD1602Display *display);
void LCD1602Display__no_blink(LCD1602Display *display);
void LCD1602Display__blink(LCD1602Display *display);
void LCD1602Display__scroll_display_left(LCD1602Display *display);
void LCD1602Display__scroll_display_right(LCD1602Display *display);
void LCD1602Display__left_to_right(LCD1602Display *display);
void LCD1602Display__right_to_left(LCD1602Display *display);
void LCD1602Display__autoscroll(LCD1602Display *display);
void LCD1602Display__no_autoscroll(LCD1602Display *display);
int LCD1602Display__print(LCD1602Display *display, char *fmt, ...);
// create custom chars
void LCD1602Display__create_char(LCD1602Display *display, int addr,
		int charmap[]);

// mid level commands
void LCD1602Display__command(LCD1602Display *display, int value);
void LCD1602Display__write(LCD1602Display *display, int value);

// low level commands
void LCD1602Display__send(LCD1602Display *display, int value, int mode);
void LCD1602Display__pulse_enable(LCD1602Display *display);
void LCD1602Display__write4bits(LCD1602Display *display, int value);
void LCD1602Display__write8bits(LCD1602Display *display, int value);

#endif /* LCD1602_H_ */
