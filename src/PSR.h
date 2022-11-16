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

#include "Plane.h"
#include "Timer.h"

#define SIZE 4096
#define PSR_PERIOD 5000000

// forward declaration
class Plane;

class PSR {
public:
	// constructor
	PSR(int numberOfPlanes) { initialize(numberOfPlanes); }

	// destructor
	~PSR() {
		shm_unlink("plane_1");
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
		shm_waitingPlanes = shm_open("waiting_planes", O_RDONLY, 0666);
		if (shm_waitingPlanes == -1) {
			perror("in shm_open() PSR");
			exit(1);
		}

		ptr_waitingPlanes = mmap(0, 4096, PROT_READ, MAP_SHARED, shm_waitingPlanes, 0);
		if (ptr_waitingPlanes == MAP_FAILED) {
			perror("in map() PSR");
			exit(1);
		}

		char allPlanes[36];

		//    char filename[8];
		//    char *arrayOfFilenames[4];


		for (int i = 0; i < numberOfPlanes * 9; i++) {
			pthread_mutex_lock(&mutex);

			char readChar = *((char *)ptr_waitingPlanes + i);
			allPlanes[i] = readChar;

			//			int j = 0;
			//			bool endOfLine = false;
			//
			//			while(!endOfLine){
			//
			//				char readChar =
			//*((char*)ptr_waitingPlanes + j);
			//
			////				if(readChar == " ")){
			////					endOfLine = true;
			////				}
			////				else{
			////					filename[j] = readChar;
			////				}
			//				j++;
			//			}
			//
			//			arrayOfFilenames[i] = strdup(filename);

			pthread_mutex_unlock(&mutex);
		}

		printf("PSR read allPlanes: %s\n", allPlanes);

		for(int i = 0; i < numberOfPlanes; i++){
			std::string filename = "";

			for(int j = 0; j < 7; j++){
				filename += allPlanes[j + i*8];
			}
			std::cout << filename << "\n";
			fileNames.push_back(filename);

			//    	filename = allPlanes[j] + allPlanes[j+1] + allPlanes[j+2] + allPlanes[j+3] + allPlanes[j+4] + allPlanes[j+5] + allPlanes[j+6] + allPlanes[j+7];
			//    	filename[0] = allPlanes[j];
			//    	filename[1] = allPlanes[j + 1];
			//    	filename[2] = allPlanes[j + 2];
			//    	filename[3] = allPlanes[j + 3];
			//    	filename[4] = allPlanes[j + 4];
			//    	filename[5] = allPlanes[j + 5];
			//    	filename[6] = allPlanes[j + 6];
			//    	filename[7] = allPlanes[j + 7];
			//    	fileNames.push_back(filename);
			//    	j += 8;

			int shm_plane = shm_open(filename.c_str(), O_RDONLY, 0666);
			if(shm_plane == -1){
				perror("in shm_open() plane");
				exit(1);
			}

			void* ptr = mmap(0, SIZE, PROT_READ, MAP_SHARED, shm_plane, 0);
			if (ptr == MAP_FAILED) {
				perror("in map() PSR");
				exit(1);
			}
			planePtrs.push_back(ptr);
		}


		for(int i = 0; i < numberOfPlanes; i++){
			printf("Plane %i: %s\n", i, fileNames.at(i));
		}


		return 0;
	}

	int start() {
		std::cout << "PSR start called\n";
		if (pthread_create(&PSRthread, &attr, &PSR::startPSR, (void *)this) !=
				EOK) {
			PSRthread = NULL;
		}
	}

	int stop() {
		pthread_join(PSRthread, NULL);
		return 0;
	}

	static void *startPSR(void *context) { ((PSR *)context)->operatePSR(); }

	void *operatePSR(void) {
		//		std::cout << "start exec\n";
		// update position every second from position and speed every second
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
				pthread_mutex_lock(&mutex);

				printf("PSR read: \n");

				int i = 0;
				for(void* ptr : planePtrs){
					printf("Plane %i: %s\n", i++, ptr);
				}
				printf("\n");

				pthread_mutex_unlock(&mutex);
			}
			rcvid = MsgReceive(chid, &msg, sizeof(msg), NULL);
			//			std::cout << "executing end\n";
		}

		ChannelDestroy(chid);

		return 0;
	}

	int discover() {
		// scan all planes and find ready (Tarrival <= Tcurrent)
		// move ready planes to ready list
	}

private:
	// list of all planes
	// list of ready planes

	std::vector<void *> planePtrs;
	std::vector<std::string> fileNames;

	pthread_t PSRthread;
	pthread_attr_t attr;
	pthread_mutex_t mutex;

	time_t at;
	time_t et;

	int shm_waitingPlanes;
	void *ptr_waitingPlanes;

	friend class Plane;
};

#endif /* PSR_H_ */
