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

class ComputerSystem{
public:
	// construcor
	ComputerSystem(){
		initialize();

		start();
	}

	// destructor
	~ComputerSystem(){

	}

	int initialize(){

		// initialize

		readInput();

		// set threads in detached state
		int rc = pthread_attr_init(&attr);
		if (rc){
			printf("ERROR, RC from pthread_attr_init() is %d \n", rc);
		}

		rc = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
		if (rc){
			printf("ERROR; RC from pthread_attr_setdetachstate() is %d \n", rc);
		}

		// maybe separate reading file and initializing planes

		return 0;
	}

	int readInput(){
		std::string filename = "./input.txt";
		std::ifstream input_file_stream;

		input_file_stream.open(filename);

		if (!input_file_stream) {
			std::cout << "Can't find file input.txt" << std::endl;
			return 1;
		}
		int ID, arrivalTime, arrivalCordX, arrivalCordY, arrivalCordZ, arrivalSpeedX, arrivalSpeedY, arrivalSpeedZ;

		std::string separator = " ";

		int pos[3];
		int vel[3];

		// start timer here for comparing with arrival time
		while(input_file_stream >> ID >> arrivalTime >>
				arrivalCordX >> arrivalCordY >> arrivalCordZ >>
				arrivalSpeedX >> arrivalSpeedY >> arrivalSpeedZ){
			std::cout << ID << separator << arrivalTime << separator << arrivalCordX << separator << arrivalCordY << separator << arrivalCordZ << separator << arrivalSpeedX << separator << arrivalSpeedY << separator << arrivalSpeedZ << std::endl;
			// create variables from inputs
			pos[0] = arrivalCordX;
			pos[1] = arrivalCordY;
			pos[2] = arrivalCordZ;
			vel[0] = arrivalSpeedX;
			vel[1] = arrivalSpeedY;
			vel[2] = arrivalSpeedZ;
			Plane plane(arrivalTime, ID, pos, vel), *p;
			p = &plane;
			planes.push_back(p);
		}
		std::cout << "number of planes: " << planes.size() << std::endl;
		return 0;
	}

	void start(){
		// start scanning planes
		// set scheduling priority

		std::cout << "cs start called\n";

		pthread_create(&primaryRadar, &attr, startPSR, this);

		pthread_create(&secondaryRadar, &attr, startSSR, this);


	}

	void stop(){
		for(Plane* plane : planes){
			if(!planes.empty()){
				// do this when t_arrival < t_current
				plane->stop();
			}
			else{
				break;
			}
		}

		pthread_join(primaryRadar, NULL);
		pthread_join(secondaryRadar, NULL);

	}

	static void* startPSR(void *context){
		std::cout << "starting PSR\n";
		return ((ComputerSystem *)context)->PSR();
	}

	void* PSR(void){
		// run primary radar
		std::cout << "primary radar started\n";
		int chid = ChannelCreate(0);
		if(chid == -1){
			std::cout <<"couldn't create channel\n";
		}

		Timer timer(chid);
		timer.setTimer(PSR_PERIOD, PSR_PERIOD);

		int rcvid;
		Message msg;

		//		int ID, arrivalTime, posX, posY, posZ, velX, velY, velZ;

		while(1){
			std::cout << "executing PSR\n";
			int i = 0;



			for(Plane* plane : planes){
				if(!planes.empty()){
					// do this when t_arrival < t_current
					airspace.push_back(plane);
					planes.remove(plane);
					i++;
				}
				else{
					// stop PSR
					ChannelDestroy(chid);

					return 0;
				}
			}
			rcvid = MsgReceive(chid, &msg, sizeof(msg), NULL);
		}

		ChannelDestroy(chid);

		return 0;
	}

	static void *startSSR(void *context){
		std::cout << "starting SSR\n";
		return ((ComputerSystem *)context)->SSR();
	}

	void* SSR(void){
		// run secondary radar
		std::cout << "secondary radar started\n";
		int chid = ChannelCreate(0);
		if(chid == -1){
			std::cout <<"couldn't create channel\n";
		}

		Timer timer(chid);
		timer.setTimer(SSR_PERIOD, SSR_PERIOD);

		int rcvid;
		Message msg;

		//		int ID, arrivalTime, posX, posY, posZ, velX, velY, velZ;


		while(1){
			std::cout << "executing SSR\n";
			for(Plane* plane : airspace){
				if(!airspace.empty()){
					int* planeInfo = plane->answerRadar();

					std::cout << planeInfo;

					//					for(int i = 0; i < (int*)planeInfo.size(); i++){
					//						std::cout << ((int*)planeInfo[i]) << " ";
					//					}

					//					std::cout << "\n";

					//					int i = 0;
					//					while(token != NULL){
					//						planeInfo[i] = itoa(token);
					//						std::cout << planeInfo[i];
					//						i++;
					//					}
					//
					//					std::cout << planeInfo;
					//					ID = planeInfo[0];
					//					arrivalTime = planeInfo[1];
					//					posX = planeInfo[2];
					//					posY = planeInfo[3];
					//					posZ = planeInfo[4];
					//					velX = planeInfo[5];
					//					velY = planeInfo[6];
					//					velZ = planeInfo[7];
					//
					////					std::cout << "plane: " + ID +
					////							"\nposition: " + posX + ", " + posY + ", " + posZ +
					////							"\nspeed: " + velX + ", " + velY + ", " + velZ + "\n";
					//
					//					// store info somewhere for trajectory calculation
					//
					//					bool stop = false;
					//
					//					if(posX < SPACE_X_MIN || posX > SPACE_X_MAX){
					//						// stop plane
					//						stop = true;
					//					}
					//					if(posY < SPACE_Y_MIN || posY > SPACE_Y_MAX){
					//						// stop plane
					//						stop = true;
					//					}
					//					if(posZ < SPACE_Z_MIN || posZ > SPACE_Z_MAX){
					//						// stop plane
					//						stop = true;
					//					}
					//
					//					if(stop){
					//						plane->stop();
					//					}

				}
				else{
					// stop SSR
					ChannelDestroy(chid);

					return 0;
				}
			}
			rcvid = MsgReceive(chid, &msg, sizeof(msg), NULL);
		}

		ChannelDestroy(chid);

		return 0;
	}


	int calculateTrajectories(){

		return 0;
	}

private:
	std::list<Plane*> planes;
	std::list<Plane*>::iterator pit1, pit2;

	std::list<Plane*> airspace;
	std::list<Plane*>::iterator ait1, ait2;
	pthread_t primaryRadar;
	pthread_t secondaryRadar;

	pthread_attr_t attr;


	// plane position matrix
	// ptr to radar (1 and 2)
	// ptr to comm
	// ptr to console
	// ptr to display

};



#endif /* COMPUTERSYSTEM_H_ */
