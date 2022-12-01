/*
 * PSR.h
 *
 *  Created on: Oct. 24, 2022
 *      Author: mihai, minhtrannhat
 */

#ifndef PSR_H_
#define PSR_H_

#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#include "Limits.h"
#include "Plane.h"
#include "SSR.h"
#include "Timer.h"

// forward declaration
class Plane;
class SSR;

class PSR {
public:
  PSR(int numberOfPlanes);
  ~PSR();
  void start();
  int stop();
  static void *startPSR(void *context);

private:
  int initialize(int numberOfPlanes);
  void *operatePSR(void);

  // update psr period based on period shm
  void updatePeriod(int chid);

  // ================= read waiting planes buffer =================
  bool readWaitingPlanes();

  // ================= write to flying planes shm =================
  void writeFlyingPlanes();

  // member parameters
  // timer object
  Timer *timer;

  // number of waiting planes left
  int numWaitingPlanes;

  // current period
  int currPeriod;

  // thread members
  pthread_t PSRthread;
  pthread_attr_t attr;
  pthread_mutex_t mutex;

  // timing members
  time_t at;
  time_t et;

  // shm members
  // waiting planes list
  int shm_waitingPlanes;
  void *waitingPlanesPtr;
  std::vector<std::string> waitingFileNames;

  // access waiting planes
  std::vector<void *> planePtrs;

  // airspace list
  int shm_flyingPlanes;
  void *flyingPlanesPtr;
  std::vector<std::string> flyingFileNames;

  // period shm
  int shm_period;
  void *periodPtr;

};

#endif /* PSR_H_ */
