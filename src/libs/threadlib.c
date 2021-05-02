/*
 * threadlib.c
 *
 *  Created on: 15 mar. 2021
 *      Author: Victoria M. Gullon and Marcos Gomez
 */

#include <stdlib.h>
#include <pthread.h>

static pthread_mutex_t multithreadingMutexes[12];

int multithreadingThreadCreate(void* (*fn)(void*)) {
	pthread_t myThread;

	return pthread_create(&myThread, NULL, fn, NULL);
}

void multithreadingLock(int key) {
	pthread_mutex_lock(&multithreadingMutexes[key]);
}

void multithreadingUnlock(int key) {
	pthread_mutex_unlock(&multithreadingMutexes[key]);
}
