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

class Display{
public:
	//constructor
	Display(){
		initialize();
	}
	//destructor
	~Display(){
		shm_unlink("display");
	}

	//Initialize thread
	int initialize(){
		/*Make threads in detached state*/
		int rc = pthread_attr_init(&attr);
		if (rc){
			printf("ERROR, RC from pthread_attr_init() is %d \n", rc);
		}

		rc = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
		if (rc){
			printf("ERROR; RC from pthread_attr_setdetachstate() is %d \n", rc);
		}


		/*Create shared memory for display*/
		// open list of waiting planes shm
		shm_display= shm_open("display", O_RDWR, 0666);
		if (shm_display == -1) {
			perror("in shm_open() Display");
			exit(1);
		}

		//		printf("finished reading to displayData\n");

		ptr_display = mmap(0, SIZE_SHM_DISPLAY, PROT_READ | PROT_WRITE, MAP_SHARED, shm_display, 0);

		if (ptr_display == MAP_FAILED) {

			perror("in map() Display");
			exit(1);
		}
		//		printf("finished mapping to positionData\n");

	}

	void start(){
		//		std::cout << "Start display function\n";
		if (pthread_create(&displayThread, &attr, &Display::startDisplay, (void *)this) != EOK) {
			displayThread = NULL;
		}
	}

	int stop(){
		pthread_join(displayThread, NULL);
		//		std::cout << "Display terminated\n";
		return 0;
	}

	static void *startDisplay(void *context){
		((Display *)context)->updateDisplay();
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
		while(1){
			if(rcvid==0){
				int axis=0;//0=ID, 1=X, 2=Y, 3=Z, 4=Height display control bit;
				std::string buffer = "";
				std::string x="",y="",z="",display_bit="";
				std::string id ="";
				for (int i = 0; i < SIZE_SHM_DISPLAY; i++) {
					char readChar = *((char *)ptr_display + i);
					if(readChar=='\\'){
						//Wherever has the \ delimiter, ends the reading
						break;
					}else if(readChar == ',' || readChar == ';'){
						if(buffer.length() >0){
//							std::cout << "display buffer: " << buffer << "\n";//Check buffer
							switch(axis){
							case 0:
								id= buffer;
								break;
							case 1:
								x = buffer;
								break;
							case 2:
								y = buffer;
								break;
							case 3:
								z = buffer;
								break;
							case 4:
								display_bit= buffer;
								break;
							}
						}
						if(readChar == ','){
							axis++;
							buffer = "";
						}else if(readChar == ';'){
							//One plane has finished loading, parsing and reset control values
							map[stoi(x)/SCALER][stoi(y)/SCALER]+=id + "\\";
							if(display_bit=="1"){
								height_display = height_display + "Plane " + id + " has height of " + z + "meters\n";
							}
							x="";
							y="";
							z="";
							id="";
							display_bit="";
							axis=0;
							buffer = "";
						}

					}else{
						buffer+=readChar;//Keep loading buffer until delimiter has been detected
						//				printf("Buffer: %s\n",buffer);
					}
					printf("%c", readChar);
				}
				printMap();
				height_display = "";
				memset(map,0, sizeof(map[0][0]) * block_count * block_count);//Reset map to 0 for next set
			}

			rcvid = MsgReceive(chid, &msg, sizeof(msg), NULL);

		}

		ChannelDestroy(chid);
		return 0;

	}

	//Print map and the height
	void printMap(){
		for(int j=0; j<block_count;j++){
			for(int k=0; k<block_count;k++){
				if(map[j][k] == "")
					printf("_|");
				else
					printf("%s|", map[j][k]);

			}
			std::cout << std::endl;
		}
		printf("%s", height_display.c_str());
	}

private:
	//time reader
	time_t at;
	time_t et;

	//threads
	pthread_t displayThread;
	pthread_attr_t attr;
	pthread_mutex_t mutex_d;// mutex for display

	//Temporary values
	std::string map[block_count][block_count]={{""}}; // Shrink 100k by 100k map to 10 by 10, each block is 10k by 10k
	std::string height_display="";

	// shm members
	int shm_display;//Display required info
	void *ptr_display;
};

#endif /* DISPLAY_H_ */
