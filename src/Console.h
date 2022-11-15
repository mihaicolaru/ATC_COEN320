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


class Console{
public:

	Console(){
		parseCommand();
	}

	~Console(){

	}
	// Test to add reading command from cmd
	//	std::string exec(const char* cmd) {
	//	    std::array<char, 128> buffer;
	//	    std::string result;
	//	    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
	//	    if (!pipe) {
	//	        throw std::runtime_error("popen() failed!");
	//	    }
	//	    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
	//	        result += buffer.data();
	//	    }
	//	    return result;
	//	}

	void parseCommand(){

		CommandState state = Selection;


		while(1){
			switch(state){
				case Selection:
					printf("Enter yes to select Violation Checker\n");
					std::cin >> buffer;
					printf(buffer);
					printf("\n");
					std::cout << buffer << std::endl;
					if(strcmp(buffer, "")==0){
						printf("It's empty\n");
					}
					else if(strcmp(buffer,"yes") == 0){
						state = ReadCommand;
						break;
					}else{
						printf("Invalid Input\n");
						break;
					}
				case ReadCommand:
					printf("Enter number for n\n");
					std::cin >> buffer;
					if(arrayDigitCheck(buffer)){
						printf("Prepare to send command\n");
						state = Send;
						break;
					}else{
						printf("Invalid Input\n");
						state = Selection;
						break;
					}
				case Send:
					printf("Send command\n");
					break;
			}
		}

		// compare given command to list of known commands
		//Keyboard input -> buffer
		//Generate interrupt (or send signal)
		// (show what u inputted)
		//Another timer

		//inf loop
		// wait for keyboard input
		// if there is input with \n in buffer
		// do smt, clear buffer
		// send msg
		// go back to loop
	}


	int bufferParser(){
		//
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
