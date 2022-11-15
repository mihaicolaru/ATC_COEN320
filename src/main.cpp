#include <iostream>
#include <stdio.h>
#include <time.h>
#include <sys/netmgr.h>
#include <sys/neutrino.h>
#include <pthread.h>
#include <unistd.h>


#include "ComputerSystem.h"
#include "ATC.h"

#include "Plane.h"
#include "Display.h"
#include "PSR.h"

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

	std::vector<Plane*> planes;

	planes.push_back(&plane1);
	planes.push_back(&plane2);
	planes.push_back(&plane3);
	planes.push_back(&plane4);

	//	std::cout << planes.at(0)->getFD() << "\n";

	int shm_planeList = shm_open("waiting_planes", O_CREAT | O_RDWR, 0666);


	ftruncate(shm_planeList, 4096);

	void* ptr = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, shm_planeList, 0);



	int i = 0;
	for(Plane* plane : planes){

		sprintf((char*)ptr + i, "%s ", plane->getFD());
		i += 8;
//		const char* fileDes = plane->getFD();
//		for(int j = 0; j < 8; j++){
//			sprintf(((char*)ptr + j + i*8), "%s", fileDes[j]);
//		}
//		i++;
	}

	//	std::shared_ptr<std::vector<Plane*>> sharedPlanes = std::make_shared<std::vector<Plane*>> (std::move(planes));

	PSR psr(sizeof(planes));
	psr.start();

	//	time (&at);
	plane1.start();

	plane1.stop();
	//	plane2.stop();
	//	plane3.stop();
	//	plane4.stop();

	//	psr.stop();
	//	time (&et);
	//	double exe = difftime(et,at);
	//	std::cout << "finished in: " << exe << "\n";
	return 0;
}
