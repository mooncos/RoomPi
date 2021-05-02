/*
 * bh1750.h
 *
 *  Created on: 13 mar. 2021
 *      Author: Victoria M. Gullon and Marcos Gomez
 */

#ifndef BH1750_H_
#define BH1750_H_

// I2C Opcodes defines

#define POWER_DOWN 0x00
#define POWER_ON 0x01

#define RESET 0x07
#define CONTINUOUS_H_RES 0x10
#define CONTINUOUS_H_RES2 0x11
#define CONTINUOUS_L_RES 0x13

#define ONE_TIME_H_RES 0x20
#define ONE_TIME_H_RES2 0x21
#define ONE_TIME_L_RES 0x23

// Adjusting sensor sensitivity is possible by changing MTreg,
// but not necessary for our case

typedef struct {
	int id; // sensor id
	int addr; // sensor i2c address
	int mode; // sensor operating mode (continuous/one time-hires/lowres)
	int fd; // file descriptor handle representing the i2c device
	int lux;
} BH1750Sensor;

BH1750Sensor* BH1750Sensor__create(int id, int addr, int mode);
void BH1750Sensor__destroy(BH1750Sensor* sensor_instance);
int BH1750Sensor__perform_measurement(BH1750Sensor* sensor_instance);
int BH1750Sensor__lux_value(BH1750Sensor* sensor_instance);

#endif /* BH1750_H_ */
