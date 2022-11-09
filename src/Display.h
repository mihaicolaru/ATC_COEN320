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
#include <mutex>
#include <errno.h>
#include <sys/siginfo.h>
#include <sys/neutrino.h>

#define SCALER 3000
#define MARGIN 100000
#define PERIOD_D 5000000 //5sec period

const int block_count = (int)MARGIN/(int)SCALER;

//need sporadic task?



//Add vector source library, should be computer system

class Display{
public:
	//constructor
	//int _posX[], int _posY[], int _posZ[]
	Display(){

		// set thread in detached state
		int rc = pthread_attr_init(&attrDisp);
		if (rc){
			printf("ERROR, RC from pthread_attr_init() is %d \n", rc);
		}

		rc = pthread_attr_setdetachstate(&attrDisp, PTHREAD_CREATE_JOINABLE);
		if (rc){
			printf("ERROR; RC from pthread_attr_setdetachstate() is %d \n", rc);
		}

		start();
	}
	//destructor
	~Display(){

	}

	bool start(){
		std::cout << "Start display function\n";
		time(&at);
		return (pthread_create(&displayThread, &attrDisp, updateStart, this) == 0);
	}

	bool stop(){
		pthread_join(displayThread, NULL);
		return 0;
	}

	static void *updateStart(void *context){
		return ((Display *)context)->updateDisplay();
	}

	void *updateDisplay(void){
		//		printf("update display called\n");
		int chid =ChannelCreate(0);
		if(chid == -1){
			std::cout << "couldn't create channel!\n";
		}

		Timer timer(chid);
		timer.setTimer( 1000000, PERIOD_D);
		printMap();

		time(&et);
		double exe = difftime(et,at);
		std::cout << "DISPLAY finished in: " << exe << std::endl;

		ChannelDestroy(chid);
		return 0;

	}


private:
	time_t at;
	time_t et;
	pthread_t displayThread;
	pthread_attr_t attrDisp;
	//	std::ofstream logfile;
	std::mutex mutex_d;
	int posX[5] = {50000,12000,30000,20000,70000};
	int posY[5] = {19000,12000,7000, 50000, 90000};
	int posZ[5] = {12000,15000,11000,10000,10000};
	//	int posX[5];
	//	int posY[5];
	//	int posZ[5];
	int map[block_count][block_count]={{0}}; // Shrink 100k by 100k map to 10 by 10, each block is 10k by 10k
	//	memset(map, 0, sizeof(map[0][0]) * block_count * block_count);


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
