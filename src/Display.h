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

#include "Limits.h"

const int block_count = (int)MARGIN/(int)SCALER;

//need sporadic task?
//Add vector source library, should be computer system

class Display{
public:
	//constructor
	//int _posX[], int _posY[], int _posZ[]
	Display(int planeCount){
		nbOfPlanes = planeCount;
		initialize();
	}
	//destructor
	~Display(){
		shm_unlink("display");
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
//		printf("shm section\n");
//		printf("%i\n", nbOfPlanes);

		shm_displayData= shm_open("display", O_RDWR, 0666);
		if (shm_displayData == -1) {
			perror("in shm_open() Display");
			exit(1);
		}
//		printf("finished reading to displayData\n");

		ptr_positionData = mmap(0, SIZE_SHM_DISPLAY, PROT_READ | PROT_WRITE, MAP_SHARED, shm_displayData, 0);

		if (ptr_positionData == MAP_FAILED) {

			perror("in map() Display");
			exit(1);
		}
//		printf("finished mapping to positionData\n");


		int axis=0;//0=X, 1=Y, 2=Z;
		std::string buffer = "";
		int planeNb=0;
		// read waiting planes shm
		for (int i = 0; i < (nbOfPlanes * 20); i++) {
			char readChar = *((char *)ptr_positionData + i);
			if(readChar == ',' || readChar == ';'){
				if(buffer.length() >0){
					std::cout << "display buffer: " << buffer << "\n";
					switch(axis){
					case 0:
						posX[planeNb]= stoi(buffer);
						break;
					case 1:
						posY[planeNb]= stoi(buffer);
						break;
					case 2:
						posZ[planeNb]= stoi(buffer);
						break;
					case 3:
						displayZ[planeNb]= stoi(buffer);
						break;
					}
				}
				if(readChar == ','){
					axis++;
					buffer = "";
					//					printf("Axis: %i\n",axis);
				}else if(readChar == ';'){
					axis=0;
					planeNb++;
					buffer = "";
					//					printf("Axis: %i\n",axis);
				}

			}else{
				buffer+=readChar;
				//				printf("Buffer: %s\n",buffer);
			}
		}
	}

	int start(){
//		std::cout << "Start display function\n";
		if(pthread_create(&displayThread, &attr, &Display::updateStart, (void *) this) != EOK){
			displayThread = NULL;
		}
		return 0;
	}

	bool stop(){
		pthread_join(displayThread, NULL);
//		std::cout << "Display terminated\n";
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

		int i = 1;
		while(i-- > 0){
//			printMap();
//			printHeight();
			rcvid = MsgReceive(chid, &msg, sizeof(msg), NULL);
		}

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
	//	int posX[5] = {50000,12000,30000,20000,70000};
	//	int posY[5] = {19000,12000,7000, 50000, 90000};
	//	int posZ[5] = {12000,15000,11000,10000,10000};

	//size not dynamic
	int posX[5];
	int posY[5];
	int posZ[5];
	int displayZ[5];
	int map[block_count][block_count]={{0}}; // Shrink 100k by 100k map to 10 by 10, each block is 10k by 10k

	int nbOfPlanes=0;

	// shm members
	int shm_displayData;//Display required info
	void *ptr_positionData;


	void printMap(){
		size_t n = sizeof(posX)/sizeof(int);
		memset(map, 0, sizeof(map[0][0]) * block_count * block_count);//Reset to 0 for next set
		if(nbOfPlanes != 0){
			for(int i=0; i<nbOfPlanes; i++){
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
		if(nbOfPlanes  != 0){
			for(int i =0; i<nbOfPlanes;i++){
				if(displayZ[i]==1){
					std::cout << "Plane " << std::to_string(i+1) <<
							" has at height: " << std::to_string(posZ[i])
							<< std::endl;
				}

			}
		}
	}
};

#endif /* DISPLAY_H_ */
