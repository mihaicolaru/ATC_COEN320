/*
 * Display.h
 *
 *  Created on: Oct. 24, 2022
 *      Author: mihai
 */

#ifndef DISPLAY_H_
#define DISPLAY_H_

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <iostream>
#include <errno.h>
#include <sys/siginfo.h>
#include <sys/neutrino.h>
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


#define SCALER 3000
#define MARGIN 100000
#define PERIOD_D 5000000 //5sec period

#define SIZE_SHM_DISPLAY 4096


const int block_count = (int)MARGIN/(int)SCALER;

//need sporadic task?
//Add vector source library, should be computer system

class Display{
public:
	//constructor
	//int _posX[], int _posY[], int _posZ[]
	Display(int numberOfPlanes){
		nbOfPlanesInAS = numberOfPlanes;
		initialize();
	}
	//destructor
	~Display(){

	}

	//Not used yet, plan for keep updating plane nb
	void currentPlaneNb(int numberOfPlanes){
		nbOfPlanesInAS = numberOfPlanes;
	}

	//Initialize thread
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
		initialize_shm();

	}

	void initialize_shm(){
		// open list of waiting planes shm
		shm_display = shm_open("display", O_RDONLY, 0666);
		if (shm_display == -1) {
			perror("in shm_open() display");
			exit(1);
		}

		ptr_display = mmap(0, SIZE_SHM_DISPLAY, PROT_READ, MAP_SHARED, shm_display, 0);
		if (ptr_display == MAP_FAILED) {
			perror("in map() PSR");
			exit(1);
		}

		// buffer to read display shm
		char buffer[SIZE];

		// read waiting planes shm
		for (int i = 0; i < SIZE; i++) {
			char readChar = *((char *)ptr_display + i);
			buffer[i] = readChar;
		}

		//		printf("PSR read allPlanes: %s\n", allPlanes);


		}
	}

	int start(){
		std::cout << "Start display function\n";
		//time(&at);
		if(pthread_create(&displayThread, &attr, &Display::updateStart, (void *) this) != EOK){
			displayThread = NULL;
		}
		return 0;
	}

	bool stop(){
		pthread_join(displayThread, NULL);
		std::cout << "Display terminated\n";
		return 0;
	}

	static void *updateStart(void *context){
		((Display *)context)->updateDisplay();
		return 0;
	}

	void *updateDisplay(void){
		//		printf("update display called\n");
		int chid = ChannelCreate(0);
		if(chid == -1){
			std::cout << "couldn't create display channel!\n";
		}

		Timer timer(chid);
		timer.setTimer(PERIOD_D, PERIOD_D);

		int rcvid;
		Message msg;

		int i = 2;

		while(1){
			pthread_mutex_lock(&mutex_d);
			for(void* ptr : planePtrs){
				printf("%s\n", ptr);

				// Then, in the function that's using the UserEvent:
				// Cast it back to a string pointer.
				std::string *sp = static_cast<std::string*>(ptr);
				// You could use 'sp' directly, or this, which does a copy.
				std::string s = *sp;
				std::cout << s << std::endl;
				std::string delim = " ";
				auto start = 0U;
				auto end = s.find(delim);
				int count = 0;
				while (end != std::string::npos)
				{
					std::cout << s.substr(start, end - start) << std::endl;
					//					switch(count){
					//					case 2:
					//
					//					}
					start = end + delim.length();
					end = s.find(delim, start);
					count++;
				}


			}
			printMap();
			pthread_mutex_unlock(&mutex_d);
			//			i--;

			rcvid = MsgReceive(chid, &msg, sizeof(msg), NULL);
		}


		//		while(i > 0){
		//			printMap();
		//			i--;
		//			rcvid = MsgReceive(chid, &msg, sizeof(msg), NULL);
		//		}

		//		time(&et);
		//		double exe = difftime(et,at);
		//		std::cout << "DISPLAY finished in: " << exe << std::endl;

		ChannelDestroy(chid);
		return 0;

	}




private:
	//time reader
	time_t at;
	time_t et;

	//threads
	pthread_t displayThread;
	pthread_attr_t attr;
	pthread_mutex_t mutex_d;// mutex for display
	int posX[5] = {50000,12000,30000,20000,70000};
	int posY[5] = {19000,12000,7000, 50000, 90000};
	int posZ[5] = {12000,15000,11000,10000,10000};
	//		int posX[5];
	//		int posY[5];
	//		int posZ[5];
	int map[block_count][block_count]={{0}}; // Shrink 100k by 100k map to 10 by 10, each block is 10k by 10k
	//	memset(map, 0, sizeof(map[0][0]) * block_count * block_count);

	int nbOfPlanesInAS=0;

	// shm members
	// waiting planes list
	int shm_display;
	void *ptr_display;
	std::vector<std::string> fileNames;

	// access waiting planes
	std::vector<void *> planePtrs;

	// airspace list
	int shm_airspace;
	void *ptr_airspace;
	std::vector<std::string> airspaceFileNames;

	friend class PSR;

	void printMap(){
		size_t n = sizeof(posX)/sizeof(int);
		//		std::cout << block_count << std::endl;
		if((int) n != 0){
			for(int i=0; i<(int) n; i++){
				map[posX[i]/SCALER][posY[i]/SCALER]++;
			}

			for(int j=0; j<block_count;j++){
				for(int k=0; k<block_count;k++){
					if(map[j][k] == 0)
						printf("_|");
					else
						printf("%d|", map[j][k]);

				}
				std::cout << std::endl;
			}

		}
	}

	void printHeight(){
		size_t n = sizeof(posZ)/sizeof(int);
		if((int) n != 0){
			for(int i =0; i<(int)n;i++){
				std::cout << "Plane " << std::to_string(i+1) <<
						" has at height: " << std::to_string(posZ[i])
						<< std::endl;

			}
		}

	}

};


#endif /* DISPLAY_H_ */
