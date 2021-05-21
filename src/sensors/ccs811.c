/*
 * ccs811.c
 *
 *  Created on: 4 abr. 2021
 *      Author: Victoria M. Gullon and Marcos Gomez
 */

#include <wiringPi.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "ccs811.h"
#include "../utils.h"
#include "../libs/systemlib.h"
#include "../libs/systemtype.h"

const char getRegister(const char reg, const int numBytes);
char* getSelectedRegister(char registerSelected);
int selectRegister(CCS811Sensor *sensor_instance, const char reg);
int readI2C(CCS811Sensor *sensor_instance, const int numBytes);
int writeI2CBytes(CCS811Sensor *sensor_instance, const char reg, int numBytes);

// FSM Functions and variables

extern int measurement_flags; // grab global co2 flags

// Timer
static void _co2_timer_isr(union sigval value);

// FSM states enum
enum _light_fsm_state {
	CO2_MEASUREMENT
};

// FSM input check functions
static int _co2_pending_measurement(fsm_t *this);

// FSM output action functions
static void _co2_do_measurement(fsm_t *this);

// { EstadoOrigen, CondicionDeDisparo, EstadoFinal, AccionesSiTransicion }
static fsm_trans_t _co2_fsm_tt[] = { { CO2_MEASUREMENT, _co2_pending_measurement, CO2_MEASUREMENT, _co2_do_measurement }, { -1, NULL, -1, NULL } };

/************************/
CCS811Sensor* CCS811Sensor__create(int id, int addr, int addr_pin, int interrupt_pin, int rst_pin) {
	CCS811Sensor *result = (CCS811Sensor*) malloc(sizeof(CCS811Sensor));

	result->id = id;
	result->addr = addr;

	result->addr_pin = addr_pin;
	result->interrupt_pin = interrupt_pin;
	result->rst_pin = rst_pin;

	// Timer instantiation
	tmr_t *co2_timer = tmr_new(_co2_timer_isr); // creado pero no iniciado
	result->timer = co2_timer;

	// FSM creation
	result->fsm = (fsm_t*) fsm_new(CO2_MEASUREMENT, _co2_fsm_tt, result); //3rd param pointer available under user_data

	return result;

}

void CCS811Sensor__destroy(CCS811Sensor *sensor_instance) {
	if (sensor_instance) {
		fsm_destroy(sensor_instance->fsm);
		tmr_destroy(sensor_instance->timer);
		free(sensor_instance);
	}
}

void CCS811Sensor__set_app_register(CCS811Sensor *sensor_instance, union ApplicationRegister app_register) {
	sensor_instance->app_register = app_register;
}

int CCS811Sensor__connect(CCS811Sensor *sensor_instance) {
	if (sensor_instance->addr_pin != -1) {
		switch (sensor_instance->addr) {
		case CCS811_ADDR_HIGH:
			digitalWrite(sensor_instance->addr_pin, HIGH);
			break;
		case CCS811_ADDR_LOW:
			digitalWrite(sensor_instance->addr_pin, LOW);
			break;
		default:
			digitalWrite(sensor_instance->addr_pin, HIGH);
			break;
		}
	}

	char *filename = "/dev/i2c-1";
	if ((sensor_instance->file = open(filename, O_RDWR)) < 0) {
		DEBUG("Connecting to the I2C bus failed");
		return ERROR;
	}
	if (ioctl(sensor_instance->file, I2C_SLAVE, sensor_instance->addr) < 0) {
		DEBUG("Setting the CCS811 as slave failed\r\n");
		return ERROR;
	}

	CCS811Sensor__read_register(sensor_instance, HW_ID);

	if (sensor_instance->app_register.hw_id == 0x81) {
		CCS811Sensor__write_register(sensor_instance, STATUS);
		CCS811Sensor__write_register(sensor_instance, APP_START);
		return OK;
	} else {
		DEBUG("HW_ID = 0x81 does not match with this device.\r\n");
		return ERROR;
	}
}

int CCS811Sensor__read_register(CCS811Sensor *sensor_instance, const char reg, int n_bytes) {
	if (selectRegister(sensor_instance, reg) == OK) {
		if (readI2C(sensor_instance, n_bytes) == OK) {
			if (getRegister(ALG_RESULT_DATA) == getRegister(reg, 0))
				if (sensor_instance->app_register.alg_result_data.eco2 > 255) {
					sensor_instance->app_register.alg_result_data.eco2 = ((sensor_instance->app_register.alg_result_data.eco2 >> 8) | (sensor_instance->app_register.alg_result_data.eco2 << 8));
					if (sensor_instance->app_register.alg_result_data.eco2 > 32768)
						sensor_instance->app_register.alg_result_data.eco2 = sensor_instance->app_register.alg_result_data.eco2 - 32768;
				}
			if (sensor_instance->app_register.alg_result_data.tvoc > 255) {
				sensor_instance->app_register.alg_result_data.tvoc = ((sensor_instance->app_register.alg_result_data.tvoc >> 8) | (sensor_instance->app_register.alg_result_data.tvoc << 8));
				if (sensor_instance->app_register.alg_result_data.tvoc > 32768)
					sensor_instance->app_register.alg_result_data.tvoc = sensor_instance->app_register.alg_result_data.tvoc - 32768;
			}
			return OK;
		} else
			return ERROR;
	} else
		return ERROR;
}

int CCS811Sensor__write_register(CCS811Sensor *sensor_instance, const char reg, const int n_bytes) {
	return writeI2CBytes(sensor_instance, reg, n_bytes);
}

void CCS811Sensor__print_errors(CCS811Sensor *sensor_instance, char *msg) {
	union ApplicationRegister app_reg_aux;
	app_reg_aux = sensor_instance->app_register;
	char error = 0;

	CCS811Sensor__read_register(sensor_instance, ERROR_ID);
	printf("............................\r\n");
	printf(".......%s ERRORS........\r\n", msg);
	printf("............................\r\n");
	if (sensor_instance->app_register.error_id.heater_fault) {
		printf("The Heater current in the CCS811 is not in range.\r\n");
		error = 1;
	}
	if (sensor_instance->app_register.error_id.heater_supply) {
		printf("The Heater voltage is not being applied correctly.\r\n");
		error = 1;
	}
	if (sensor_instance->app_register.error_id.max_resistance) {
		printf("The sensor resistance measurement has reached or exceeded the maximum range.\r\n");
		error = 1;
	}
	if (sensor_instance->app_register.error_id.meas_mode_invalid) {
		printf("Request to write an unsupported mode to MEAS_MODE.\r\n");
		error = 1;
	}
	if (sensor_instance->app_register.error_id.read_reg_invalid) {
		printf("Read request to a mailbox ID that is invalid.\r\n");
		error = 1;
	}
	if (sensor_instance->app_register.error_id.write_reg_invalid) {
		printf("Write request to a mailbox ID that is invalid.\r\n");
		error = 1;
	}
	if (!error) {
		printf("No errors happened!\r\n");
	}
	printf("............................\r\n");

	sensor_instance->app_register = app_reg_aux;
}

int CCS811Sensor__set_environment_data(CCS811Sensor *sensor_instance, float temp, float humidity) {
	int humidityInteger = (int) humidity;
	int temperatureInteger = (int) temp;

	CCS811Sensor_clear_app_register(sensor_instance);
	sensor_instance->app_register.env_data.humidity = humidityInteger;
	sensor_instance->app_register.env_data.humidity_fraction = (uint16_t) ((humidity - humidityInteger) * 512);
	sensor_instance->app_register.env_data.temperature = temperatureInteger + 25;
	sensor_instance->app_register.env_data.temperature_fraction = (uint16_t) ((temp - temperatureInteger) * 512);
	if (CCS811Sensor__write_register(sensor_instance, ENV_DATA) == ERROR)
		return ERROR;
	return OK;
}

void CCS811Sensor__reset(CCS811Sensor *sensor_instance) {
	pinMode(sensor_instance->rst_pin, OUTPUT);
	digitalWrite(sensor_instance->rst_pin, LOW);
	digitalWrite(sensor_instance->rst_pin, HIGH);
}

uint8_t CCS811Sensor__available(CCS811Sensor *sensor_instance) {
	pinMode(sensor_instance->interrupt_pin, INPUT);
	return digitalRead(sensor_instance->interrupt_pin);
}

int CCS811Sensor_print_status(CCS811Sensor *sensor_instance) {
	union ApplicationRegister app_reg_aux;
	app_reg_aux = sensor_instance->app_register;
	if (CCS811Sensor__read_register(sensor_instance, STATUS) == ERROR)
		return ERROR;

	printf("app_valid is 0x%x\r\n", app_reg_aux.status.app_valid);
	printf("data_ready is 0x%x\r\n", app_reg_aux.status.data_ready);
	printf("error is 0x%x\r\n", app_reg_aux.status.error);
	printf("fwmode is 0x%x\r\n", app_reg_aux.status.fwmode);

	sensor_instance->app_register = app_reg_aux;
	return OK;
}

void CCS811Sensor_clear_app_register(CCS811Sensor *sensor_instance) {
	for (int i = 0; i < 8; i++)
		sensor_instance->app_register.buffer[i] = 0;
}

void CCS811Sensor_print_app_register(CCS811Sensor *sensor_instance) {
	printf("printAppReg:\t\t");
	for (int i = 0; i < 8; i++)
		printf("0x%x ", sensor_instance->app_register.buffer[i]);
	printf("\r\n");
}

char* getSelectedRegister(char registerSelected) {
	switch (registerSelected) {
	case 0x00:
		return "STATUS\r\n";
		break;
	case 0x01:
		return "MEAS_MODE\r\n";
		break;
	case 0x02:
		return "ALG_RESULT_DATA\r\n";
		break;
	case 0x03:
		return "RAW_DATA\r\n";
		break;
	case 0x05:
		return "ENV_DATA\r\n";
		break;
	case 0x06:
		return "NTC\r\n";
		break;
	case 0x10:
		return "THRESHOLDS\r\n";
		break;
	case 0x11:
		return "BASELINE\r\n";
		break;
	case 0x20:
		return "HW_ID\r\n";
		break;
	case 0x21:
		return "HW_VERSION\r\n";
		break;
	case 0x23:
		return "FW_BOOT_VERSION\r\n";
		break;
	case 0x24:
		return "FW_APP_VERSION\r\n";
		break;
	case 0xA0:
		return "INTERNAL_STATE\r\n";
		break;
	case 0xE0:
		return "ERROR_ID\r\n";
		break;
	case 0xF1:
		return "APP_ERASE\r\n";
		break;
	case 0xF2:
		return "APP_DATA\r\n";
		break;
	case 0xF3:
		return "APP_VERIFY\r\n";
		break;
	case 0xF4:
		return "APP_START\r\n";
		break;
	case 0xFF:
		return "SW_RESET\r\n";
		break;

	default:
		return "UNK_REGISTER\r\n";
		break;
	}
}

int selectRegister(CCS811Sensor *sensor_instance, const char reg) {
	const char buffer[1] = { reg };
	if (write(sensor_instance->file, buffer, 1) != 1) {
		DEBUG("Writing to the CCS811 failed (I2C transaction failed).\r\n");
		return ERROR;
	} else {
		DEBUG(getSelectedRegister(buffer[0]));
		return OK;
	}
}

int readI2C(CCS811Sensor *sensor_instance, const int numBytes) {
	CCS811Sensor_clear_app_register(sensor_instance);
	if (read(sensor_instance->file, sensor_instance->app_register.buffer, numBytes) != numBytes) {
		DEBUG("Reading the I2C bus failed\r\n");
		return ERROR;
	} else {
		DEBUG("readI2C:\t\t");
		for (char i = 0; i < 8; i++)
			DEBUG("0x%x ", sensor_instance->app_register.buffer[i]);
		DEBUG("count=%d\r\n", numBytes);
		return OK;
	}
}

int writeI2CBytes(CCS811Sensor *sensor_instance, const char reg, int numBytes) {
	unsigned char bufferData[9] = { reg, 0, 0, 0, 0, 0, 0, 0, 0 };
	for (int i = 0; i < numBytes; i++)
		bufferData[i + 1] = sensor_instance->app_register.buffer[i];
	if (write(sensor_instance->file, bufferData, (numBytes + 1)) != (numBytes + 1)) {
		/* ERROR HANDLING: i2c transaction failed */
		DEBUG("Writing to the I2C bus failed\r\n");
		return ERROR;
	} else {
		DEBUG("writeI2CBytes: \t\t");
		for (int i = 0; i < 9; i++)
			DEBUG("0x%x ", bufferData[i]);
		DEBUG("count=%d\r\n", numBytes + 1);
		return OK;
	}
}

const char getRegister(const char reg, const int numBytes) {
	return reg;
}

/************************/

static void _co2_timer_isr(union sigval value) {
	piLock(MEASUREMENT_LOCK);
	measurement_flags |= FLAG_CO2_PENDING_MEASUREMENT;
	piUnlock(MEASUREMENT_LOCK);
}

static int _co2_pending_measurement(fsm_t *this) {
	return (measurement_flags & FLAG_CO2_PENDING_MEASUREMENT);
}

static void _co2_do_measurement(fsm_t *this) {
	CCS811Sensor *ccs = (CCS811Sensor*) this->user_data;

	extern SystemType *roompi_system; // get the current system
	SensorValueType t_value = roompi_system->root_system->sensor_values[0];
	SensorValueType rh_value = roompi_system->root_system->sensor_values[1];

	if (t_value.type != is_error && rh_value.type != is_error) { //no se si se puede hacer esto porque ahora esto es controlado por master y quizas aunque siga el flag de pending activo ya tenemos medida quw poder usar
		//CCS811Sensor__set_environment_data(ccs, t_value.val.fval, rh_value.val.fval);
	} else {
		//CCS811Sensor__set_environment_data(ccs, 25.0, 50.0);
	}

	// check if co2 measurement available, if not nothing happens
		delay(200);
		CCS811Sensor__read_register(ccs, ALG_RESULT_DATA);
		int eco2 = ccs->app_register.alg_result_data.eco2;
		int err = ccs->app_register.status.error; // No error: 0, Error: 1
		SensorValueType res_co2_val; // craft SensorValueType instance with type Integer and value measured co2 or error

		if (eco2 == 0)
			err = 1;
		if (err > 0) {
			// we have an error
			res_co2_val.type = is_error;
			res_co2_val.val.ival = 0;
		} else {
			res_co2_val.type = is_int;
			res_co2_val.val.ival = eco2;
		}

		piLock(STORAGE_LOCK);
		CircularBufferPush(roompi_system->root_system->sensor_storage[3], &res_co2_val, sizeof(res_co2_val)); // co2 circular buffer is at index 2 of the table
		piUnlock(STORAGE_LOCK);

		piLock(MEASUREMENT_LOCK);
		measurement_flags &= ~(FLAG_CO2_PENDING_MEASUREMENT);
		piUnlock(MEASUREMENT_LOCK);
	}
