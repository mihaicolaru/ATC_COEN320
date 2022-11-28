/*
 * PSR.h
 *
 *  Created on: Oct. 24, 2022
 *      Author: mihai, minhtrannhat
 */

#ifndef PSR_H_
#define PSR_H_

#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#include "SSR.h"
#include "Plane.h"
#include "Timer.h"
#include "Limits.h"


// forward declaration
class Plane;
class SSR;

class PSR {
public:
	// constructor
	PSR(int numberOfPlanes) {
		currPeriod = PSR_PERIOD;
		numWaitingPlanes = numberOfPlanes;

		// initialize thread and shm members
		initialize(numberOfPlanes);
	}

	// destructor
	~PSR() {
		shm_unlink("waiting_planes");
		shm_unlink("flying_planes");
		shm_unlink("period");
		pthread_mutex_destroy(&mutex);
		delete timer;
	}

	// start execution function
	void start() {
		//		std::cout << "psr start called\n";
		time(&at);
		if (pthread_create(&PSRthread, &attr, &PSR::startPSR, (void *)this) != EOK) {
			PSRthread = NULL;
		}
	}

	// join execution thread
	int stop() {
		//		std::cout << "psr stop called\n";
		pthread_join(PSRthread, NULL);
		return 0;
	}

	// entry point for execution thread
	static void *startPSR(void *context) { ((PSR *)context)->operatePSR(); }


private:

	// member functions
	int initialize(int numberOfPlanes) {
		// set thread in detached state
		int rc = pthread_attr_init(&attr);
		if (rc) {
			printf("ERROR, RC from pthread_attr_init() is %d \n", rc);
		}

		rc = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
		if (rc) {
			printf("ERROR; RC from pthread_attr_setdetachstate() is %d \n", rc);
		}

		// open list of waiting planes shm
		shm_waitingPlanes = shm_open("waiting_planes", O_RDWR, 0666);
		if (shm_waitingPlanes == -1) {
			perror("in shm_open() PSR");
			exit(1);
		}

		// map waiting planes shm
		waitingPlanesPtr = mmap(0, SIZE_SHM_PSR, PROT_READ | PROT_WRITE, MAP_SHARED, shm_waitingPlanes, 0);
		if (waitingPlanesPtr == MAP_FAILED) {
			perror("in map() PSR");
			exit(1);
		}

		//		printf("waiting planes: %s\n", ptr_waitingPlanes);

		std::string FD_buffer = "";

		for(int i = 0; i < SIZE_SHM_PSR; i++){
			char readChar = *((char *)waitingPlanesPtr + i);

			if(readChar == ','){
				//				std::cout << "PSR initialize() found a planeFD: " << FD_buffer << "\n";

				waitingFileNames.push_back(FD_buffer);

				// open shm for current plane
				int shm_plane = shm_open(FD_buffer.c_str(), O_RDONLY, 0666);
				if(shm_plane == -1){
					perror("in shm_open() PSR plane");

					exit(1);
				}

				// map memory for current plane
				void* ptr = mmap(0, SIZE_SHM_PLANES, PROT_READ, MAP_SHARED, shm_plane, 0);
				if (ptr == MAP_FAILED) {
					perror("in map() PSR");
					exit(1);
				}

				planePtrs.push_back(ptr);

				FD_buffer = "";
				continue;
			}
			else if(readChar == ';'){
				waitingFileNames.push_back(FD_buffer);

				// open shm for current plane
				int shm_plane = shm_open(FD_buffer.c_str(), O_RDONLY, 0666);
				if(shm_plane == -1){
					perror("in shm_open() PSR plane");

					exit(1);
				}

				// map memory for current plane
				void* ptr = mmap(0, SIZE_SHM_PLANES, PROT_READ, MAP_SHARED, shm_plane, 0);
				if (ptr == MAP_FAILED) {
					perror("in map() PSR");
					exit(1);
				}

				planePtrs.push_back(ptr);
				break;
			}

			FD_buffer += readChar;
		}

		// open ssr shm
		shm_flyingPlanes = shm_open("flying_planes", O_RDWR, 0666);
		if (shm_flyingPlanes == -1) {
			perror("in shm_open() PSR: airspace");
			exit(1);
		}

		// map shm
		flyingPlanesPtr = mmap(0, SIZE_SHM_SSR, PROT_READ | PROT_WRITE, MAP_SHARED, shm_flyingPlanes, 0);
		if (flyingPlanesPtr == MAP_FAILED) {
			printf("map failed airspace\n");
			return -1;
		}

		// open period shm
		shm_period = shm_open("period", O_RDONLY, 0666);
		if (shm_period == -1) {
			perror("in shm_open() PSR: period");
			exit(1);
		}

		// map airspace shm
		periodPtr = mmap(0, SIZE_SHM_PERIOD, PROT_READ, MAP_SHARED, shm_period, 0);
		if (periodPtr == MAP_FAILED) {
			perror("in map() PSR: period");
			exit(1);
		}

		return 0;
	}

	// ================= execution thread =================
	void *operatePSR(void) {
		time(&at);
		//		std::cout << "start exec\n";
		// create channel to communicate with timer
		int chid = ChannelCreate(0);
		if (chid == -1) {
			std::cout << "couldn't create channel!\n";
		}

		Timer *newTimer = new Timer(chid);
		timer = newTimer;
		timer->setTimer(PSR_PERIOD, PSR_PERIOD);

		int rcvid;
		Message msg;

		while (1) {
			if (rcvid == 0) {
				// lock mutex
				pthread_mutex_lock(&mutex);

				// check period shm to see if must update period
				updatePeriod(chid);

				// read waiting planes buffer
				bool move = readWaitingPlanes();

				// if planes to be moved are found, write to flying planes shm
				if(move){
					writeFlyingPlanes();
				}

				// clear buffer for next flying planes list
				flyingFileNames.clear();

				// unlock mutex
				pthread_mutex_unlock(&mutex);

				// check for PSR termination
				if(numWaitingPlanes <= 0){
					//					std::cout << "psr done\n";
					ChannelDestroy(chid);
					return 0;
				}
			}
			rcvid = MsgReceive(chid, &msg, sizeof(msg), NULL);
		}

		ChannelDestroy(chid);

		return 0;
	}

	// update psr period based on period shm
	void updatePeriod(int chid){
		printf("psr read period: %s\n", periodPtr);
		int newPeriod = atoi((char *)periodPtr);
		if(newPeriod != currPeriod){
			std::cout << "psr period changed to " << newPeriod << "\n";
			currPeriod = newPeriod;
			Timer *newTimer = new Timer(chid);
			Timer *oldTimer = timer;
			timer = newTimer;
			timer->setTimer(currPeriod, currPeriod);
			delete oldTimer;
		}
	}

	// ================= read waiting planes buffer =================
	bool readWaitingPlanes(){
		bool move = false;
		int i = 0;
		auto it = planePtrs.begin();
		while(it != planePtrs.end()){
			//					char readChar = *((char *)*it);

			// find first comma after the ID
			int j = 0;
			for(; j < 4; j++){
				if(*((char*)*it + j) == ','){
					break;
				}
			}

			// extract arrival time
			int curr_arrival_time = atoi((char *)(*it) + j + 1);
			//						std::cout << "current plane arrival time: " << curr_arrival_time << "\n";

			// compare with current time
			// if t_arrival < t_current
			time (&et);
			double t_current = difftime(et,at);
			//					std::cout << "current time: " << t_current << ", arrival time: " << curr_arrival_time << "\n";

			if(curr_arrival_time <= t_current){
				move = true;

				//						std::cout << "psr found " << waitingFileNames.at(i) << " to move\n";

				// add current fd to airspace fd vector
				flyingFileNames.push_back(waitingFileNames.at(i));

				// remove current fd from waiting planes fd vector
				waitingFileNames.erase(waitingFileNames.begin() + i);

				// remove current plane from ptr vector
				it = planePtrs.erase(it);

				numWaitingPlanes--;
				//						std::cout << "psr number of waiting planes: " << numWaitingPlanes << "\n";
			}
			else{
				i++;	// only increment if no plane to transfer
				++it;
			}
		}
		//				std::cout << "psr waiting planes buffer:\n";
		//				for(std::string name : waitingFileNames){
		//					std::cout << name << "\n";
		//				}
		//
		//				std::cout << "psr flying planes buffer:\n";
		//				for(std::string name : flyingFileNames){
		//					std::cout << name << "\n";
		//				}

		//		std::cout << "psr end read waiting planes\n";
		return move;
	}

	// ================= write to flying planes shm =================
	void writeFlyingPlanes(){
		//					pthread_mutex_lock(&mutex);
		//					printf("airspace before move: %s\n", ptr_airspace);
		//					std::cout << "planes to move:\n";
		//					for(std::string name : airspaceFileNames){
		//						std::cout << name << "\n";
		//					}
		// read current flying planes shm
		std::string currentAirspace = "";
		std::string currentPlane = "";

		int i = 0;

		// read current to flying planes shm
		while(i < SIZE_SHM_SSR){
			char readChar = *((char *)flyingPlanesPtr + i);

			if(readChar == ';'){
				// termination character found
				if(i == 0){
					// no planes
					//								std::cout << "PSR no current flying planes in shm\n";
					break;
				}

				//							printf("psr last plane: %s\n", currentPlane);
				// check if plane already in list
				bool inList = true;

				//							std::cout << "checking if " << currentPlane << " already in flying list\n";
				for(std::string name : flyingFileNames){
					if(currentPlane == name){
						inList = false;
						//									std::cout << currentPlane << " already in list\n";
						break;
					}
				}

				if(inList){
					//								std::cout << "psr added " << currentPlane << "\n";
					currentAirspace += currentPlane;
					currentAirspace += ',';
				}

				break;
			}
			else if(readChar == ','){
				// check if plane already in list

				//							printf("psr found plane: %s\n", currentPlane);

				bool inList = true;
				//							std::cout << "checking if " << currentPlane << " already in flying list\n";
				for(std::string name : flyingFileNames){
					if(currentPlane == name){
						inList = false;
						//									std::cout << currentPlane << " already in list\n";
						break;
					}
				}

				if(inList){
					//								std::cout << "psr added " << currentPlane << "\n";
					currentAirspace += currentPlane;
					currentAirspace += ',';
				}

				currentPlane = "";
				i++;
				continue;
			}

			currentPlane += readChar;
			i++;
		}
		// end read current flying planes

		//					std::cout << "psr flying planes before adding new: " << currentAirspace << "\n";

		// add planes to transfer buffer
		i = 0;
		for(std::string filename : flyingFileNames){
			if(i == 0){
				currentAirspace += filename;
				i++;
			}
			else{
				currentAirspace += ",";
				currentAirspace += filename;
			}
		}
		currentAirspace += ";";
		//					std::cout << "psr flying planes after adding new: " << currentAirspace << "\n";

		// write new flying planes list to shm
		sprintf((char *)flyingPlanesPtr , "%s", currentAirspace.c_str());
		//					printf("psr flying planes after write: %s\n", ptr_flyingPlanes);
	}

	// member parameters
	// timer object
	Timer *timer;

	// number of waiting planes left
	int numWaitingPlanes;

	// current period
	int currPeriod;

	// thread members
	pthread_t PSRthread;
	pthread_attr_t attr;
	pthread_mutex_t mutex;

	// timing members
	time_t at;
	time_t et;

	// shm members
	// waiting planes list
	int shm_waitingPlanes;
	void *waitingPlanesPtr;
	std::vector<std::string> waitingFileNames;

	// access waiting planes
	std::vector<void *> planePtrs;

	// airspace list
	int shm_flyingPlanes;
	void *flyingPlanesPtr;
	std::vector<std::string> flyingFileNames;

	// period shm
	int shm_period;
	void *periodPtr;

	friend class Plane;
};

#endif /* PSR_H_ */
