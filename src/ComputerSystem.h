/*
 * ComputerSystem.h
 *
 *  Created on: Oct. 24, 2022
 *      Author: mihai
 */

#ifndef COMPUTERSYSTEM_H_
#define COMPUTERSYSTEM_H_

#include <cstdlib>
#include <errno.h>
#include <fstream>
#include <list>
#include <pthread.h>
#include <stdio.h>
#include <sys/neutrino.h>
#include <sys/siginfo.h>
#include <time.h>
#include <vector>

#include "Display.h"
#include "Limits.h"
#include "Plane.h"
#include "SSR.h"
#include "Timer.h"

class Plane; // forward declaration

// prediction container
struct trajectoryPrediction {
	int id;

	std::vector<int> posX;
	std::vector<int> posY;
	std::vector<int> posZ;

	int t;

	bool keep; // keep for next iteration
};

struct aircraft {
	int id;
	int t_arrival;
	int pos[3];
	int vel[3];
	bool keep; // keep for next iteration
	bool moreInfo;
	int commandCounter;
};

class ComputerSystem {
public:
	// construcor
	ComputerSystem(int numberOfPlanes);

	// destructor
	~ComputerSystem();

	// start computer system
	void start();

	// join computer system thread
	int stop();

	// entry point for execution function
	static void *startComputerSystem(void *context);

private:
	// ================= private member functions =================
	int initialize();

	// execution function
	void *calculateTrajectories();

	// ================= read airspace shm =================
	bool readAirspace();

	// ================= prune predictions =================
	void cleanPredictions();

	// ================= compute airspace violations =================
	void computeViolations(std::ofstream *out);

	// ================= send airspace info to display =================
	void writeToDisplay();

	// ================= update period =================
	void updatePeriod(int chid);

	// ================= members =================

	// timer object
	Timer *timer;

	// number of planes left
	int numPlanes;

	// current period
	int currPeriod;

	// thread members
	pthread_t computerSystemThread;
	pthread_attr_t attr;
	pthread_mutex_t mutex;

	// execution time members
	time_t startTime;
	time_t finishTime;

	// shm members
	// airspace shm
	int shm_airspace;
	void *airspacePtr;
	std::vector<aircraft *> flyingPlanesInfo;
	std::vector<trajectoryPrediction *> trajectoryPredictions;

	// period shm
	int shm_period;
	void *periodPtr;

	// display shm
	int shm_display;
	void *displayPtr;

	// comm list (same as waiting list psr)
	std::vector<void *> commPtrs;
	std::vector<std::string> commNames;
};

#endif /* COMPUTERSYSTEM_H_ */
