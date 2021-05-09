/*
 * circularbuffer.h
 *
 *  Created on: 10 may. 2021
 *      Author: mgbra
 */

#ifndef CIRCULARBUFFER_H_
#define CIRCULARBUFFER_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

struct s_circularBuffer {

	size_t size; //capacity bytes size
	size_t dataSize; //occupied data size
	size_t tailOffset; //head offset, the oldest byte position offset
	size_t headOffset; //tail offset, the lastest byte position offset
	void *buffer;

};

/*
 A circular buffer(circular queue, cyclic buffer or ring buffer), is a data structure that uses a single, fixed-size buffer as if it were connected end-to-end. This structure lends itself easily to buffering data streams. visit https://en.wikipedia.org/wiki/Circular_buffer to see more information.
 */

typedef struct s_circularBuffer *CircularBuffer;

// Construct CircularBuffer with ‘size' in byte. You must call CircularBufferFree() in balance for destruction.
CircularBuffer CircularBufferCreate(size_t size);

// Destruct CircularBuffer
void CircularBufferFree(CircularBuffer cBuf);

// Reset the CircularBuffer
void CircularBufferReset(CircularBuffer cBuf);

//get the capacity of CircularBuffer
size_t CircularBufferGetCapacity(CircularBuffer cBuf);

//same as CircularBufferGetCapacity, Just for compatibility with older versions
size_t CircularBufferGetSize(CircularBuffer cBuf);

//get occupied data size of CircularBuffer
size_t CircularBufferGetDataSize(CircularBuffer cBuf);

// Push data to the tail of a circular buffer from 'src' with 'length' size in byte.
void CircularBufferPush(CircularBuffer cBuf, void *src, size_t length);

// Pop data from a circular buffer to 'dataOut'  with wished 'length' size in byte,return the actual data size in byte popped out,which is less or equal to the input 'length parameter.
size_t CircularBufferPop(CircularBuffer cBuf, size_t length, void *dataOut);

// Read data from a circular buffer to 'dataOut'  with wished 'length' size in byte,return the actual data size in byte popped out,which is less or equal to the input 'length parameter.
size_t CircularBufferRead(CircularBuffer cBuf, size_t length, void *dataOut);

//for test purpose, print the circular buffer's data content by printf(...); the 'hex' parameters indicates that if the data should be printed in asscii string or hex data format.
void CircularBufferPrint(CircularBuffer cBuf, bool hex);

#endif /* CIRCULARBUFFER_H_ */
