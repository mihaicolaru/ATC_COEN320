/*
 * PSR.h
 *
 *  Created on: Oct. 24, 2022
 *      Author: mihai, minhtrannhat
 */

#ifndef PSR_H_
#define PSR_H_

#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "Timer.h"
#include "Plane.h"

#define SIZE 4096
#define PSR_PERIOD 5000000

class PSR {
public:
	// constructor
	PSR() {

		initialize();
	}

	// destructor
	~PSR() {
		shm_unlink("plane_1");
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

		shm_fd = shm_open("plane_1", O_RDONLY, 0666);
		if(shm_fd == -1){
			perror("in shm_open() PSR");
			exit(1);
		}

		ptr = mmap(0, SIZE, PROT_READ, MAP_SHARED, shm_fd, 0);
		if(ptr == MAP_FAILED){
			perror("in map() PSR");
			exit(1);
		}

		return 0;
	}

	int start(){
		std::cout << "PSR start called\n";
		if(pthread_create(&PSRthread, &attr, &PSR::startPSR, (void* ) this) != EOK){
			PSRthread = NULL;
		}
	}

	int stop(){
		pthread_join(PSRthread, NULL);
		return 0;
	}

	static void *startPSR(void *context){
		((PSR *)context)->operatePSR();
	}

	void* operatePSR(void){
		//		std::cout << "start exec\n";
		// update position every second from position and speed every second
		int chid = ChannelCreate(0);
		if(chid == -1){
			std::cout << "couldn't create channel!\n";
		}

		Timer timer(chid);
		timer.setTimer(PSR_PERIOD, PSR_PERIOD);

		int rcvid;
		Message msg;


		while(1) {
			if(rcvid == 0){
				pthread_mutex_lock(&mutex);

				printf("PSR read: ");
				printf("%s", ptr);
				printf("\n");

				pthread_mutex_unlock(&mutex);
			}
			rcvid = MsgReceive(chid, &msg, sizeof(msg), NULL);
			//			std::cout << "executing end\n";
		}

		ChannelDestroy(chid);

		return 0;
	}

	int discover() {
		// scan all planes and find ready (Tarrival <= Tcurrent)
		// move ready planes to ready list
	}

private:
	// list of all planes
	// list of ready planes

	pthread_t PSRthread;
	pthread_attr_t attr;
	pthread_mutex_t mutex;

	time_t at;
	time_t et;

	int shm_fd;
	void *ptr;

	friend class Plane;
};

#endif /* PSR_H_ */
