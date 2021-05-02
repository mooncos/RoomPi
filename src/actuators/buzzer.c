/*
 * buzzer.c
 *
 *  Created on: 10 mar. 2021
 *      Author: Victoria M. Gullon and Marcos Gomez
 */

#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/***************/

#include "buzzer.h"

BuzzerOutput* BuzzerOutput__create(int id, int data_pin) {
	BuzzerOutput *result = (BuzzerOutput*) malloc(sizeof(BuzzerOutput));
	result->data_pin = data_pin;
	result->status = 0;
	result->id = id;

	pinMode(result->data_pin, OUTPUT);
	BuzzerOutput__disable(result);

	return result;
}

void BuzzerOutput__destroy(BuzzerOutput *buzzer) {
	if (buzzer) {
		BuzzerOutput__disable(buzzer);
		free(buzzer);
	};
}

void BuzzerOutput__enable(BuzzerOutput *buzzer) {
	digitalWrite(buzzer->data_pin, HIGH);
	buzzer->status = 1;
}

void BuzzerOutput__disable(BuzzerOutput *buzzer) {
	digitalWrite(buzzer->data_pin, LOW);
	buzzer->status = 0;
}

void BuzzerOutput__toggle(BuzzerOutput *buzzer) {
	(buzzer->status) ?
			BuzzerOutput__disable(buzzer) : BuzzerOutput__enable(buzzer);
}
