/*
 * ComputerSystem.h
 *
 *  Created on: Oct. 24, 2022
 *      Author: mihai
 */

#ifndef COMPUTERSYSTEM_H_
#define COMPUTERSYSTEM_H_

#include <vector>
#include <list>
#include <fstream>
#include <time.h>
#include <chrono>
#include "Plane.h"

class ComputerSystem{
public:
	// construcor
	ComputerSystem(){

		initialize();

		start();
	}

	// destructor
	~ComputerSystem(){

	}

	int initialize(){
		// start timer here for comparing with arrival time
		readInput();

		return 0;
	}

	int readInput(){
		std::string filename = "./input.txt";
		std::ifstream input_file_stream;

		input_file_stream.open(filename);

		if (!input_file_stream) {
			std::cout << "Can't find file input.txt" << std::endl;
			return 1;
		}
		int ID, arrivalTime, arrivalCordX, arrivalCordY, arrivalCordZ, arrivalSpeedX, arrivalSpeedY, arrivalSpeedZ;

		std::string separator = " ";

		int pos[3];
		int vel[3];

		time (&at);
		std::cout << "Timer start\n";
		while(input_file_stream >> ID >> arrivalTime >>
				arrivalCordX >> arrivalCordY >> arrivalCordZ >>
				arrivalSpeedX >> arrivalSpeedY >> arrivalSpeedZ){
			std::cout << ID << separator << arrivalTime << separator << arrivalCordX << separator << arrivalCordY << separator << arrivalCordZ << separator << arrivalSpeedX << separator << arrivalCordY << separator << arrivalSpeedZ << std::endl;
			// create variables from inputs


			pos[0] = arrivalCordX;
			pos[1] = arrivalCordY;
			pos[2] = arrivalCordZ;
			vel[0] = arrivalSpeedX;
			vel[1] = arrivalSpeedY;
			vel[2] = arrivalSpeedZ;
			Plane plane(arrivalTime, ID, pos, vel), *p;
			p = &plane;
			planes.push_back(p);
		}
		std::cout << "number of planes: " << planes.size() << std::endl;
		return 0;
	}

	void start(){
		// start scanning planes
	}

	void stop(){
		for(Plane* plane : planes){
			if(!planes.empty()){
				//et = std::chrono::steady_clock::now();
				//std::cout << "Plane: "  << " finished in: " << std::chrono::duration_cast<std::chrono::microseconds>(et-at).count()/CLOCKS_PER_SEC << std::endl;

				plane->stop();
				time (&et);
				double exe = difftime(et,at);
				std::cout << "finished in: " << exe;
			}
		}
	}

	int deployPSR(){
		// run primary radar
		//call clock current
		//at = std::chrono::steady_clock::now();
		//time (&at);
		return 0;
	}

	int deploySSR(){
		// run secondary radar
		return 0;
	}

	int calculateTrajectories(){

		return 0;
	}

private:
	std::list<Plane*> planes;
	std::list<Plane*> airspace;
	//std::chrono::steady_clock::time_point at;
	//std::chrono::steady_clock::time_point et;
	time_t at;
	time_t et;
	int lasIndex;	// index of last plane in airspace
	// plane position matrix
	// ptr to radar (1 and 2)
	// ptr to comm
	// ptr to console
	// ptr to display

};



#endif /* COMPUTERSYSTEM_H_ */
