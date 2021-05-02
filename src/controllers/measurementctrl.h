/*
 * measurementctrl.h
 *
 *  Created on: 15 mar. 2021
 *      Author: Victoria M. Gullon and Marcos Gomez
 */

#ifndef CONTROLLERS_MEASUREMENTCTRL_H_
#define CONTROLLERS_MEASUREMENTCTRL_H_

#include "../libs/fsm.h"
#include "../libs/timerlib.h"
#include "../libs/systemlib.h"

// 00X1111X1111X111
#define FLAG_TEMP_HUMID_PENDING_MEASUREMENT 0x01
#define FLAG_LIGHT_PENDING_MEASUREMENT 0x02
#define FLAG_CO2_PENDING_MEASUREMENT 0x04
// FUTURE: keep adding new sensors with their pending measurement flags (0x04 for C02 Sensor, 0x08 for Sound Level Sensor)
#define FLAG_TEMP_ANOMALY 0x10
#define FLAG_HUMID_ANOMALY 0x20
#define FLAG_LIGHT_ANOMALY 0x40
#define FLAG_CO2_ANOMALY 0x80
// FUTURE: keep adding sensors and anomalous values flags (0x80 C02 anomaly, 0x100 Sound Level anomaly)
#define FLAG_TEMP_EMERGENCY 0x200
#define FLAG_HUMID_EMERGENCY 0x400
#define FLAG_LIGHT_EMERGENCY 0x800
#define FLAG_CO2_EMERGENCY 0x1000
// FUTURE: keep adding sensors and emergency values flags (0x2000 Sound Level emergency)

#define MEASUREMENT_LOCK 0

typedef struct {
	fsm_t *fsm; // FSM that performs measurements from the various sensors and stores them in the sensors' objects
	tmr_t *timer; // timer that goberns a flag used by the measurement FSM (30 s periodic)
} MeasurementCtrl;

MeasurementCtrl* MeasurementCtrl__setup(SystemContext *this_system);
void MeasurementCtrl__destroy(MeasurementCtrl *this);

#endif /* CONTROLLERS_MEASUREMENTCTRL_H_ */
