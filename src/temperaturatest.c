/*
 ============================================================================
 Name        : Test1.c
 Author      : marcos y victoria
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <wiringPi.h>

#include "sensors/dht11.h"

int temperaturatest(void) {

	wiringPiSetup();

	DHT11Sensor *misensor = DHT11Sensor__create(1, 3);
	float mitemperatura, mihumedad;
	int err = DHT11Sensor__perform_measurement(misensor);

	while (err != 0) {
		err = DHT11Sensor__perform_measurement(misensor);
		switch (err) {
		case 2:
			printf("Error en el checksum, repitiendo medida...\n");
			break;
		case 0:
			break;
		default:
			return 1;
			break;
		}
	}
	//mitemperatura = DHT11Sensor__t_value(misensor);
	//mihumedad = DHT11Sensor__rh_value(misensor);
	printf("Lectura correcta, imprimiendo valores del sensor %d\n",
			misensor->id);
	mitemperatura = misensor->t_value;
	mihumedad = misensor->rh_value;

	printf("La temperatura medida es: %.2f\nLa humedad medida es: %.2f\n",
			mitemperatura, mihumedad);



	DHT11Sensor__destroy(misensor);

	return 0;
}
