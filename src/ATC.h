/*
 * ATC.h
 *
 *  Created on: Nov. 12, 2022
 *      Author: Mihai
 */

#ifndef ATC_H_
#define ATC_H_

#include <vector>
#include <list>
#include <fstream>
#include <time.h>
#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <sys/siginfo.h>
#include <sys/neutrino.h>

#include "Timer.h"
#include "Display.h"

#define PSR_PERIOD 5000000
#define SSR_PERIOD 5000000

#define SPACE_X_MIN 0
#define SPACE_X_MAX 100000
#define SPACE_Y_MIN 0
#define SPACE_Y_MAX 100000
#define SPACE_Z_MIN 0
#define SPACE_Z_MAX 25000
#define SPACE_ELEVATION 15000

class ATC{
public:

	ATC(){

	}

	~ATC(){

	}

	int initialize(){
		// read input from file
		// initialize shared memory space for each plane
		// save address pointer to planes vector AND readyPlanes vector
		// initialize shared memory space for airspace
		// initialize plane thread attr struc (if exit on end of exec requires different attr)
		// initialize thread attr struct (joinable)
		return 0;	// set to error code if any
	}

	int start(){
		// start timer for arrivalTime comparison
		// create thread for each plane with argument casted to void pointer to assign shared memory object to thread
		// create thread for PSR
		// create thread for SSR
		// create thread for ComputerSystem
		// create thread for Display
		// create thread for Console
		// create thread for Comm

		// ============ execution time ============

		// join each plane thread (or have them terminate on exec complete)
		// join PSR
		// join SSR
		// join ComputerSystem
		// join Display
		// join Console
		// join Comm
		return 0;	// set to error code if any
	}

	static void *startPlaneUpdate(void *context){
		// set priority
		return ((ATC *)context)->updatePlanePosition();
	}

	static void *startPSR(void *context){
		// set priority
		return ((ATC *)context)->PSR();
	}

	static void *startSSR(void *context){
		// set priority
		return ((ATC *)context)->SSR();
	}

	static void *startComputerSystem(void *context){
		// set priority
		return ((ATC *)context)->ComputerSystem();
	}

	static void *startDisplay(void *context){
		// set priority
		return ((ATC *)context)->Display();
	}

	static void *startConsole(void *context){
		// set priority
		return ((ATC *)context)->Console();
	}

	static void *startComm(void *context){
		// set priority
		return ((ATC *)context)->Comm();
	}



protected:

	void* updatePlanePosition(void){
		// update plane position
		// checks for airspace limitations
		// if out of airspace, set plane sm_obj to null, terminate thread
	}

	void* PSR(void){
		// poll waiting planes
		// if null enter critical section: remove plane
		// else if t_current > t_arrival enter critical section:
		// copy then remove element waiting planes.at(i)
		// append to end of airspace
		// leave critical section
		// when waiting planes queue is empty terminate thread
	}

	void* SSR(void){
		// poll airspace
		// read ID, pos, vel
		// create entry in airspace sm_obj
		// once all planes polled, notify computer system (msgsend)
		// when plane found to be null enter critical section:
		// remove plane
		// leave critical section
		// when waiting planes AND airspace queues are empty terminate thread
	}

	void* ComputerSystem(void){
		// when notified, parse airspace sm_obj
		// check for airspace violations within [t, t+3min], notify if found
		// populate map for display (maybe lock critical section), send command if info needed
		// when command received, start interrupt routine related to command
	}

	void* Display(void){
		// print airspace from map value every 5 seconds
		// if command from computer exists, update next print with necessary info
	}

	void* Console(void){
		// generate interrupt when keyboard input detected or needed
		// fill buffer with keyboard input until line termination
		// parse command, send to computerSystem
	}

	void* Comm(void){
		// when notified by computerSystem, sends message m to plane R
	}

};


#endif /* ATC_H_ */
