/*
 * SSR.h
 *
 *  Created on: Oct. 24, 2022
 *      Author: mihai
 */

#ifndef SSR_H_
#define SSR_H_

#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#include "Limits.h"
#include "Plane.h"
#include "Timer.h"

class Plane;

class SSR {
public:
	SSR(int numberOfPlanes);
	~SSR();
	int start();
	int stop();
	static void *startSSR(void *context);

private:
	int initialize(int numberOfPlanes);
	void *operateSSR(void);
	void updatePeriod(int chid);
	bool readFlyingPlanes();
	bool getPlaneInfo();
	void writeFlyingPlanes();

	// member parameters
	// timer object
	Timer *timer;

	// current period
	int currPeriod;

	// thread members
	pthread_t SSRthread;
	pthread_attr_t attr;
	pthread_mutex_t mutex;

	// execution time members
	time_t startTime;
	time_t finishTime;


	// list of planes in airspace
	std::vector<std::string> fileNames;
	std::vector<void *> planePtrs;

	// list of waiting planes
	std::vector<std::string> waitingFileNames;
	std::vector<void *> waitingPtrs;

	// flying planes list
	int shm_flyingPlanes;
	void *flyingPlanesPtr;
	std::vector<std::string> flyingFileNames;

	// airspace shm
	int shm_airspace;
	void *airspacePtr;

	// period shm
	int shm_period;
	void *periodPtr;

	// number of planes left
	int numPlanes;

};

#endif /* SSR_H_ */
