/*
 * systemtype.h
 *
 *  Created on: 15 mar. 2021
 *      Author: Victoria M. Gullon and Marcos Gomez
 */

#ifndef LIBS_SYSTEMTYPE_H_
#define LIBS_SYSTEMTYPE_H_

#include <stdlib.h>

#include "../controllers/measurementctrl.h"
#include "../controllers/outputctrl.h"
#include "systemlib.h"

typedef struct {
	SystemContext* root_system;
	MeasurementCtrl* root_measurement_ctrl;
	OutputCtrl* root_output_ctrl;
} SystemType;

SystemType* SystemType__setup(SystemContext* system, MeasurementCtrl* measurementctrl, OutputCtrl* outputctrl);
void SystemType__destroy(SystemType* this);

#endif /* LIBS_SYSTEMTYPE_H_ */
