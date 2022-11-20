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

#define SIZE_SHM_PLANES 4096

class Plane {
public:

	// constructor
	Plane(int _ID, int _arrivalTime, int _position[3], int _speed[3]){
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

		// instantiate filename
		fileName = "plane_" + std::to_string(ID);


//		printf("Plane %i filename: %s\n", ID, fileName.c_str());

		// open shm object
		shm_fd = shm_open(fileName.c_str(), O_CREAT | O_RDWR, 0666);
		if(shm_fd == -1){
			perror("in shm_open() plane");
			exit(1);
		}

		// set the size of shm
		ftruncate(shm_fd, SIZE_SHM_PLANES);

		// map shm
		ptr = mmap(0, SIZE_SHM_PLANES, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
		if(ptr == MAP_FAILED){
			printf("map failed\n");
			return -1;
		}

		// update string of plane data
		updateString();


		// initial write
		sprintf((char* )ptr, "%s", planeString.c_str());

		return 0;
	}

	// call static function to start thread
	int start(){
		//		std::cout << "start called\n";
		if(pthread_create(&planeThread, &attr, &Plane::updateStart, (void *) this) != EOK){
			planeThread = NULL;
		}

		return 0;
	}

	// join execution thread
	bool stop(){

		pthread_join(planeThread, NULL);
		return 0;
	}

	// entry point for execution thread
	static void *updateStart(void *context){
		//		std::cout << "updateStart called\n";
		// set priority
		((Plane *)context)->updatePosition();
		return 0;
	}

	// update position every second from position and speed
	void* updatePosition(void){
		// create channel to link timer
		int chid = ChannelCreate(0);
		if(chid == -1){
			std::cout << "couldn't create channel!\n";
		}

		// create timer and set offset and period
		Timer timer(chid);
		timer.setTimer(arrivalTime * 1000000, PLANE_PERIOD);

		// buffers for message from timer
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
					// save modifications to string
					updateString();

					pthread_mutex_lock(&mutex);

					// check for airspace limits, write to shm

					if(position[0] < SPACE_X_MIN || position[0] > SPACE_X_MAX){
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
				}
			}
			// wait until next timer pulse
			rcvid = MsgReceive(chid, &msg, sizeof(msg), NULL);
		}

		ChannelDestroy(chid);

		return 0;
	}

	// stringify plane data members
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

	std::string getFileString(){
		return fileName;
	}

	int receiveCommand(){
		// receive command from comm subsystem via computer, adjust speed or position
		// adjust member variables according to command
		return 0;
	}


private:
	// data members
	int arrivalTime;
	int ID;
	int position [3];
	int speed [3];

	// thread members
	pthread_t planeThread;
	pthread_attr_t attr;
	pthread_mutex_t mutex;

	// timing members
	time_t at;
	time_t et;

	// shm members
	int shm_fd;
	void *ptr;
	std::string planeString;
	std::string fileName;

};


#endif /* PLANE_H_ */
