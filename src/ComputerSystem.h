/*
 * ComputerSystem.h
 *
 *  Created on: Oct. 24, 2022
 *      Author: mihai
 */

#ifndef COMPUTERSYSTEM_H_
#define COMPUTERSYSTEM_H_

#include <vector>
#include <fstream>
#include <time.h>

#include "Plane.h"

class ComputerSystem{
public:
	// construcor
	ComputerSystem(){
		initialize();
	}

	// destructor
	~ComputerSystem(){

	}

	int initialize(){
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

		while(input_file_stream >> ID >> arrivalTime >>
				arrivalCordX >> arrivalCordY >> arrivalCordZ >>
				arrivalSpeedX >> arrivalSpeedY >> arrivalSpeedZ){
			std::cout << ID << separator << arrivalTime << separator << arrivalCordX << separator << arrivalCordY << separator << arrivalCordZ << separator << arrivalSpeedX << separator << arrivalCordY << separator << arrivalSpeedZ << std::endl;
			// create variables from inputs

		}

		return 0;
	}

	void start(){

	}

	int deployPSR(){
		// run primary radar
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
	std::vector<Plane> planes;
	std::vector<Plane> airspace;

	// plane position matrix
	// ptr to radar (1 and 2)
	// ptr to comm
	// ptr to console
	// ptr to display

};



#endif /* COMPUTERSYSTEM_H_ */
