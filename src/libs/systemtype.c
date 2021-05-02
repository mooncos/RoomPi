/*
 * systemtype.c
 *
 *  Created on: 15 mar. 2021
 *      Author: Victoria M. Gullon and Marcos Gomez
 */


#include "systemtype.h"
#include "../controllers/measurementctrl.h"

SystemType* SystemType__setup(SystemContext* system, MeasurementCtrl* measurementctrl, OutputCtrl* outputctrl) {
	SystemType* result = (SystemType*) malloc(sizeof(SystemType));
	result->root_system = system;
	result->root_measurement_ctrl = measurementctrl;
	result->root_output_ctrl = outputctrl;

	return result;
}

void SystemType__destroy(SystemType* this) {
	if (this) {
		MeasurementCtrl__destroy(this->root_measurement_ctrl);
		OutputCtrl__destroy(this->root_output_ctrl);
		SystemContext__destroy(this->root_system);
		free(this);
	}
}
