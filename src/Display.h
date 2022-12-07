/*
 * Display.h
 *
 *  Created on: Oct. 24, 2022
 *      Author: mihai
 */

#ifndef DISPLAY_H_
#define DISPLAY_H_

#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>

#include "Limits.h"
#include "Timer.h"

// blockcount is the height and width of the display
const int block_count = (int)MARGIN / (int)SCALER + 1;

class Display {
public:
	Display();
	~Display();
	int initialize();
	void start();
	int stop();
	static void *startDisplay(void *context);
	void *updateDisplay(void);
	void printMap();

private:
	// execution time members
	time_t startTime;
	time_t finishTime;


	// threads
	pthread_t displayThread;
	pthread_attr_t attr;
	pthread_mutex_t mutex; // mutex for display

	// Temporary values
	std::string map[block_count][block_count] = {
			{""}}; // Shrink 100k by 100k map to 10 by 10, each block is 10k by 10k
	std::string height_display = "";

	// shm members
	int shm_display; // Display required info
	void *ptr_display;
};

#endif /* DISPLAY_H_ */
