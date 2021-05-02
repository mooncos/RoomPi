/*
 * buzzer.h
 *
 *  Created on: 10 mar. 2021
 *      Author: Victoria M. Gullon and Marcos Gomez
 */

#ifndef BUZZER_H_
#define BUZZER_H_

typedef struct {
	int id; // entero
	int status;
	int data_pin;
} BuzzerOutput;

BuzzerOutput* BuzzerOutput__create(int id, int data_pin);
void BuzzerOutput__destroy(BuzzerOutput* buzzer);
void BuzzerOutput__enable(BuzzerOutput* buzzer);
void BuzzerOutput__disable(BuzzerOutput* buzzer);
void BuzzerOutput__toggle(BuzzerOutput* buzzer);

#endif /* BUZZER_H_ */
