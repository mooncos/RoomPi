/*
 * bh1750.c
 *
 *  Created on: 13 mar. 2021
 *      Author: Victoria M. Gullon and Marcos Gomez
 */


#include <wiringPi.h>
#include <wiringPiI2C.h> // for I2C SMBUS communication
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <byteswap.h> // byte swapping native processor macros

/************************/

#include "bh1750.h"
#include "../utils.h"
#include "../libs/systemlib.h"
#include "../libs/systemtype.h"

// FSM Functions and variables

extern int measurement_flags; // grab global light flags

// Timer
static void _light_timer_isr(union sigval value);

// FSM states enum
enum _light_fsm_state { LIGHT_MEASUREMENT };

// FSM input check functions
static int _light_pending_measurement(fsm_t *this);

// FSM output action functions
static void _light_do_measurement(fsm_t *this);

// { EstadoOrigen, CondicionDeDisparo, EstadoFinal, AccionesSiTransicion }
static fsm_trans_t _light_fsm_tt[] = {
		{ LIGHT_MEASUREMENT, _light_pending_measurement, LIGHT_MEASUREMENT, _light_do_measurement },
		{-1, NULL, -1, NULL}
};

/************************/


BH1750Sensor* BH1750Sensor__create(int id, int addr, int mode) {
	BH1750Sensor* result = (BH1750Sensor*) malloc(sizeof(BH1750Sensor));
	result->id = id;
	result->addr = addr;
	result->mode = mode;
	result->lux = 0;
	result->fd = wiringPiI2CSetup(addr);

	// Timer instantiation
	tmr_t *light_timer = tmr_new(_light_timer_isr); // creado pero no iniciado
	result->timer = light_timer;

	// FSM creation
	result->fsm = (fsm_t *) fsm_new(LIGHT_MEASUREMENT, _light_fsm_tt, result); //3rd param pointer available under user_data


	return result;
}

void BH1750Sensor__destroy(BH1750Sensor* sensor_instance)  {
	if (sensor_instance) {
		fsm_destroy(sensor_instance->fsm);
		tmr_destroy(sensor_instance->timer);
		free(sensor_instance);
	}
}

int BH1750Sensor__lux_value(BH1750Sensor* sensor_instance) {
	return sensor_instance->lux;
}

int BH1750Sensor__perform_measurement(BH1750Sensor* sensor_instance) {
	//wiringPiI2CWrite(sensor_instance->fd, RESET);
	int r = wiringPiI2CWrite(sensor_instance->fd, sensor_instance->mode); // error if function returns < 0
	delay(180);
	short word = wiringPiI2CReadReg16(sensor_instance->fd, 0x00);
	word = bswap_16(word);
	sensor_instance->lux = word / 1.2;
	return r;
}

/************************/

static void _light_timer_isr(union sigval value) {
	piLock(MEASUREMENT_LOCK);
	measurement_flags |= FLAG_LIGHT_PENDING_MEASUREMENT;
	piUnlock(MEASUREMENT_LOCK);
}

static int _light_pending_measurement(fsm_t *this) {
	return (measurement_flags & FLAG_LIGHT_PENDING_MEASUREMENT);
}

static void _light_do_measurement(fsm_t *this) {
	BH1750Sensor* bh = (BH1750Sensor*) this->user_data;
	int r = BH1750Sensor__perform_measurement(bh);

	SensorValueType res_light_val; // craft SensorValueType instance with type Integer and value measured lux or error
	if (r < 0) {
		// we have an error
		res_light_val.type = is_error;
		res_light_val.val.ival = 0;
	} else {
		piLock(MEASUREMENT_LOCK);
		measurement_flags &= ~(FLAG_LIGHT_PENDING_MEASUREMENT);
		piUnlock(MEASUREMENT_LOCK);
		res_light_val.type = is_int;
		res_light_val.val.ival = BH1750Sensor__lux_value(bh);
	}

	extern SystemType *roompi_system; // get the current system
	piLock(STORAGE_LOCK);
	CircularBufferPush(roompi_system->root_system->sensor_storage[2], (SensorValueType*) &res_light_val, sizeof(res_light_val)); // light circular buffer is at index 2 of the table
	piUnlock(STORAGE_LOCK);
}
