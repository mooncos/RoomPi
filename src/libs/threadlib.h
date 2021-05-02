/*
 * threadlib.h
 *
 * Custom Multithreading library based on wiringpi due to locking key numbers limitations.
 *
 *  Created on: 15 mar. 2021
 *      Author: Victoria M. Gullon and Marcos Gomez
 */

#ifndef LIBS_THREADLIB_H_
#define LIBS_THREADLIB_H_

#include <pthread.h>

#define	M_THREAD(X)	void *X (__attribute__((unused)) void *dummy)

int multithreadingThreadCreate (void *(*fn)(void *));
void multithreadingLock(int key);
void multithreadingUnlock(int key);

#endif /* LIBS_THREADLIB_H_ */
