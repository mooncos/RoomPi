/*
 * utils.h
 *
 *  Created on: 10 mar. 2021
 *      Author: Victoria M. Gullon and Marcos Gomez
 */

#ifndef UTILS_H_
#define UTILS_H_

#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitToggle(value, bit) ((value) ^= (1UL << (bit)))

#endif /* UTILS_H_ */
