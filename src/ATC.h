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

#include "Plane.h"
#include "Timer.h"

#define PSR_PERIOD 5000000
#define SSR_PERIOD 5000000

#define SPACE_X_MIN 0
#define SPACE_X_MAX 100000
#define SPACE_Y_MIN 0
#define SPACE_Y_MAX 100000
#define SPACE_Z_MIN 0
#define SPACE_Z_MAX 25000
#define SPACE_ELEVATION 15000

class ATC {
public:
  ATC() {
    initialize();

    start();
  }

  ~ATC() {
    // release all shared memory pointers
  }

  int initialize() {
    // read input from file
    readInput();

    // initialize shm for waiting planes (contains all planes)
    shm_waitingPlanes = shm_open("waiting_planes", O_CREAT | O_RDWR, 0666);
    if (shm_waitingPlanes == -1) {
      perror("in shm_open() ATC: waiting planes");
      exit(1);
    }

    // map shm
    waitingPtr = mmap(0, SIZE_SHM_PSR, PROT_READ | PROT_WRITE, MAP_SHARED,
                      shm_waitingPlanes, 0);
    if (waitingPtr == MAP_FAILED) {
      printf("map failed waiting planes\n");
      return -1;
    }

    // save file descriptors to shm
    int i = 0;
    int j = 0;
    for (Plane *plane : planes) {
      printf("initialize: %s\n", planes.at(j++)->getFD());
      sprintf((char *)waitingPtr + i, "%s ", plane->getFD());
      i += 8;
    }

    // initialize shm for airspace (contains no planes)
    shm_airspace = shm_open("airspace", O_CREAT | O_RDWR, 0666);
    if (shm_airspace == -1) {
      perror("in shm_open() ATC: airspace");
      exit(1);
    }

    // map shm
    airspacePtr = mmap(0, SIZE_SHM_PSR /*change to airspace*/,
                       PROT_READ | PROT_WRITE, MAP_SHARED, shm_airspace, 0);
    if (airspacePtr == MAP_FAILED) {
      printf("map failed airspace\n");
      return -1;
    }

    // create PSR object with number of planes
    PSR *current_psr = new PSR(planes.size());
    psr = current_psr;

    return 0; // set to error code if any
  }

  int start() {

    // start threaded objects
    psr->start();
    for (Plane *plane : planes) {
      plane->start();
    }

    // ============ execution time ============

    // join threaded objects
    for (Plane *plane : planes) {
      plane->stop();
    }

    psr->stop();

    return 0; // set to error code if any
  }

protected:
  int readInput() {
    // open input.txt
    std::string filename = "./input.txt";
    std::ifstream input_file_stream;

    input_file_stream.open(filename);

    if (!input_file_stream) {
      std::cout << "Can't find file input.txt" << std::endl;
      return 1;
    }

    int ID, arrivalTime, arrivalCordX, arrivalCordY, arrivalCordZ,
        arrivalSpeedX, arrivalSpeedY, arrivalSpeedZ;

    std::string separator = " ";

    // parse input.txt to create plane objects
    while (input_file_stream >> ID >> arrivalTime >> arrivalCordX >>
           arrivalCordY >> arrivalCordZ >> arrivalSpeedX >> arrivalSpeedY >>
           arrivalSpeedZ) {

      std::cout << ID << separator << arrivalTime << separator << arrivalCordX
                << separator << arrivalCordY << separator << arrivalCordZ
                << separator << arrivalSpeedX << separator << arrivalSpeedY
                << separator << arrivalSpeedZ << std::endl;

      int pos[3] = {arrivalCordX, arrivalCordY, arrivalCordZ};
      int vel[3] = {arrivalSpeedX, arrivalSpeedY, arrivalSpeedZ};

      // create plane objects and add pointer to each plane to a vector
      Plane *plane = new Plane(ID, arrivalTime, pos, vel);
      planes.push_back(plane);
    }

    int i = 0;
    for (Plane *plane : planes) {
      printf("readinput: %s\n", plane->getFD());
    }

    return 0;
  }

  // ============================ MEMBERS ============================

  // planes
  std::vector<Plane *> planes; // vector of plane objects

  // psr
  PSR *psr;

  // timers
  time_t startTime;
  time_t endTime;

  // shm members
  // waiting planes
  int shm_waitingPlanes;
  void *waitingPtr;

  // airspace
  int shm_airspace;
  void *airspacePtr;

  pthread_mutex_t mutex;
};

#endif /* ATC_H_ */
