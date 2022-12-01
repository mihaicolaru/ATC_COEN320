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

#include "Plane.h"
#include "Timer.h"
#include "Limits.h"

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

	// timing members
	time_t at;
	time_t et;

	// list of planes in airspace
	std::vector<std::string> fileNames;
	std::vector<void *> planePtrs;

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

	friend class Plane;
	friend class PSR;

	// number of planes left
	int numPlanes;
};


#endif /* SSR_H_ */
