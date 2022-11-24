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

#define CS_PERIOD 1000000

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

				// ================= read airspace shm =================
				std::string readBuffer = "";
				int j = 0;
//				printf("computer system read airspace: %s\n", ptr_airspace);
//				std::cout << "compsys current planes:\n";
//				for(aircraft *craft : flyingPlanesInfo){
//					std::cout << "plane " << craft->id << ", keep: " << craft->keep << "\n";
//				}

				// variable buffer, these get overwritten as needed
				int id, arrTime, pos[3], vel[3];

				// find planes in airspace
				for(int i = 0; i < SIZE_SHM_AIRSPACE; i++){
					char readChar = *((char *)ptr_airspace + i);

					// end of airspace shm, line termination found
					if(readChar == ';'){
						// i=0 no planes found
						if(i == 0){
							std::cout << "compsys no flying planes\n";
							break;
						}

						// load last field in buffer
						vel[2] = atoi(readBuffer.c_str());
						std::cout << "compsys found last plane " << id << "\n";

						// check if already in airspace, if yes update with current data
						bool inList = false;
						for(aircraft *craft : flyingPlanesInfo){
							std::cout << "target plane id: " << id << "\n";
							std::cout << "current plane: " << craft->id << "\n";

							if(craft->id == id){
								std::cout << "compsys: plane " << craft->id << " already in list, updating new values\n";
								// TODO: implement check of current position with predicted position

								// if found, update with current info
								craft->pos[0] = pos[0];
								craft->pos[1] = pos[1];
								craft->pos[2] = pos[2];
								craft->vel[0] = vel[0];
								craft->vel[1] = vel[1];
								craft->vel[2] = vel[2];
								// if found, keep, if not will be removed
								craft->keep = true;
								// it is already in the list, do not add new
								inList = true;

								std::cout << "compsys plane " << craft->id << " new values:\n"
										<< "posx " << craft->pos[0] << ", posy " << craft->pos[1] << ", posz " << craft->pos[2] << "\n"
										<< "velx " << craft->vel[0] << ", vely " << craft->vel[1] << ", velz " << craft->vel[2] << "\n"
										<< "set keep value to: " << craft->keep << "\n";
								break;
							}
						}

						// if plane was not already in the list, add it
						if(!inList){
							std::cout << "compsys found new plane " << id << "\n";
							// new pointer to struct, set members from read
							aircraft *currentAircraft = new aircraft();
							currentAircraft->id = id;
							currentAircraft->t_arrival = arrTime;
							currentAircraft->pos[0] = pos[0];
							currentAircraft->pos[1] = pos[1];
							currentAircraft->pos[2] = pos[2];
							currentAircraft->vel[0] = vel[0];
							currentAircraft->vel[1] = vel[1];
							currentAircraft->vel[2] = vel[2];
							currentAircraft->keep = true;	// keep for first computation
							flyingPlanesInfo.push_back(currentAircraft);	// add to struct pointer vector
						}

						break;
					}
					// found plane in airspace (plane termination character)
					else if(readChar == '/'){
						// load last value in buffer
						vel[2] = atoi(readBuffer.c_str());
						std::cout << "compsys found next plane " << id << "\n";

						// check if already in airspace, if yes update with current data
						bool inList = false;
						for(aircraft *craft : flyingPlanesInfo){
							std::cout << "target plane id: " << id << "\n";
							std::cout << "current plane: " << craft->id << "\n";

							if(craft->id == id){
								std::cout << "compsys: plane " << craft->id << " already in list, updating new values\n";
								// TODO: implement check of current position with predicted position

								// if found, update with current info
								craft->pos[0] = pos[0];
								craft->pos[1] = pos[1];
								craft->pos[2] = pos[2];
								craft->vel[0] = vel[0];
								craft->vel[1] = vel[1];
								craft->vel[2] = vel[2];
								// if found, keep, if not will be removed
								craft->keep = true;
								// if already in list, do not add new
								inList = true;

								std::cout << "compsys plane " << craft->id << " new values:\n"
										<< "posx " << craft->pos[0] << ", posy " << craft->pos[1] << ", posz " << craft->pos[2] << "\n"
										<< "velx " << craft->vel[0] << ", vely " << craft->vel[1] << ", velz " << craft->vel[2] << "\n"
										<< "set keep value to: " << craft->keep << "\n";
								break;
							}
						}

						// if plane was not already in the list, add it
						if(!inList){
							// new pointer to struct, set members from read
							std::cout << "compsys found new plane " << id << "\n";
							aircraft *currentAircraft = new aircraft();
							currentAircraft->id = id;
							currentAircraft->t_arrival = arrTime;
							currentAircraft->pos[0] = pos[0];
							currentAircraft->pos[1] = pos[1];
							currentAircraft->pos[2] = pos[2];
							currentAircraft->vel[0] = vel[0];
							currentAircraft->vel[1] = vel[1];
							currentAircraft->vel[2] = vel[2];
							currentAircraft->keep = true;	// keep for first computation
							flyingPlanesInfo.push_back(currentAircraft);	// add to struct pointer vector
						}

						// reset buffer and index for next plane
						readBuffer = "";
						j = 0;
						continue;
					}
					// found next data field in current plane
					else if(readChar == ','){
						//						std::cout << "found next plane field data: " << readBuffer << "\n";
						switch(j){
						// add whichever character the index j has arrived to
						case 0:
							id = atoi(readBuffer.c_str());
							readBuffer = "";
							j++;
							continue;
						case 1:
							arrTime = atoi(readBuffer.c_str());
							readBuffer = "";
							j++;
							continue;
						case 2:
							pos[0] = atoi(readBuffer.c_str());
							readBuffer = "";
							j++;
							continue;
						case 3:
							pos[1] = atoi(readBuffer.c_str());
							readBuffer = "";
							j++;
							continue;
						case 4:
							pos[2] = atoi(readBuffer.c_str());
							readBuffer = "";
							j++;
							continue;
						case 5:
							vel[0] = atoi(readBuffer.c_str());
							readBuffer = "";
							j++;
							continue;
						case 6:
							vel[1] = atoi(readBuffer.c_str());
							readBuffer = "";
							j++;
							continue;
						default:
							// do nothing
							readBuffer = "";
							break;
						}
					}
					readBuffer += readChar;
				}
				// ================= end read airspace =================

//				std::cout << "remaining planes:\n";
//				for(aircraft *craft : flyingPlanesInfo){
//					//					craft.keep = true;
//					std::cout << "plane " << craft->id << ", keep: " << craft->keep << "\n";
//					std::cout << "new values: posx "<< craft->pos[0] << ", posy " << craft->pos[1] << ", posz " << craft->pos[2] << "\n";
//				}


				// ================= print airspace info =================

				printf("airspace that was read: %s\n", ptr_airspace);
				// print what was found, remove what is no longer in the airspace
				auto it = flyingPlanesInfo.begin();
				while(it != flyingPlanesInfo.end()){
					std::cout << "plane " << (*it)->id << " keep: " << (*it)->keep << "\n";
					bool temp = (*it)->keep;	// check if plane was terminated

					if(!temp){
						std::cout << "compsys found plane " << (*it)->id << " terminated\n";
						it = flyingPlanesInfo.erase(it);
						numPlanes--;
						std::cout << "computer system number of planes left: " << numPlanes << "\n";
					}
					else{
						// print plane info
						printf("plane %i:\n", (*it)->id);
						printf("posx: %i, posy: %i, posz: %i\n", (*it)->pos[0], (*it)->pos[1], (*it)->pos[2]);
						printf("velx: %i, vely: %i, velz: %i\n", (*it)->vel[0], (*it)->vel[1], (*it)->vel[2]);

						(*it)->keep = false;	// if found next time, this will become true again

						// only increment if no plane to remove
						++it;
					}
				}
				// ================= end print airspace =================


			}


			// termination check
			if(numPlanes <= 0){
				std::cout << "computer system done\n";
				ChannelDestroy(chid);
				return 0;
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
	std::vector<aircraft *> flyingPlanesInfo;

	// display shm
	int shm_displayData;
	void *ptr_positionData;



	friend class Plane;

};

#endif /* COMPUTERSYSTEM_H_ */
