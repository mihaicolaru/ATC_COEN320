/*
 * Plane.h
 *
 *  Created on: Oct. 24, 2022
 *      Author: mihai
 */

#ifndef PLANE_H_
#define PLANE_H_

#include <iostream>
#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <sys/siginfo.h>
#include <sys/neutrino.h>

#include "Timer.h"

#define OFFSET 1000000
#define PERIOD 5000000

class Plane {
public:
	// constructor
	Plane(time_t _arrivalTime, int _ID, int _position[3], int _speed[3]){
		// initialize members
		arrivalTime = _arrivalTime;
		ID = _ID;
		for(int i = 0; i < 3; i++){
			position[i] = _position[i];
			speed[i] = _speed[i];
		}
		std::cout << "plane created\nposition: "
				<< position[0] << ", " << position[1] << ", " << position[2] << "\nspeed: "
				<< speed[0] << ", " << speed[0] << ", " << speed[0] << "\n";
	}

	// destructor
	~Plane(){

	}

	bool start(){
		std::cout << "start called\n";
		return (pthread_create(&planeThread, NULL, updateStart, this) == 0);
	}

	static void *updateStart(void *context){
		std::cout << "updateStart called\n";
		return ((Plane *)context)->updatePosition();
	}

	void* updatePosition(void){
		std::cout << "start exec\n";
		// update position every second from position and speed every second
		int chid;
		if ((chid = ChannelCreate (0)) == -1) {
		        fprintf (stderr, "%s:  couldn't create channel!\n");
		        perror (NULL);
		        exit (EXIT_FAILURE);
		    }

		Timer timer(chid);
		timer.setTimer(OFFSET, PERIOD);	//set timer with polling period 5

		int rcvid;
		Message msg;

		while(1) {
			std::cout << "executing start\n";
			rcvid = MsgReceive(chid, &msg, sizeof(msg), NULL);
			std::cout << rcvid << std::endl;

			if(rcvid == 0){
				for(int i = 0; i < 3; i++){
					position[i] = position[i] + speed[i];
				}
				std::cout << "current position: " << position[0] << ", " << position[1] << ", " << position[2] << "\n";
			}
			std::cout << "executing end\n";
		}

		ChannelDestroy(chid);

		return 0;
	}

	int answerRadar(){
		// return ID speed and position, per radar request
		return 0;
	}


	int receiveCommand(){
		// receive command from comm subsystem, adjust speed or position
		return 0;
	}


private:
	time_t arrivalTime;
	int ID;
	int position [3];
	int speed [3];
	pthread_t planeThread;
};


#endif /* PLANE_H_ */
