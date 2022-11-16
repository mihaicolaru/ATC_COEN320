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
	time_t at;
	time_t et;
	//	ATC atc;

	int arrivalTime1 = 3;
	int ID1 = 1;
	int position1[3] = {90000, 90000, 15000};
	int speed1[3] = {1000, 1000, 0};

	int arrivalTime2 = 1;
	int ID2 = 2;
	int position2[3] = {90000, 90000, 15000};
	int speed2[3] = {2000, 500, 0};

	int arrivalTime3 = 4;
	int ID3 = 3;
	int position3[3] = {90000, 90000, 15000};
	int speed3[3] = {1500, 1400, 0};

	int arrivalTime4 = 1;
	int ID4 = 4;
	int position4[3] = {90000, 90000, 15000};
	int speed4[3] = {2500, 400, 0};

	Plane plane1(arrivalTime1, ID1, position1, speed1);
	Plane plane2(arrivalTime2, ID2, position2, speed2);
	Plane plane3(arrivalTime3, ID3, position3, speed3);
	Plane plane4(arrivalTime4, ID4, position4, speed4);

	std::vector<Plane *> planes;

	planes.push_back(&plane1);
	planes.push_back(&plane2);
	planes.push_back(&plane3);
	planes.push_back(&plane4);

	// create shm of waiting planes
	int shm_planeList = shm_open("waiting_planes", O_CREAT | O_RDWR, 0666);

	ftruncate(shm_planeList, SIZE_SHM_PSR);

	// map the memory
	void *ptr = mmap(0, SIZE_SHM_PSR, PROT_READ | PROT_WRITE, MAP_SHARED, shm_planeList, 0);

	// save file descriptors to shm
	int i = 0;
	for (Plane *plane : planes) {
		sprintf((char *)ptr + i, "%s ", plane->getFD());
		i += 8;
	}

	PSR psr(planes.size());
	Display display(planes.size());
	psr.start();
	display.start();
	for(Plane * plane : planes){
		plane->start();
	}

	for(Plane * plane : planes){
		plane->stop();
	}

	//	time (&at);

	psr.stop();
	//	time (&et);
	//	double exe = difftime(et,at);
	//	std::cout << "finished in: " << exe << "\n";
	return 0;
}
