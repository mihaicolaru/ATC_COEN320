/*
 * ATC.h
 *
 *  Created on: Nov. 12, 2022
 *      Author: Mihai
 */

#ifndef ATC_H_
#define ATC_H_

#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <list>
#include <pthread.h>
#include <stdio.h>
#include <string>
#include <sys/mman.h>
#include <sys/neutrino.h>
#include <sys/siginfo.h>
#include <time.h>
#include <vector>

#include "ComputerSystem.h"
#include "Display.h"
#include "Limits.h"
#include "PSR.h"
#include "Plane.h"
#include "SSR.h"
#include "Timer.h"

class ATC {
public:
  ATC();
  ~ATC();
  int start();

protected:
  int readInput();
  int initialize();

  // ============================ MEMBERS ============================

  // planes
  std::vector<Plane *> planes; // vector of plane objects

  // primary radar
  PSR *psr;

  // secondary radar
  SSR *ssr;

  // display
  Display *display;

  // computer system
  ComputerSystem *computerSystem;

  // timers
  time_t startTime;
  time_t endTime;

  // shm members
  // waiting planes
  int shm_waitingPlanes;
  void *waitingPtr;

  // flying planes
  int shm_flyingPlanes;
  void *flyingPtr;

  // shm airspace
  int shm_airspace;
  void *airspacePtr;

  // shm period
  int shm_period;
  void *periodPtr;

  // shm display
  int shm_display;
  void *displayPtr;

  pthread_mutex_t mutex;

};

#endif /* ATC_H_ */
