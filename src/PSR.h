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

#define SIZE_SHM_PLANES 4096
#define SIZE_SHM_PSR 4096
#define PSR_PERIOD 3000000

// forward declaration
class Plane;
class SSR;

class PSR {
public:
	// constructor
	PSR(int numberOfPlanes) {

		numWaitingPlanes = numberOfPlanes;

		initialize(numberOfPlanes); }

	// destructor
	~PSR() {

		//		for(std::string filename : waitingFileNames){
		//			shm_unlink(filename.c_str());
		//		}
		pthread_mutex_destroy(&mutex);
	}

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
		ptr_waitingPlanes = mmap(0, SIZE_SHM_PSR, PROT_READ | PROT_WRITE, MAP_SHARED, shm_waitingPlanes, 0);
		if (ptr_waitingPlanes == MAP_FAILED) {
			perror("in map() PSR");
			exit(1);
		}

		//		printf("waiting planes: %s\n", ptr_waitingPlanes);

		std::string FD_buffer = "";

		for(int i = 0; i < SIZE_SHM_PSR; i++){
			char readChar = *((char *)ptr_waitingPlanes + i);

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
			perror("in shm_open() ATC: airspace");
			exit(1);
		}

		// map shm
		ptr_flyingPlanes = mmap(0, SIZE_SHM_SSR, PROT_READ | PROT_WRITE, MAP_SHARED, shm_flyingPlanes, 0);
		if (ptr_flyingPlanes == MAP_FAILED) {
			printf("map failed airspace\n");
			return -1;
		}

		return 0;
	}

	void start() {
		//		std::cout << "PSR start called\n";
		time(&at);
		if (pthread_create(&PSRthread, &attr, &PSR::startPSR, (void *)this) != EOK) {
			PSRthread = NULL;
		}
	}

	int stop() {
		//		std::cout << "psr stop called\n";
		pthread_join(PSRthread, NULL);
		return 0;
	}

	// entry point for execution thread
	static void *startPSR(void *context) { ((PSR *)context)->operatePSR(); }

	// execution thread
	void *operatePSR(void) {
		time(&at);
		//		std::cout << "start exec\n";
		// create channel to communicate with timer
		int chid = ChannelCreate(0);
		if (chid == -1) {
			std::cout << "couldn't create channel!\n";
		}

		Timer timer(chid);
		timer.setTimer(PSR_PERIOD, PSR_PERIOD);

		int rcvid;
		Message msg;

		while (1) {
			if (rcvid == 0) {

				bool move = false;

				int i = 0;
				pthread_mutex_lock(&mutex);

				// ================= read waiting planes shm =================
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
				// ================= end read waiting planes =================


				//				std::cout << "psr waiting planes buffer:\n";
				//				for(std::string name : waitingFileNames){
				//					std::cout << name << "\n";
				//				}
				//
				//				std::cout << "psr flying planes buffer:\n";
				//				for(std::string name : flyingFileNames){
				//					std::cout << name << "\n";
				//				}

				//				pthread_mutex_unlock(&mutex);




				// ================= write to flying planes shm =================


				// if planes to be moved are found, write to flying planes shm
				if(move){
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

					// ================= read current to flying planes shm =================
					while(i < SIZE_SHM_SSR){
						char readChar = *((char *)ptr_flyingPlanes + i);

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
					// ================= end read current flying planes =================

					//					std::cout << "psr flying planes before adding new: " << currentAirspace << "\n";

					// ================= add planes to transfer buffer =================
					// add planes to transfer
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
					// ================= end add planes to buffer =================

					//					std::cout << "psr flying planes after adding new: " << currentAirspace << "\n";


					// ================= write to flying planes shm =================
					//					pthread_mutex_lock(&mutex);

					// write new flying planes list to shm
					sprintf((char *)ptr_flyingPlanes , "%s", currentAirspace.c_str());
					//					printf("psr flying planes after write: %s\n", ptr_flyingPlanes);
					// ================= end write =================
					//					pthread_mutex_unlock(&mutex);
				}

				// ================= end write to flying planes =================



				// clear buffer for next flying planes list
				flyingFileNames.clear();

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

	// add function to change timer settings depending on n (congestion control)

private:

	int numWaitingPlanes;

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
	void *ptr_waitingPlanes;
	std::vector<std::string> waitingFileNames;

	// access waiting planes
	std::vector<void *> planePtrs;

	// airspace list
	int shm_flyingPlanes;
	void *ptr_flyingPlanes;
	std::vector<std::string> flyingFileNames;

	friend class Plane;
};

#endif /* PSR_H_ */
