/*
 * Display.h
 *
 *  Created on: Oct. 24, 2022
 *      Author: mihai
 */

#ifndef DISPLAY_H_
#define DISPLAY_H_

#define SCALER 10000

#include <stdio.h>
#include <time.h>

#include "Plane.h"
//Add vector source library, should be computer system

class Display{
public:
	//constructor
	Display(){
		test();
	}
	//destructor
	~Display(){

	}


private:
	int posX[5] = {10000,12000,30000,20000,70000};
	int posY[5] = {10000,12000,7000, 50000, 90000};
	int posZ[5] = {12000,15000,11000,10000,10000};
	int map[10][10] = {
			{0,0,0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0,0,0}}; // Shrink 100k by 100k map to 10 by 10, each block is 10k by 10k

	void test(){
		size_t n = sizeof(posX)/sizeof(int);
		for(int i=0; i<n; i++){
			map[posX[i]/SCALER][posY[i]/SCALER]++;
		}

		for(int j=0; j<10;j++){
			for(int k=0; k<10;k++){
				printf("%d ", map[j][k]);
			}
			std::cout << std::endl;
		}
	}

};


#endif /* DISPLAY_H_ */
