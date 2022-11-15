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
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <sys/siginfo.h>
#include <sys/neutrino.h>
#include <time.h>

#include "Timer.h"
#include "PSR.h"

#define OFFSET 1000000
#define PLANE_PERIOD 1000000

#define SPACE_X_MIN 0
#define SPACE_X_MAX 100000
#define SPACE_Y_MIN 0
#define SPACE_Y_MAX 100000
#define SPACE_Z_MIN 0
#define SPACE_Z_MAX 25000
#define SPACE_ELEVATION 15000

#define SIZE 4096

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

		initialize();

	}

	// destructor
	~Plane(){
		shm_unlink(fileName.c_str());
		pthread_mutex_destroy(&mutex);
	}

	int initialize(){

		// set thread in detached state
		int rc = pthread_attr_init(&attr);
		if (rc){
			printf("ERROR, RC from pthread_attr_init() is %d \n", rc);
		}

		rc = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
		if (rc){
			printf("ERROR; RC from pthread_attr_setdetachstate() is %d \n", rc);
		}

		char number[2];
		itoa(ID, number, 10);
		// open shm
		fileName = "plane_" + std::to_string(ID);
//		fileName += number;


		std::cout << "plane filename: " << fileName << "\n";

		shm_fd = shm_open(fileName.c_str(), O_CREAT | O_RDWR, 0666);
		if(shm_fd == -1){
			perror("in shm_open() plane");
			exit(1);
		}

		updateString();

		//		std::cout << planeString << "\n";

		ftruncate(shm_fd, sizeof(planeString));

		// map shm
		ptr = mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
		if(ptr == MAP_FAILED){
			printf("map failed\n");
			return -1;
		}



		// initial write
		sprintf((char* )ptr, "%s", planeString.c_str());
		//		printf("Initial read: ");
		//		printf("%s\n", ptr);

		return 0;
	}

	int start(){
		//		std::cout << "start called\n";
		if(pthread_create(&planeThread, &attr, &Plane::updateStart, (void *) this) != EOK){
			planeThread = NULL;
		}

		return 0;
	}

	bool stop(){

		pthread_join(planeThread, NULL);
		logfile.close();

		return 0;
	}

	static void *updateStart(void *context){
		//		std::cout << "updateStart called\n";
		// set priority
		((Plane *)context)->updatePosition();
		return 0;
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
					updateString();
					//					std::cout << planeString << "\n";

					pthread_mutex_lock(&mutex);


					if(position[0] < SPACE_X_MIN || position[0] > SPACE_X_MAX){
						// change shared mem object to null
						planeString = "terminated";
						sprintf((char* )ptr, "%s", planeString.c_str());
						ChannelDestroy(chid);
						return 0;
					}
					if(position[1] < SPACE_Y_MIN || position[1] > SPACE_Y_MAX){
						planeString = "terminated";
						sprintf((char* )ptr, "%s", planeString.c_str());
						ChannelDestroy(chid);
						return 0;
					}
					if(position[2] < SPACE_Z_MIN || position[2] > SPACE_Z_MAX){
						planeString = "terminated";
						sprintf((char* )ptr, "%s", planeString.c_str());
						ChannelDestroy(chid);
						return 0;
					}

					// write plane to shared memory
					sprintf((char* )ptr, "%s", planeString.c_str());

					pthread_mutex_unlock(&mutex);

					//					std::cout << "executing\n";
					//					std::cout << "plane " << ID << ":\ncurrent position: " << position[0] << ", " << position[1] << ", " << position[2] << "\n";
					//					logfile << "plane " << ID << ":\ncurrent position: " << position[0] << ", " << position[1] << ", " << position[2] << "\n";

				}
			}
			rcvid = MsgReceive(chid, &msg, sizeof(msg), NULL);
			//			std::cout << "executing end\n";
		}

		ChannelDestroy(chid);

		return 0;
	}

	void updateString(){
		std::string s = " ";
		planeString = std::to_string(ID) + " " + std::to_string(arrivalTime) + " " +
				std::to_string(position[0]) + " " + std::to_string(position[1]) + " " + std::to_string(position[2]) + " " +
				std::to_string(speed[0]) + " " + std::to_string(speed[1]) + " " + std::to_string(speed[2]) + "\n";
	}

	std::string getString(){
		return planeString;
	}

	void Print(){
		std::cout << planeString << "\n";
	}

	const char* getFD(){
		return fileName.c_str();
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

		//		int planeInfo[8];
		//
		//		planeInfo[0] = ID;
		//		planeInfo[1] = arrivalTime;
		//		planeInfo[2] = position[0];
		//		planeInfo[3] = position[1];
		//		planeInfo[4] = position[2];
		//		planeInfo[5] = speed[0];
		//		planeInfo[6] = speed[1];
		//		planeInfo[7] = speed[2];
		//
		//		std::cout << &planeInfo[0];
		//
		//		return &planeInfo[0];
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
	pthread_mutex_t mutex;
	time_t at;
	time_t et;

	int shm_fd;
	void *ptr;
	std::string planeString;
	std::string fileName;
	friend class PSR;
};


#endif /* PLANE_H_ */
