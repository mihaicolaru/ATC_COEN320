/*
 * Console.h
 *
 *  Created on: Oct. 24, 2022
 *      Author: mihai
 */

#ifndef CONSOLE_H_
#define CONSOLE_H_


#include <iostream>
#include <stdio.h>
#include <string>

#include "Limits.h"


class Console{
public:

	Console(){

	}

	~Console(){

	}




	int readCommand(){
		std::string filename = "./command.txt";
		std::ifstream input_file_stream;

		input_file_stream.open(filename);

		if (!input_file_stream) {
			std::cout << "Can't find file input.txt" << std::endl;
			return 1;
		}
	}

private:
	enum CommandState{
		Selection,
		ReadCommand,
		Send
	};

	char buffer[30];//Needs to be char if reading from keyboard direction
	/* Keyboard input idea:
	 * 1- Read directly from keyboard, but no way to see it
	 * 2- Write on console, needs to create interrupt, mutex, and able to
	 * 		differentiate from other displaying outputs
	 * 3- Terminal, have not figure out a way to do it.
	 * 		1 possible solution is from cmd.
	 * */

	bool arrayDigitCheck(char numArray[])
	{
		int i;
		const int len = strlen(numArray);

		for (i = 0; i < len; i++)
		{

			if (!isdigit(numArray[i])) return false;
		}
		return true;
	}

	// This is to control the input.
	// Needs to add sequence for each command




	//Input n second ahead of time and check for collision
	//Request plane to change altitude, speed and position

	//Ignore for now
	//Transmission send(R,m) R=plane, m=command


};


#endif /* CONSOLE_H_ */
