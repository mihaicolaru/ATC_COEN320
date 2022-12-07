/*
 * Plane.h
 *
 *  Created on: Oct. 24, 2022
 *      Author: mihai
 */

#ifndef PLANE_H_
#define PLANE_H_

#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <pthread.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/neutrino.h>
#include <sys/siginfo.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "Limits.h"
#include "Timer.h"

class Plane {
public:
	// constructor
	Plane(int _ID, int _arrivalTime, int _position[3], int _speed[3]);

	// destructor
	~Plane();

	// call static function to start thread
	int start();

	// join execution thread
	bool stop();

	// entry point for execution thread
	static void *startPlane(void *context);

	// get plane file descriptor as const char array
	const char *getFD() { return fileName.c_str(); }

private:
	// initialize thread and shm members
	int initialize();

	// update position every second from position and speed
	void *flyPlane(void);

	// check comm system for potential commands
	void answerComm();

	// update position based on speed
	void updatePosition();

	// stringify plane data members
	void updateString();

	// check airspace limits for operation termination
	int checkLimits();

	// print plane info
	void Print();

	// data members
	int arrivalTime;
	int ID;
	int position[3];
	int speed[3];

	// command members
	int commandCounter;
	bool commandInProgress;

	// thread members
	pthread_t planeThread;
	pthread_attr_t attr;
	pthread_mutex_t mutex;

	// execution time members
	time_t startTime;
	time_t finishTime;


	// shm members
	int shm_fd;
	void *ptr;
	std::string planeString;
	std::string fileName;
};

#endif /* PLANE_H_ */
