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

#include "SSR.h"
#include "Plane.h"
#include "Timer.h"
#include "Display.h"

#define CS_PERIOD 5000000

class Plane;

struct aircraft{
	int id;
	int t_arrival;
	int pos[3];
	int vel[3];
	bool keep;
};

class ComputerSystem{
public:
	// construcor
	ComputerSystem(int numberOfPlanes){
		numPlanes = numberOfPlanes;
		initialize();
	}

	// destructor
	~ComputerSystem(){
		shm_unlink("airspace");
		shm_unlink("display");
		pthread_mutex_destroy(&mutex);
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

		// shared memory from ssr for the airspace
		shm_airspace = shm_open("airspace", O_RDONLY, 0666);
		if (shm_airspace == -1) {
			perror("in compsys shm_open() airspace");
			exit(1);
		}

		ptr_airspace = mmap(0, SIZE_SHM_AIRSPACE, PROT_READ, MAP_SHARED, shm_airspace, 0);
		if(ptr_airspace == MAP_FAILED) {
			perror("in compsys map() airspace");
			exit(1);
		}

		// open shm display
		shm_displayData = shm_open("display", O_RDWR, 0666);
		if(shm_displayData == -1) {
			perror("in compsys shm_open() display");
			exit(1);
		}

		// map display shm
		ptr_positionData = mmap(0, SIZE_SHM_DISPLAY, PROT_READ | PROT_WRITE, MAP_SHARED, shm_displayData, 0);
		if(ptr_positionData == MAP_FAILED) {
			perror("in copmsys map() display");
			exit(1);
		}

		return 0;
	}

	// start computer system
	int start(){
		if (pthread_create(&computerSystemThread, &attr, &ComputerSystem::startComputerSystem, (void *)this) != EOK) {
			computerSystemThread = NULL;
		}
	}

	// join computer system thread
	int stop(){
		pthread_join(computerSystemThread, NULL);
		return 0;
	}

	static void *startComputerSystem(void *context) { ((ComputerSystem *)context)->calculateTrajectories(); }

	void *calculateTrajectories(void){
		// create channel to communicate with timer
		int chid = ChannelCreate(0);
		if(chid == -1){
			std::cout << "couldn't create channel\n";
		}

		Timer timer(chid);
		timer.setTimer(CS_PERIOD, CS_PERIOD);

		int rcvid;
		Message msg;

		while(1){
			if(rcvid == 0){
//
//				//				pthread_mutex_lock(&mutex);
//				//				printf("cs read airspace:\n");
//				//				for(int i = 0; i < SIZE_SHM_AIRSPACE; i++){
//				//					char readChar = *((char *)ptr_airspace + i);
//				//					if(readChar == ';'){
//				//						break;
//				//					}
//				//					printf("%c", (void *)readChar);
//				//				}
//				//				printf("\n");
//				//				pthread_mutex_unlock(&mutex);
//
//				// read airspace shm
//				std::string readBuffer = "";
//				int j = 0;
//				aircraft currentAircraft;
//
//				for(int i = 0; i < SIZE_SHM_AIRSPACE; i++){
//					char readChar = *((char *)ptr_airspace + i);
//
//					if(readChar == ';'){
//						// end of airspace shm
//						if(i == 0){
//							break;	// no planes
//						}
//						// load last field in plane
//						// found plane
//						//						std::cout << "found last plane: " << readBuffer << "\n";
//						currentAircraft.vel[2] = atoi(readBuffer.c_str());
//
//						// check if already in airspace, if yes compare with current frame data
//						bool inList = true;
//						for(aircraft craft : flyingPlanesInfo){
//							if(craft.id == currentAircraft.id){
//								// TODO: implement check of current position with predicted position
//
//								craft.pos[0] = currentAircraft.pos[0];
//								craft.pos[1] = currentAircraft.pos[1];
//								craft.pos[2] = currentAircraft.pos[2];
//								craft.vel[0] = currentAircraft.vel[0];
//								craft.vel[1] = currentAircraft.vel[1];
//								craft.vel[2] = currentAircraft.vel[2];
//								// if found, keep, if not will be removed
//								inList = false;
//								craft.keep = true;
//							}
//						}
//
//						// if plane was not already in the list, add it
//						if(inList){
//							currentAircraft.keep = true;	// keep for first computation
//							flyingPlanesInfo.push_back(currentAircraft);
//						}
//
//						break;
//					}
//					else if(readChar == '/'){
//						// found plane
//						//						std::cout << "found next plane: " << readBuffer << "\n";
//						currentAircraft.vel[2] = atoi(readBuffer.c_str());
//
//						// check if already in airspace, if yes compare with current frame data
//						bool inList = true;
//						for(aircraft craft : flyingPlanesInfo){
//							if(craft.id == currentAircraft.id){
//								// TODO: implement check of current position with predicted position
//
//								craft.pos[0] = currentAircraft.pos[0];
//								craft.pos[1] = currentAircraft.pos[1];
//								craft.pos[2] = currentAircraft.pos[2];
//								craft.vel[0] = currentAircraft.vel[0];
//								craft.vel[1] = currentAircraft.vel[1];
//								craft.vel[2] = currentAircraft.vel[2];
//
//								craft.keep = true;
//								inList = false;
//							}
//						}
//
//						// if plane was not already in the list, add it
//						if(inList){
//							currentAircraft.keep = true;	// keep for first computation
//							flyingPlanesInfo.push_back(currentAircraft);
//						}
//
//						// reset buffer and index for next plane
//						readBuffer = "";
//						j = 0;
//						continue;
//					}
//					else if(readChar == ','){
//						// found next plane data field
//						//						std::cout << "found next plane field data: " << readBuffer << "\n";
//
//						switch(j){
//						case 0:
//							currentAircraft.id = atoi(readBuffer.c_str());
//							readBuffer = "";
//							j++;
//							continue;
//						case 1:
//							currentAircraft.t_arrival = atoi(readBuffer.c_str());
//							readBuffer = "";
//							j++;
//							continue;
//						case 2:
//							currentAircraft.pos[0] = atoi(readBuffer.c_str());
//							readBuffer = "";
//							j++;
//							continue;
//						case 3:
//							currentAircraft.pos[1] = atoi(readBuffer.c_str());
//							readBuffer = "";
//							j++;
//							continue;
//						case 4:
//							currentAircraft.pos[2] = atoi(readBuffer.c_str());
//							readBuffer = "";
//							j++;
//							continue;
//						case 5:
//							currentAircraft.vel[0] = atoi(readBuffer.c_str());
//							readBuffer = "";
//							j++;
//							continue;
//						case 6:
//							currentAircraft.vel[1] = atoi(readBuffer.c_str());
//							readBuffer = "";
//							j++;
//							continue;
//							//						case 7:
//							//							currentAircraft.vel[2] = atoi(readBuffer.c_str());
//							//							readBuffer = "";
//							//							j++;
//							//							continue;
//						default:
//							// do nothing
//							readBuffer = "";
//							break;
//						}
//					}
//					readBuffer += readChar;
//				}
//
//				//				printf("computer system read airspace:\n");
//
//				// print what was found
//				int i = 0;
//				auto it = flyingPlanesInfo.begin();
//				while(it != flyingPlanesInfo.end()){
//					bool temp = *it.keep;
//					if(temp){
//						it = flyingPlanesInfo.erase(it);
//						numPlanes--;
//					}
//					else{
//						//						printf("plane %i:\n", craft.id);
//						//						printf("posx: %i, posy: %i, posz: %i\n", craft.pos[0], craft.pos[1], craft.pos[2]);
//						//						printf("velx: %i, vely: %i, velz: %i\n", craft.vel[0], craft.vel[1], craft.vel[2]);
//
//						*it.keep = false;
//						i++;
//						++it;
//					}
//				}
//
//			}
//
//			if(numPlanes <= 0){
//				ChannelDestroy(chid);
//				return 0;
			}

			rcvid = MsgReceive(chid, &msg, sizeof(msg), NULL);
		}

		ChannelDestroy(chid);

		return 0;
	}

private:
	// members
	//	std::vector<map[33][33]> airspaceFrames;

	int numPlanes;


	// thread members
	pthread_t computerSystemThread;
	pthread_attr_t attr;
	pthread_mutex_t mutex;

	// timing members
	time_t at;
	time_t et;

	// shm members
	// airspace shm
	int shm_airspace;
	void *ptr_airspace;
	std::vector<aircraft> flyingPlanesInfo;

	// display shm
	int shm_displayData;
	void *ptr_positionData;



	friend class Plane;

};

#endif /* COMPUTERSYSTEM_H_ */
