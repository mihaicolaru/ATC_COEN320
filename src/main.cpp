#include <iostream>
#include <pthread.h>
#include <stdio.h>
#include <sys/netmgr.h>
#include <sys/neutrino.h>
#include <time.h>
#include <unistd.h>

#include "ComputerSystem.h"

#include "ATC.h"
#include "Display.h"
#include "PSR.h"
#include "SSR.h"
#include "Plane.h"

int main() {
  time_t at;
  time_t et;
  ATC atc;
  atc.start();


  return 0;
}
