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
#include "Limits.h"

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

		commandCounter = 0;
		commandInProgress = false;

		// initialize thread members
		initialize();
	}

	// destructor
	~Plane(){
		shm_unlink(fileName.c_str());
		pthread_mutex_destroy(&mutex);
	}

	// call static function to start thread
	int start(){
		//		std::cout << getFD() << " start called\n";
		if(pthread_create(&planeThread, &attr, &Plane::startPlane, (void *) this) != EOK){
			planeThread = NULL;
		}

		return 0;
	}

	// join execution thread
	bool stop(){
		//		std::cout << getFD() << " stop called\n";
		pthread_join(planeThread, NULL);
		return 0;
	}

	// entry point for execution thread
	static void *startPlane(void *context){
		//		std::cout << "updateStart called\n";
		// set priority
		((Plane *)context)->flyPlane();
		return 0;
	}

	// get plane file descriptor as const char array
	const char* getFD(){
		return fileName.c_str();
	}



private:

	// initialize thread and shm members
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


		// initial write + space for comm system
		sprintf((char* )ptr, "%s0;", planeString.c_str());
		//		printf("%s\n", ptr);

		return 0;
	}

	// update position every second from position and speed
	void* flyPlane(void){
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
				//				std::cout << getFD() << " first iteration\n";
				start = false;
			}
			else{
				if(rcvid == 0){
					pthread_mutex_lock(&mutex);

					// check comm system for potential command
					answerComm();

					// update position based on speed
					updatePosition();

					// check for airspace limits, write to shm
					if(checkLimits() == 0){
						std::cout << getFD << " terminated\n";
						ChannelDestroy(chid);
						return 0;
					}

					pthread_mutex_unlock(&mutex);
				}
			}
			// wait until next timer pulse
			rcvid = MsgReceive(chid, &msg, sizeof(msg), NULL);
		}

		ChannelDestroy(chid);

		return 0;
	}

	// check comm system for potential commands
	void answerComm(){
		// check if executing command
		if(commandInProgress){
			// decrement counter
//			std::cout << "command execution incomplete\n";
			commandCounter--;
			if(commandCounter <= 0){
				commandInProgress = false;
//				speed[2] = 0;
			}
			return;
		}
		speed[2] = 0;

//		printf("answerComm() read: %s\n", ptr);

		// find end of plane info
		int i = 0;
		char readChar;
		for(; i < SIZE_SHM_PLANES; i++){
			readChar = *((char *)ptr + i);
			if(readChar == ';'){
				break;	// found end
			}
		}


		// check for command presence
		if(*((char *)ptr + i + 1) == ';' || *((char *)ptr + i + 1) == '0' ){
//			std::cout << "no command\n";
			return;
		}

		// set index to next
		i++;
		int startIndex = i;
		readChar = *((char *)ptr + i);
		std::string buffer = "";
		while(readChar != ';'){
			buffer += readChar;
			readChar = *((char *)ptr + ++i);
		}
		//		readChar = *((char *)ptr + ++i);
		buffer += ';';
//		std::cout << getFD() << " read command: " << buffer << "\n";


		// parse command
		std::string parseBuf = "";
		int currParam;
		for(int j = 0; j <= buffer.size(); j++){
			char currChar = buffer[j];

			switch(currChar){
			case ';':
				// reached end of command, apply command
//				std::cout << "setting vel[" << currParam << "] to " << parseBuf << "\n";
				speed[currParam] = std::stoi(parseBuf);
				break;
			case '/':
				// read end of one velocity command, apply command, clear buffer
//				std::cout << "setting vel[" << currParam << "] to " << parseBuf << "\n";
				speed[currParam] = std::stoi(parseBuf);
				parseBuf = "";
				continue;
			case 'x':
				// command to x velocity
				currParam = 0;
				continue;
			case 'y':
				// command to y velocity
				currParam = 1;
				continue;
			case 'z':
				// command to z velocity
				currParam = 2;
				continue;
			case ',':
				// spacer, clear buffer
				parseBuf = "";
				continue;
			default:
				parseBuf += currChar;
				continue;
			}
		}

		for(int k = 0; k < 500; k += abs(speed[2])){
			commandCounter++;
		}

		commandInProgress = true;

		// remove command
		sprintf((char*)ptr + startIndex, "0;");
//		printf("after remove command: %s\n", ptr);
	}

	// update position based on speed
	void updatePosition(){
		//		std::cout << getFD() << " updating position\n";
		for(int i = 0; i < 3; i++){
			position[i] = position[i] + speed[i];
		}
		// save modifications to string
		updateString();
		//		Print();
	}

	// stringify plane data members
	void updateString(){
		std::string s = ",";
		planeString = std::to_string(ID) + s + std::to_string(arrivalTime) + s +
				std::to_string(position[0]) + s + std::to_string(position[1]) + s + std::to_string(position[2]) + s +
				std::to_string(speed[0]) + s + std::to_string(speed[1]) + s + std::to_string(speed[2]) + ";";
	}

	// check airspace limits for operation termination
	int checkLimits(){
		if(position[0] < SPACE_X_MIN || position[0] > SPACE_X_MAX){
			planeString = "terminated";
			sprintf((char* )ptr, "%s0;", planeString.c_str());
			return 0;
		}
		if(position[1] < SPACE_Y_MIN || position[1] > SPACE_Y_MAX){
			planeString = "terminated";
			sprintf((char* )ptr, "%s0;", planeString.c_str());
			return 0;
		}
		if(position[2] < SPACE_Z_MIN || position[2] > SPACE_Z_MAX){
			planeString = "terminated";
			sprintf((char* )ptr, "%s0;", planeString.c_str());
			return 0;
		}

		// write plane to shared memory
		sprintf((char* )ptr, "%s0;", planeString.c_str());
		return 1;
	}


	// print plane info
	void Print(){
		std::cout << planeString << "\n";
	}

	// data members
	int arrivalTime;
	int ID;
	int position [3];
	int speed [3];

	// command members
	int commandCounter;
	bool commandInProgress;

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
