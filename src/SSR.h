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

	// constructor
	SSR(int numberOfPlanes){
		currPeriod = SSR_PERIOD;
		// initialize thread and shm members
		initialize(numberOfPlanes);
	}

	// destructor
	~SSR(){
		shm_unlink("flying_planes");
		shm_unlink("airspace");
		shm_unlink("period");
		pthread_mutex_destroy(&mutex);
		delete timer;
	}

	int start() {
		//		std::cout << "ssr start called\n";
		if (pthread_create(&SSRthread, &attr, &SSR::startSSR, (void *)this) != EOK) {
			SSRthread = NULL;
		}
	}

	int stop() {
		//		std::cout << "ssr stop called\n";
		pthread_join(SSRthread, NULL);
		return 0;
	}

	// entry point for execution thread
	static void *startSSR(void *context) { ((SSR *)context)->operateSSR(); }

private:

	// member functions
	// initialize thread and shm members
	int initialize(int numberOfPlanes) {
		numPlanes = numberOfPlanes;
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
		shm_flyingPlanes = shm_open("flying_planes", O_RDWR, 0666);
		if (shm_flyingPlanes == -1) {
			perror("in shm_open() SSR: flying planes");
			exit(1);
		}

		flyingPlanesPtr = mmap(0, SIZE_SHM_SSR, PROT_READ | PROT_WRITE, MAP_SHARED, shm_flyingPlanes, 0);
		if (flyingPlanesPtr == MAP_FAILED) {
			perror("in map() SSR: flying planes");
			exit(1);
		}

		// open airspace shm
		shm_airspace = shm_open("airspace", O_RDWR, 0666);
		if (shm_airspace == -1) {
			perror("in shm_open() SSR: airspace");
			exit(1);
		}

		// map airspace shm
		airspacePtr = mmap(0, SIZE_SHM_AIRSPACE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_airspace, 0);
		if (airspacePtr == MAP_FAILED) {
			perror("in map() SSR: airspace");
			exit(1);
		}

		// open period shm
		shm_period = shm_open("period", O_RDONLY, 0666);
		if (shm_period == -1) {
			perror("in shm_open() SSR: period");
			exit(1);
		}

		// map airspace shm
		periodPtr = mmap(0, SIZE_SHM_PERIOD, PROT_READ, MAP_SHARED, shm_period, 0);
		if (periodPtr == MAP_FAILED) {
			perror("in map() SSR: period");
			exit(1);
		}

		return 0;
	}

	// execution thread
	void *operateSSR(void) {
		//		std::cout << "start exec\n";
		// create channel to communicate with timer
		int chid = ChannelCreate(0);
		if (chid == -1) {
			std::cout << "couldn't create channel!\n";
		}

		Timer *newTimer = new Timer(chid);
		timer = newTimer;
		timer->setTimer(SSR_PERIOD, SSR_PERIOD);

		int rcvid;
		Message msg;

		while (1) {
			if (rcvid == 0) {
				// lock mutex
				pthread_mutex_lock(&mutex);

				// check period shm to see if must update period
				updatePeriod(chid);

				// read flying planes shm
				bool writeFlying = readFlyingPlanes();

				// get info from planes, write to airspace
				bool writeAirspace = getPlaneInfo();

				// if new/terminated plane found, write new flying planes
				if(writeFlying || writeAirspace){
					writeFlyingPlanes();
				}

				// unlock mutex
				pthread_mutex_unlock(&mutex);

				// check for termination
				if(numPlanes <= 0){
					std::cout << "ssr done\n";
					ChannelDestroy(chid);

					return 0;
				}

			}
			rcvid = MsgReceive(chid, &msg, sizeof(msg), NULL);
		}

		ChannelDestroy(chid);

		return 0;
	}

	// update ssr period based on period shm
	void updatePeriod(int chid){
//		printf("ssr read period: %s\n", periodPtr);
		int newPeriod = atoi((char *)periodPtr);
		if(newPeriod != currPeriod){
//			std::cout << "ssr period changed to " << newPeriod << "\n";
			currPeriod = newPeriod;
			timer->setTimer(currPeriod, currPeriod);
		}
	}

	// ================= read flying planes shm =================
	bool readFlyingPlanes(){
		bool write = false;
		std::string FD_buffer = "";

		for(int i = 0; i < SIZE_SHM_SSR; i++){
			//					std::cout << "buffer: " << FD_buffer << "\n";
			char readChar = *((char *)flyingPlanesPtr + i);

			// shm termination character found
			if(readChar == ';'){
				if(i == 0){
					//							std::cout << "ssr no flying planes\n";
					break;	// no flying planes
				}

				//						std::cout << "ssr last plane: " << FD_buffer << "\n";

				// check if flying plane was already in list
				bool inFile = false;

				for(std::string filename : flyingFileNames){
					if(filename == FD_buffer){
						//								std::cout << "ssr: " << FD_buffer << " already in list\n";
						inFile = true;
						break;
					}
				}

				// if not, add to list
				if(!inFile){
					write = true;	// found plane that is not already flying

					//							std::cout << "added: " << FD_buffer << "\n";
					flyingFileNames.push_back(FD_buffer);

					// open shm for current plane
					int shm_plane = shm_open(FD_buffer.c_str(), O_RDONLY, 0666);
					if(shm_plane == -1){
						perror("in shm_open() SSR plane");

						exit(1);
					}

					// map memory for current plane
					void* ptr = mmap(0, SIZE_SHM_PLANES, PROT_READ, MAP_SHARED, shm_plane, 0);
					if (ptr == MAP_FAILED) {
						perror("in map() SSR");
						exit(1);
					}

					planePtrs.push_back(ptr);

					// end of flying planes
					break;
				}
				break;
			}
			// found a plane, open shm
			else if(readChar == ','){
				//						std::cout << "ssr found: " << FD_buffer << "\n";

				// check if plane was already in list
				bool inFile = false;

				for(std::string filename : flyingFileNames){
					if(filename == FD_buffer){
						//								std::cout << FD_buffer << " already in list\n";
						inFile = true;
					}
				}

				// if not, add to list
				if(!inFile){
					write = true;	// found new flying plane that wasn't already flying

					//							std::cout << "added: " << FD_buffer << "\n";
					flyingFileNames.push_back(FD_buffer);

					// open shm for current plane
					int shm_plane = shm_open(FD_buffer.c_str(), O_RDONLY, 0666);
					if(shm_plane == -1){
						perror("in shm_open() SSR plane");

						exit(1);
					}

					// map memory for current plane
					void* ptr = mmap(0, SIZE_SHM_PLANES, PROT_READ, MAP_SHARED, shm_plane, 0);
					if (ptr == MAP_FAILED) {
						perror("in map() SSR");
						exit(1);
					}

					planePtrs.push_back(ptr);

					// reset buffer for next plane
					FD_buffer = "";
					continue;
				}

				FD_buffer = "";
				continue;
			}

			// just add the char to the buffer
			FD_buffer += readChar;
		}

		return write;
	}

	// ================= get info from planes =================
	bool getPlaneInfo(){
		bool write = false;
		// buffer for all plane info
		std::string airspaceBuffer = "";


		int i = 0;
		auto it = planePtrs.begin();
		while(it != planePtrs.end()){
			std::string readBuffer = "";
			char readChar = *((char *)*it);

			// remove plane (terminated)
			if(readChar == 't'){
				write = true;	// found plane to remove

				//						std::cout << "ssr found terminated\n";

				// remove current fd from flying planes fd vector
				flyingFileNames.erase(flyingFileNames.begin() + i);

				// remove current plane from ptr vector
				it = planePtrs.erase(it);

				// reduce number of planes
				numPlanes--;
				std::cout << "ssr number of planes: " << numPlanes << "\n";
			}
			// plane not terminated, read all data and add to buffer for airspace
			else{
				int j = 0;
				//						std::cout << "reading current plane data\n";
				for(; j < SIZE_SHM_PLANES; j++){
					char readChar = *((char *)*it + j);

					// end of plane shm
					if(readChar == ';'){
						break;
					}

					// if not first plane read, append "/" to front (plane separator)
					if(i != 0 && j == 0){
						readBuffer += "/";	// plane separator
					}
					// add current character to buffer
					readBuffer += readChar;
				}
				// no planes
				if(j == 0){
					break;
				}
				i++;	// only increment if no plane to terminate and plane info added
				++it;
			}

			// add current buffer to buffer for airspace shm
			airspaceBuffer += readBuffer;
		}

		// termination character for airspace write
		airspaceBuffer += ";";

		//								std::cout << "ssr current airspace: " << airspaceBuffer << "\n";
		sprintf((char *)airspacePtr, "%s", airspaceBuffer.c_str());
		//				printf("ssr airspace after write: %s\n", airspacePtr);


		return write;
	}

	// ================= write new flying planes shm =================
	void writeFlyingPlanes(){
		// new flying planes buffer
		std::string currentAirspace = "";

		int j = 0;
		// add planes to transfer to buffer
		for(std::string filename : flyingFileNames){
			if(j == 0){
				currentAirspace += filename;
				j++;
			}
			else{
				currentAirspace += ",";
				currentAirspace += filename;
			}
		}
		// termination character for flying planes list
		currentAirspace += ";";


		//					std::cout << "ssr current flying planes list: " << currentAirspace << "\n";

		// write new flying planes list
		sprintf((char *)flyingPlanesPtr, "%s", currentAirspace.c_str());
		//					printf("ssr flying planes after write: %s\n", flyingPlanesPtr);
	}


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
