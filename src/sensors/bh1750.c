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

BH1750Sensor* BH1750Sensor__create(int id, int addr, int mode) {
	BH1750Sensor* result = (BH1750Sensor*) malloc(sizeof(BH1750Sensor));
	result->id = id;
	result->addr = addr;
	result->mode = mode;
	result->lux = 0;
	result->fd = wiringPiI2CSetup(addr);

	return result;
}

void BH1750Sensor__destroy(BH1750Sensor* sensor_instance)  {
	if (sensor_instance) {
		free(sensor_instance);
	}
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

int BH1750Sensor__lux_value(BH1750Sensor* sensor_instance) {
	return sensor_instance->lux;
}
