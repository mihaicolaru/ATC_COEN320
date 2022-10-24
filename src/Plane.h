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

class Plane {
public:
	// constructor
	Plane(){

	}

	// destructor
	~Plane(){

	}

	int updatePosition(){
		// update position every second from position and speed every second
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
};


#endif /* PLANE_H_ */
