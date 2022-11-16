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


int main() {

	// create shm of planes to display
	int shm_display = shm_open("display", O_CREAT | O_RDWR, 0666);

	ftruncate(shm_display, SIZE_SHM_PSR);

	// map the memory
	void *ptr = mmap(0, SIZE_SHM_PSR, PROT_READ | PROT_WRITE, MAP_SHARED, shm_planeList, 0);

	// figure out your input string to send to display (from computer system)
	std::string inputString = "";

	char arrayString[inputString.size()];// dont do this but basically u need a char array the size of the string, needs to be hardcoded

	// transfer string to char buffer

	// save file descriptors to shm
	for (int i = 0; i < sizeof(arrayString); i++) {
		sprintf((char *)ptr + i, "%s ", arrayString[i]);// writes inputstring to shm character by character
	}

	Display display(4);
//	display.start();


	return 0;
}
