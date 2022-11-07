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

    ComputerSystem atc;

    atc.stop();

	int arrivalTime1 = 3;
	int ID1 = 1;
	int position1[3] = {90000, 90000, 15000};
	int speed1[3] = {1000, 1000, -1000};

//	int arrivalTime2 = 1;
//	int ID2 = 2;
//	int position2[3] = {90000, 90000, 15000};
//	int speed2[3] = {2000, 500, 1000};
//
//	int arrivalTime3 = 4;
//	int ID3 = 3;
//	int position3[3] = {90000, 90000, 15000};
//	int speed3[3] = {1500, 1400, 950};
//
//	int arrivalTime4 = 1;
//	int ID4 = 4;
//	int position4[3] = {90000, 90000, 15000};
//	int speed4[3] = {2500, 400, 2000};


	Plane plane1(arrivalTime1, ID1, position1, speed1);
//	Plane plane2(arrivalTime2, ID2, position2, speed2);
//	Plane plane3(arrivalTime3, ID3, position3, speed3);
//	Plane plane4(arrivalTime4, ID4, position4, speed4);

	plane1.stop();
//	plane2.stop();
//	plane3.stop();
//	plane4.stop();

	return 0;
}
