/*
 * ccs811.h
 *
 *  Created on: 4 abr. 2021
 *      Author: Victoria M. Gullon and Marcos Gomez
 *      Parts by: Alvaro Salazar <alvaro@denkitronik.com> (github.com/denkitronik/CCS811)
 */

#ifndef SENSORS_CCS811_H_
#define SENSORS_CCS811_H_

#include<stdint.h>
#include "../libs/fsm.h"
#include "../libs/timerlib.h"
#include "../libs/circularbuffer.h"

#define FLAG_CO2_PENDING_MEASUREMENT 0x04

#define MODE0_IDLE 0b000
#define MODE1_EACH_1S 0b001
#define MODE2_EACH_10S 0b010
#define MODE3_EACH_60S 0b011
#define MODE4_EACH_250MS 0b100

//#ifdef DEB
//#define  DEBUG(...)  fprintf( stderr, __VA_ARGS__ )
//#else
#define DEBUG(fmt, ...)   do {} while (0)
//#endif

#define CCS811_ADDR_HIGH    0x5b    ///< Pin Addr is high: sets address to 0x5b
#define CCS811_ADDR_LOW     0x5a    ///< Pin Addr is low: sets address to 0x5a
#define STATUS              0x00,1  ///< (R) (Bootloader) (Application) Status register. Read only, 1 byte
#define MEAS_MODE           0x01,1  ///< (RW) (Application) meas_mode register, Read/Write 1 byte
#define ALG_RESULT_DATA     0x02,8  ///< (R) (Application) Algorythm result data register. Read, up to 8 bytes
#define RAW_DATA            0x03,2  ///< (R) (Application) Raw data register. Read, 2 bytes
#define ENV_DATA            0x05,4  ///< (W) (Application) Environment data register. Write, 4 bytes
#define NTC                 0x06,4  ///< (R) (Application) (Old V1.0 Only) NTC sensor register. Read, 4 bytes
#define THRESHOLDS          0x10,4  ///< (W) (Application) Thresholds register. Write, 4 bytes
#define BASELINE            0x11,2  ///< (RW) (Application) Baseline register. Read/Write, 2 bytes
#define HW_ID               0x20,1  ///< (R) (Bootloader) (Application) Hardware id register. Read, 1 byte
#define HW_VERSION          0x21,1  ///< (R) (Bootloader) (Application) Hardware version register. Read, 1 byte
#define FW_BOOT_VERSION     0x23,2  ///< (R) (Bootloader) (Application) Firmware boot version register. Read, 2 bytes
#define FW_APP_VERSION      0x24,2  ///< (R) (Bootloader) (Application) Firmware application version register. Read, 2 bytes
#define INTERNAL_STATE      0xA0,1  ///< (R) (Application) Internal Status register (No documentation available for this register). Read, 1 byte
#define ERROR_ID            0xE0,1  ///< (R) (Bootloader) (Application) Error Id register. Read, 1 byte
#define APP_ERASE           0xF1,4  ///< (W) (Bootloader) If the correct 4 bytes (0xE7 0xA7 0xE6 0x09) are written to this register in a single sequence the device will start the application erase
#define APP_DATA            0xF2,9  ///< (W) (Bootloader) Transmit flash code for the bootloader to write to the application flash code space.
#define APP_VERIFY          0xF3,0  ///< (W) (Bootloader) Starts the process of the bootloader checking though the application to make sure a full image is valid.
#define APP_START           0xF4,0  ///< (W) (Bootloader) Application start. Used to transition the CCS811 state from boot to application mode, a write with no data is required. Before performing a write to APP_START the Status register should be accessed to check if there is a valid application present.
#define SW_RESET            0xFF,4  ///< (W) (Bootloader) (Application) Software reset register. Write, 4 bytes
#define ERROR               -1      ///< Error code. A function could return this in error case.
#define OK                  0       ///< OK code. A function could return this in successful case.

union ApplicationRegister {
	///Structure of the status register of the CCS811. (See datasheet [1])
	struct {
		uint8_t error :1;          ///< 0: No error.\n 1: There is an error on the I²C or sensor, the ERROR_ID register (0xE0) contains the error source
		uint8_t reserved1 :2;      ///< Not used bits
		uint8_t data_ready :1;     ///< 0: No new data samples are ready.\n 1: A new data sample is ready in ALG_RESULT_DATA, this bit is cleared when ALG_RESULT_DATA is read
		uint8_t app_valid :1;      ///< 0: No application firmware loaded.\n 1: Valid application firmware loaded.
		uint8_t app_verify :1; ///< (Boot mode only)\n 0: 0: No verify completed.\n 1: Application verify operation completed.\n After issuing a VERIFY command the application software must wait 70ms before issuing any transactions to CCS811 over the I²C interface.
		uint8_t app_erase :1;      ///< (Boot mode only)\n 0: No erase completed.\n 1: Application erase operation completed.
		uint8_t fwmode :1;         ///< 0: Firmware is in boot mode, this allows new firmware to be loaded.\n 1: Firmware is in application mode.
	} status;

	///Structure of the measurement mode register of the CCS811. (See datasheet [1])
	struct {
		uint8_t reserved1 :2;      ///< Not used bits
		uint8_t int_thresh :1; ///< 0: Normal interrupt mode (if enabled).\n 1: Interrupt mode (if enabled) only asserts the nINT signal (driven low) if the new ALG_RESULT_DATA crosses one of the thresholds set in the THRESHOLDS register by more than the hysteresis value
		uint8_t int_data_ready :1; ///< 0: Interrupt generation is disabled.\n 1: The nINT signal is asserted (driven low) when a new sample is ready in ALG_RESULT_DATA
		uint8_t driveMode :3;      ///< Sets the measurement mode
		uint8_t reserved2 :1;      ///< Not used bit
	} meas_mode;

	///Structure of the algorythm result data register of the CCS811. (See datasheet [1])
	struct {
		uint16_t eco2 :16;             ///< eCO2 ppm
		uint16_t tvoc :16;             ///< TVOC ppb
		uint8_t status :8;             ///< Status register
		uint8_t error_id :8;           ///< Error_id register
		uint8_t raw_data_current :6;   ///< The value of the current through the sensor (0μA to 63μA)
		uint16_t raw_data_adc :10;     ///< 10bit ADC voltage across the sensor with the selected current (1023 = 1.65V)
	} alg_result_data;                  //Algorithm Results Data

	///Structure of the environment data register of the CCS811. (See datasheet [1])
	struct {
		uint16_t temperature_fraction :9;  ///< Temperature fraction in 1/512 degree celsius
		uint8_t temperature :7;            ///< Temperature integer part in degree celsius
		uint16_t humidity_fraction :9;     ///< Humidity fraction in 1/512%RH
		uint8_t humidity :7;               ///< Humidity integer part in %RH
	} env_data;                             ///Environment Data

	///Structure of the NTC register of the CCS811. (See datasheet [1])
	struct {
		uint16_t v_Rntc :16;   ///< Voltage across R_NTC (mV)
		uint16_t v_Rref :16;   ///< Voltage across R_REF (mV)
	} ntc;                      //NTC Register

	///Structure of the threshold register of the CCS811. (See datasheet [1])
	struct {
		uint8_t hysteresis :8;         ///< (Old V1.0 only) Hysteresis in ppm of eCO2 in order to avoid multiple interrupts
		uint16_t med_high_thresh :16;  ///< Medium to high threshold (in ppm) for eCO2 interrupt
		uint16_t low_med_thresh :16;   ///< Low to medium threshold (in ppm) for eCO2 interrupt
	} threshold;                        //Threshold Register

	///Structure of the Firmware bootloader version register of the CCS811. (See datasheet [1])
	struct {
		uint8_t trivial :8;    ///< Trivial number of version
		uint8_t minor :4;      ///< Minor number of version
		uint8_t major :4;      ///< Major number of version
	} fw_boot_version;          //Firmware bootloader version. Format: MAJOR.MINOR.TRIVIAL

	///Structure of the Firmware application version register of the CCS811. (See datasheet [1])
	struct {
		uint8_t trivial :8;    ///< Trivial number of version
		uint8_t minor :4;      ///< Minor number of version
		uint8_t major :4;      ///< Major number of version
	} fw_app_version;           //Firmware application version. Format: MAJOR.MINOR.TRIVIAL

	///Structure of the error id register of the CCS811. (See datasheet [1])
	struct {
		uint8_t write_reg_invalid :1;  ///< Invalid register address ID
		uint8_t read_reg_invalid :1;   ///< Read request to a mailbox ID that is invalid
		uint8_t meas_mode_invalid :1;  ///< Request to write an unsupported mode to MEAS_MODE
		uint8_t max_resistance :1;     ///< The sensor resistance measurement has reached or exceeded the maximum range
		uint8_t heater_fault :1;       ///< The Heater current in the CCS811 is not in range
		uint8_t heater_supply :1;      ///< The Heater voltage is not being applied correctly
		uint8_t reserved :2;           ///< Reserved
	} error_id;

	/* Warning:
	 * SW_RESET Register (0xFF)
	 * If the correct 4 bytes (0x11 0xE5 0x72 0x8A) are written to this
	 * register in a single sequence the device will reset and return to
	 * BOOT mode.
	 */

	uint8_t sw_reset;       ///< Software reset register
	uint16_t baseline;      ///< Encoded version of the current baseline used in Algorithm Calculations
	uint8_t hw_id;          ///< Hardware id register
	uint8_t hw_version;     ///< Hardware version register
	char buffer[8];         ///< Buffer to store data result register
};

typedef struct {
	int id; // sensor id
	int addr; // sensor i2c address
	int addr_pin; // address setting pin
	int interrupt_pin; // interrupt pin
	int rst_pin; // reset pin
	int file; // file descriptor for i2c

	union ApplicationRegister app_register; // application register

	fsm_t *fsm; // FSM that performs a measurement from the co2 sensor
	tmr_t *timer; // timer that goberns a flag used by the co2 sensor measurement FSM (x s periodic) // no se el tiempo aun
} CCS811Sensor;

CCS811Sensor* CCS811Sensor__create(int id, int addr, int addr_pin, int interrupt_pin, int rst_pin);
void CCS811Sensor__destroy(CCS811Sensor *sensor_instance);
void CCS811Sensor__set_app_register(CCS811Sensor *sensor_instance, union ApplicationRegister app_register);
int CCS811Sensor__connect(CCS811Sensor *sensor_instance);
int CCS811Sensor__read_register(CCS811Sensor *sensor_instance, const char reg, int n_bytes);
int CCS811Sensor__write_register(CCS811Sensor *sensor_instance, const char reg, const int n_bytes);
void CCS811Sensor__print_errors(CCS811Sensor *sensor_instance, char *msg);
int CCS811Sensor__set_environment_data(CCS811Sensor *sensor_instance, float temp, float humidity);
void CCS811Sensor__reset(CCS811Sensor *sensor_instance);
uint8_t CCS811Sensor__available(CCS811Sensor *sensor_instance);
int CCS811Sensor_print_status(CCS811Sensor *sensor_instance);
void CCS811Sensor_clear_app_register(CCS811Sensor *sensor_instance);
void CCS811Sensor_print_app_register(CCS811Sensor *sensor_instance);

#endif /* SENSORS_CCS811_H_ */
