#include <iostream>
#include <pthread.h>
#include <stdio.h>
#include <sys/netmgr.h>
#include <sys/neutrino.h>
#include <time.h>
#include <unistd.h>

#include "ComputerSystem.h"

#include "Display.h"
#include "PSR.h"
#include "Plane.h"

#define SIZE 4096

int main() {

	// create shm of planes to display
	int shm_display = shm_open("display", O_CREAT | O_RDWR, 0666);

	ftruncate(shm_display, SIZE);


	// map the memory
	void *ptr = mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_display, 0);
	if (ptr== MAP_FAILED) {
		printf("ptr failed in main\n");
		return -1;
	}

	// figure out your input string to send to display (from computer system)
	std::string inputString = "900,8000,16000,1;8000,30000,17000,0;";

	char arrayString[inputString.length()]="900,8000,16000,1;8000,30000,17000,0;";// dont do this but basically u need a char array the size of the string, needs to be hardcoded

	// save file descriptors to shm
	for (int i = 0; i < sizeof(arrayString); i++) {
		sprintf((char *)ptr + i, "%c", arrayString[i]);// writes inputstring to shm character by character
	}

	Display display(2);
	display.start();
	display.stop();
	return 0;
}
