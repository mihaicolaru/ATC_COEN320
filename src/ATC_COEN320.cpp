#include <iostream>
#include <stdio.h>
#include <time.h>
#include <sys/netmgr.h>
#include <sys/neutrino.h>
#include <pthread.h>
#include <unistd.h>


#include "ComputerSystem.h"

#include "Plane.h"


int main() {
	time_t arrivalTime;
	int ID = 1;
	int position[3] = {1, 1, 1};
	int speed[3] = {1, 1, 1};

	Plane plane1(arrivalTime, ID, position, speed);

	plane1.start();


	plane1.stop();

	return 0;
}
