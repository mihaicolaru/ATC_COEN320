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

#define SIZE_SHM_AIRSPACE 4096
#define SIZE_SHM_SSR 4096
#define SIZE_SHM_PSR 4096
#define SSR_PERIOD 2000000

class Plane;

class SSR {
public:

	// constructor
	SSR(int numberOfPlanes){ initialize(numberOfPlanes); }

	// destructor
	~SSR(){

		shm_unlink("flying_planes");
		shm_unlink("airspace");
		for(std::string filename : fileNames){
			shm_unlink(filename.c_str());
		}
		pthread_mutex_destroy(&mutex);
	}

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

		ptr_flyingPlanes = mmap(0, SIZE_SHM_SSR, PROT_READ | PROT_WRITE, MAP_SHARED, shm_flyingPlanes, 0);
		if (ptr_flyingPlanes == MAP_FAILED) {
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
		ptr_airspace = mmap(0, SIZE_SHM_AIRSPACE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_airspace, 0);
		if (ptr_airspace == MAP_FAILED) {
			perror("in map() SSR: airspace");
			exit(1);
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

				bool write = false;

				// read flying planes shm

				std::string FD_buffer = "";

				pthread_mutex_lock(&mutex);
				for(int i = 0; i < SIZE_SHM_SSR; i++){
					//					std::cout << "buffer: " << FD_buffer << "\n";
					char readChar = *((char *)ptr_flyingPlanes + i);


					if(readChar == ';'){
						if(i == 0){
//							std::cout << "ssr no flying planes\n";
							break;	// no flying planes
						}

//						std::cout << "ssr last plane: " << FD_buffer << "\n";

						bool inFile = true;

						for(std::string filename : flyingFileNames){
							if(filename == FD_buffer){
//								std::cout << FD_buffer << " already in list\n";
								inFile = false;
							}
						}

						if(inFile){
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

						bool inFile = true;

						for(std::string filename : flyingFileNames){
							if(filename == FD_buffer){
//								std::cout << FD_buffer << " already in list\n";
								inFile = false;
							}
						}

						if(inFile){
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


				// get plane info

				// buffer for all plane info
				std::string airspaceBuffer = "";


				int i = 0;
				auto it = planePtrs.begin();
				//				pthread_mutex_lock(&mutex);
				while(it != planePtrs.end()){
					std::string readBuffer = "";
					char readChar = *((char *)*it);

					if(readChar == 't'){
						write = true;	// found plane to remove

						// remove plane (terminated)
//						std::cout << "ssr found terminated\n";

						// remove current fd from flying planes fd vector
						flyingFileNames.erase(flyingFileNames.begin() + i);

						// remove current plane from ptr vector
						it = planePtrs.erase(it);

						numPlanes--;
//						std::cout << "ssr number of planes: " << numPlanes << "\n";
					}
					else{
						int j = 0;
//						std::cout << "reading current plane data\n";
						for(; j < SIZE_SHM_PLANES; j++){
							char readChar = *((char *)*it + j);
							if(readChar == ';'){
								break;
							}

							if(i != 0 && j == 0){
								readBuffer += "/";	// plane separator
							}
							readBuffer += readChar;
						}
						if(j == 0){
							break;	// no planes
						}
						i++;	// only increment if no plane to terminate and plane info added
						++it;
					}

					airspaceBuffer += readBuffer;
				}

				// termination character for airspace write
				airspaceBuffer += ";";


				if(write){
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
					sprintf((char *)ptr_flyingPlanes, "%s", currentAirspace.c_str());
//					printf("ssr flying planes after write: %s\n", ptr_flyingPlanes);

				}

				// write new airspace to shm
//				std::cout << "ssr current airspace: " << airspaceBuffer << "\n";
				sprintf((char *)ptr_airspace, "%s", airspaceBuffer.c_str());
//				printf("ssr airspace after write: %s\n", ptr_airspace);

				pthread_mutex_unlock(&mutex);

				// check for termination
				if(numPlanes <= 0){
//					std::cout << "ssr done\n";
					ChannelDestroy(chid);

					return 0;
				}

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

	// flying planes list
	int shm_flyingPlanes;
	void *ptr_flyingPlanes;
	std::vector<std::string> flyingFileNames;

	// airspace shm
	int shm_airspace;
	void *ptr_airspace;

	friend class Plane;
	friend class PSR;

	// number of planes left
	int numPlanes;
};


#endif /* SSR_H_ */
