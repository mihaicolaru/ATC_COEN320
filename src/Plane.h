/*
 * Plane.h
 *
 *  Created on: Oct. 24, 2022
 *      Author: mihai
 */

#ifndef PLANE_H_
#define PLANE_H_

#include <sstream>
#include <fstream>
#include <iostream>
#include <mutex>
#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <sys/siginfo.h>
#include <sys/neutrino.h>
#include <time.h>

#include "Timer.h"
#include "Limits"


#define OFFSET 1000000
#define PLANE_PERIOD 1000000

#define SPACE_X_MIN 0
#define SPACE_X_MAX 100000
#define SPACE_Y_MIN 0
#define SPACE_Y_MAX 100000
#define SPACE_Z_MIN 0
#define SPACE_Z_MAX 25000
#define SPACE_ELEVATION 15000

class Plane {
public:

	// constructor
	Plane(int _arrivalTime, int _ID, int _position[3], int _speed[3]){
		// initialize members
		arrivalTime = _arrivalTime;
		ID = _ID;
		for(int i = 0; i < 3; i++){
			position[i] = _position[i];
			speed[i] = _speed[i];
		}

		// set thread in detached state
		int rc = pthread_attr_init(&attr);
		if (rc){
			printf("ERROR, RC from pthread_attr_init() is %d \n", rc);
		}

		rc = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
		if (rc){
			printf("ERROR; RC from pthread_attr_setdetachstate() is %d \n", rc);
		}

		std::string filename = "log_" + std::to_string(ID) + ".txt";

		logfile.open(filename);

		logfile << "plane created\nposition: " << position[0] << ", " << position[1] << ", " << position[2] << "\nspeed: " << speed[0] << ", " << speed[1] << ", " << speed[2] << "\n";

//		start();

	}

	// destructor
	~Plane(){
	}

	bool start(){
		std::cout << "start called\n";
//		time(&at);
		return (pthread_create(&planeThread, &attr, updateStart, this) == 0);

//		pthread_join(planeThread, NULL);
	}

	bool stop(){

		pthread_join(planeThread, NULL);
		logfile.close();

		return 0;
	}

	static void *updateStart(void *context){
		//		std::cout << "updateStart called\n";
		// set priority
		return ((Plane *)context)->updatePosition();
	}

	void* updatePosition(void){
		//		std::cout << "start exec\n";
		// update position every second from position and speed every second
		int chid = ChannelCreate(0);
		if(chid == -1){
			std::cout << "couldn't create channel!\n";
		}

		logfile << "plane " << ID << " started:\ncurrent position: " << position[0] << ", " << position[1] << ", " << position[2] << "\n";

		Timer timer(chid);
		timer.setTimer(arrivalTime * 1000000, PLANE_PERIOD);

		int rcvid;
		Message msg;

		bool start = true;

		while(1) {
			if(start){
				// first cycle, wait for arrival time
				start = false;
			}
			else{
				if(rcvid == 0){
					for(int i = 0; i < 3; i++){
						position[i] = position[i] + speed[i];
					}

					if(position[0] < SPACE_X_MIN || position[0] > SPACE_X_MAX){
						// change shared mem object to null
						ChannelDestroy(chid);
						return 0;
					}
					if(position[1] < SPACE_Y_MIN || position[1] > SPACE_Y_MAX){
						ChannelDestroy(chid);
						return 0;
					}
					if(position[2] < SPACE_Z_MIN || position[2] > SPACE_Z_MAX){
						ChannelDestroy(chid);
						return 0;
					}
//					std::cout << "executing\n";
					std::cout << "plane " << ID << ":\ncurrent position: " << position[0] << ", " << position[1] << ", " << position[2] << "\n";
					logfile << "plane " << ID << ":\ncurrent position: " << position[0] << ", " << position[1] << ", " << position[2] << "\n";

				}
			}
			rcvid = MsgReceive(chid, &msg, sizeof(msg), NULL);
			//			std::cout << "executing end\n";
		}

		ChannelDestroy(chid);

		return 0;
	}

	int* answerRadar(){
		// might need a mutex here, maybe not since only read


		// return ID speed and position, per radar request

		//		char* planeInfo;
		//		std::string separator;
		//		separator = " ";
		//		std::stringstream info;
		//		info << ID << separator << arrivalTime << separator << position[0] << separator << position[1] << separator << position[2] << separator << speed[0]  << separator << speed[1]  << separator << speed[2];
		//
		//		planeInfo = info.str();

		int planeInfo[8];

		planeInfo[0] = ID;
		planeInfo[1] = arrivalTime;
		planeInfo[2] = position[0];
		planeInfo[3] = position[1];
		planeInfo[4] = position[2];
		planeInfo[5] = speed[0];
		planeInfo[6] = speed[1];
		planeInfo[7] = speed[2];

		std::cout << &planeInfo[0];

		return &planeInfo[0];
	}


	int receiveCommand(){
		// receive command from comm subsystem, adjust speed or position
		return 0;
	}


private:
	int arrivalTime;
	int ID;
	int position [3];
	int speed [3];
	pthread_t planeThread;
	pthread_attr_t attr;
	std::ofstream logfile;
	std::mutex mutex;
	time_t at;
	time_t et;
};


#endif /* PLANE_H_ */
