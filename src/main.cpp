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
#include "ATC.h"

#define SIZE_DISPLAY 4096

#define SIZE 4096

int main() {

	ATC atc;
	atc.start();

	return 0;
}
