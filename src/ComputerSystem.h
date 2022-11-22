/*
 * ComputerSystem.h
 *
 *  Created on: Oct. 24, 2022
 *      Author: mihai
 */

#ifndef COMPUTERSYSTEM_H_
#define COMPUTERSYSTEM_H_

#include <vector>
#include <list>
#include <fstream>
#include <time.h>
#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <sys/siginfo.h>
#include <sys/neutrino.h>

#include "Plane.h"
#include "Timer.h"
#include "Display.h"

class Plane;

class ComputerSystem{
public:
	// construcor
	ComputerSystem(){ initialize(); start(); }

	// destructor
	~ComputerSystem(){

	}

	int initialize(){

		// initialize
		// set threads in detached state
		int rc = pthread_attr_init(&attr);
		if (rc){
			printf("ERROR, RC from pthread_attr_init() is %d \n", rc);
		}

		rc = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
		if (rc){
			printf("ERROR; RC from pthread_attr_setdetachstate() is %d \n", rc);
		}

		// shared memory import from the display to present the data
		shm_displayData = shm_open("display", O_RDWR, 0666);
		if(shm_displayData == -1) {
			perror("in shm_open() Display");
			exit(1);
		}

		//pointer for the display data
		ptr_positionData = mmap(0, SIZE_DISPLAY, PROT_READ | PROT_WRITE, MAP_SHARED, shm_displayData, 0);
		if(ptr_positionData == MAP_FAILED) {
			perror("in map() Display");
			exit(1);
		}

		// shared memory from ssr for the airspace
		shm_airspace = shm_open("airspace", O_RDWR, 0666);
		if (shm_airspace == -1) {
			perror("in shm_open() PSR");
			exit(1);
		}
		// maybe separate reading file and initializing planes

		return 0;
	}

	int start(){
		if (pthread_create(&computerSystemThread, &attr, &ComputerSystem::startComputerSystem, (void *)this) != EOK) {
			computerSystemThread = NULL;
		}
	}

	int stop(){
		pthread_join(computerSystemThread, NULL);
		return 0;
	}

	static void *startComputerSystem(void *context) { ((ComputerSystem *)context)->calculateTrajectories(); }

	int calculateTrajectories(){

		return 0;
	}

private:
	std::list<Plane*> planes;
	std::list<Plane*>::iterator pit1, pit2;

	std::list<Plane*> airspace;
	std::list<Plane*>::iterator ait1, ait2;

	std::vector<pthread_t> planeThreads;

	// thread members
	pthread_t computerSystemThread;
	pthread_attr_t attr;

	// timing members
	time_t at;
	time_t et;

	// display list
	int shm_displayData;
	void *ptr_positionData;

	// waiting planes list
	int shm_waitingPlanes;
	void *ptr_waitingPlanes;
	std::vector<std::string> fileNames;

	// access waiting planes
	std::vector<void *> planePtrs;

	// airspace list
	int shm_airspace;
	void *ptr_airspace;
	std::vector<std::string> airspaceFileNames;

	friend class Plane;

};

#endif /* COMPUTERSYSTEM_H_ */
