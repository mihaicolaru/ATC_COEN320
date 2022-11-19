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

#define SIZE_SHM_PLANES 4096
#define SIZE_SHM_PSR 4096
#define PSR_PERIOD 5000000

class Plane;

class SSR {
public:

	// constructor
	SSR(int numberOfPlanes){ initialize(numberOfPlanes); }

	// destructor
	~SSR(){
		for(std::string filename : fileNames){
					shm_unlink(filename.c_str());
				}
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
		shm_airspace = shm_open("waiting_planes", O_RDWR, 0666);
		if (shm_airspace == -1) {
			perror("in shm_open() PSR");
			exit(1);
		}

		ptr_airspace = mmap(0, SIZE_SHM_PSR, PROT_READ | PROT_WRITE, MAP_SHARED, shm_airspace, 0);
		if (ptr_airspace == MAP_FAILED) {
			perror("in map() PSR");
			exit(1);
		}

		// buffer to read waitingPlanes shm
		char allPlanes[36];

		// read waiting planes shm
		for (int i = 0; i < numberOfPlanes * 9; i++) {
			char readChar = *((char *)ptr_airspace + i);
			allPlanes[i] = readChar;
		}

		//		printf("PSR read allPlanes: %s\n", allPlanes);

		// link shm objects for each plane
		for(int i = 0; i < numberOfPlanes; i++){
			std::string filename = "";

			// read 1 plane filename
			for(int j = 0; j < 7; j++){
				filename += allPlanes[j + i*8];
			}
						std::cout << filename << "\n";
			// store filenames to vector
			fileNames.push_back(filename);

			// open shm for current plane
			int shm_plane = shm_open(filename.c_str(), O_RDONLY, 0666);
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

			// store shm pointer to vector
			planePtrs.push_back(ptr);
		}
		return 0;
	}

	int start() {
			//		std::cout << "SSR start called\n";
			if (pthread_create(&SSRthread, &attr, &SSR::startSSR, (void *)this) != EOK) {
				SSRthread = NULL;
			}
		}

		int stop() {
			pthread_join(SSRthread, NULL);
			return 0;
		}

		// entry point for execution thread
		static void *startSSR(void *context) { ((SSR *)context)->operateSSR(); }

		// execution thread
		void *operateSSR(void) {
			//		std::cout << "start exec\n";
			// create channel to communicate with timer
			int chid = ChannelCreate(0);
			if (chid == -1) {
				std::cout << "couldn't create channel!\n";
			}

			Timer timer(chid);
			timer.setTimer(SSR_PERIOD, SSR_PERIOD);

			int rcvid;
			Message msg;

			while (1) {
				if (rcvid == 0) {
					pthread_mutex_lock(&mutex);

					printf("PSR read: \n");

					int i = 0;
					for(void* ptr : planePtrs){
						printf("Plane %i: %s\n", i++, ptr);
						// parse plane shm to extract t_arrival
						// compare with current time
						// if t_arrival < t_current OR "terminated"

						// remove current plane from waitingplanes fildes vector
						// add current plane to airspace fildes vector

						// remove current plane from ptr vector
					}
					// write new waitingPlanes to shm from waitingplanes fildes vector
					// write new airspace to shm from airspace fildes vector

					printf("\n");

					pthread_mutex_unlock(&mutex);
				}
				rcvid = MsgReceive(chid, &msg, sizeof(msg), NULL);
			}

			ChannelDestroy(chid);

			return 0;
		}

private:
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

	// airspace list
	int shm_airspace;
	void *ptr_airspace;
	std::vector<std::string> airspaceFileNames;

	friend class Plane;
};


#endif /* SSR_H_ */
