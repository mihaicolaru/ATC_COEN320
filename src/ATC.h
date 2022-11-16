/*
 * ATC.h
 *
 *  Created on: Nov. 12, 2022
 *      Author: Mihai
 */

#ifndef ATC_H_
#define ATC_H_

#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <list>
#include <pthread.h>
#include <stdio.h>
#include <string>
#include <sys/mman.h>
#include <sys/neutrino.h>
#include <sys/siginfo.h>
#include <time.h>
#include <vector>

#include "Plane.h"
#include "Timer.h"

#define PSR_PERIOD 5000000
#define SSR_PERIOD 5000000

#define SPACE_X_MIN 0
#define SPACE_X_MAX 100000
#define SPACE_Y_MIN 0
#define SPACE_Y_MAX 100000
#define SPACE_Z_MIN 0
#define SPACE_Z_MAX 25000
#define SPACE_ELEVATION 15000


class ATC {
public:
	ATC() {
		initialize();

		start();
	}

	~ATC() {
		// release all shared memory pointers
	}

	int initialize() {
		// read input from file
		readInput();
		// initialize shm for waiting planes (contains all planes)
		// initialize shared memory space for airspace (contains no planes)
		// create PSR object with number of planes

		return 0; // set to error code if any
	}

	int start() {



		// start timer for arrivalTime comparison
		// assign shared memory object to thread create thread for PSR create thread
		// for SSR create thread for ComputerSystem create thread for Display create
		// thread for Console create thread for Comm

		// ============ execution time ============

		// call plane-> stop for all plane objects
		// call psr stop
		return 0; // set to error code if any
	}


protected:

	int readInput() {
		// open input.txt
		std::string filename = "./input.txt";
		std::ifstream input_file_stream;

		input_file_stream.open(filename);

		if (!input_file_stream) {
			std::cout << "Can't find file input.txt" << std::endl;
			return 1;
		}

		int ID, arrivalTime, arrivalCordX, arrivalCordY, arrivalCordZ,
		arrivalSpeedX, arrivalSpeedY, arrivalSpeedZ;

		std::string separator = " ";

		// parse input.txt to create plane objects
		while (input_file_stream >> ID >> arrivalTime >> arrivalCordX >>
				arrivalCordY >> arrivalCordZ >> arrivalSpeedX >> arrivalSpeedY >>
				arrivalSpeedZ) {

			std::cout << ID << separator << arrivalTime << separator << arrivalCordX
					<< separator << arrivalCordY << separator << arrivalCordZ
					<< separator << arrivalSpeedX << separator << arrivalSpeedY
					<< separator << arrivalSpeedZ << std::endl;


			// create plane objects and add pointer to each plane to a vector

		}
	}


	// ============================ MEMBERS ============================

	// plane queues
	std::vector<Plane *> Planes; // vector of pointers to plane objects
	std::vector<Plane *> waitingPlanes; // planes not in airspace
	std::vector<Plane *> airspace;      // planes in airspace

	// timers
	time_t startTime;
	time_t endTime;

	// shm members
	// waiting planes
	int shm_waitingPlanes;
	void* waitingPtr;

	// airspace
	int shm_airspace;
	void* airspacePtr;
};

#endif /* ATC_H_ */
