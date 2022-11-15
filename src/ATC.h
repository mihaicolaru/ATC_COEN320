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

// struct Plane{
//	int ID;
//	int arrivalTime;
//	int position[3];
//	int speed[3];
// };

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
    // initialize shared memory space for each plane
    // save address pointer to planes vector AND readyPlanes vector
    readInput();
    // initialize shared memory space for airspace
    // initialize plane thread attr struc (if exit on end of exec requires
    // different attr) initialize thread attr struct (joinable)
    return 0; // set to error code if any
  }

  int start() {
    // start timer for arrivalTime comparison
    // create thread for each plane with argument casted to void pointer to
    // assign shared memory object to thread create thread for PSR create thread
    // for SSR create thread for ComputerSystem create thread for Display create
    // thread for Console create thread for Comm

    // ============ execution time ============

    // join each plane thread (or have them terminate on exec complete)
    // join PSR
    // join SSR
    // join ComputerSystem
    // join Display
    // join Console
    // join Comm
    return 0; // set to error code if any
  }

  int getNumberOfPlanes() { return this->Planes.size(); }

  friend void *startPlaneUpdate(void *context) {
    // set priority
    return ((ATC *)context)->UpdatePlanePosition();
  }

  static void *startPSR(void *context) {
    // set priority
    return ((ATC *)context)->PSR();
  }

  static void *startSSR(void *context) {
    // set priority
    return ((ATC *)context)->SSR();
  }

  static void *startComputerSystem(void *context) {
    // set priority
    return ((ATC *)context)->ComputerSystem();
  }

  static void *startDisplay(void *context) {
    // set priority
    return ((ATC *)context)->Display();
  }

  static void *startConsole(void *context) {
    // set priority
    return ((ATC *)context)->Console();
  }

  static void *startComm(void *context) {
    // set priority
    return ((ATC *)context)->Comm();
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

    // parse input.txt
    while (input_file_stream >> ID >> arrivalTime >> arrivalCordX >>
           arrivalCordY >> arrivalCordZ >> arrivalSpeedX >> arrivalSpeedY >>
           arrivalSpeedZ) {

      std::cout << ID << separator << arrivalTime << separator << arrivalCordX
                << separator << arrivalCordY << separator << arrivalCordZ
                << separator << arrivalSpeedX << separator << arrivalSpeedY
                << separator << arrivalSpeedZ << std::endl;

      // initialize shared memory space for each plane
      int fd;
      Plane *plane_addr;

      // create new shared memory object
      // std::string planeName = "/plane_" + ID;
      std::string planeName = "plane_" + std::to_string(ID);

      fd = shm_open(planeName.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0777);
      if (fd == -1) {
        fprintf(stderr, "Open plane shared memory object failed: %s\n",
                strerror(errno));
        return EXIT_FAILURE;
      }

      // set memory object size
      if (ftruncate(fd, sizeof(*plane_addr)) == -1) {
        fprintf(stderr, "ftruncate: %s\n", strerror(errno));
        return EXIT_FAILURE;
      }

      // map memory object
      plane_addr = (Plane *)mmap(0, sizeof(*plane_addr), PROT_READ | PROT_WRITE,
                                 MAP_SHARED, fd, 0);
      if (plane_addr == MAP_FAILED) {
        fprintf(stderr, "mmap failed: %s\n", strerror(errno));
        return EXIT_FAILURE;
      }

      // write to shared memory object

      //			plane_addr->ID = ID;
      //			plane_addr->arrivalTime = arrivalTime;
      //			plane_addr->position[0] = arrivalCordX;
      //			plane_addr->position[1] = arrivalCordY;
      //			plane_addr->position[2] = arrivalCordZ;
      //			plane_addr->speed[0] = arrivalSpeedX;
      //			plane_addr->speed[1] = arrivalSpeedY;
      //			plane_addr->speed[2] = arrivalSpeedZ;
      //
      //
      //
      //			std::cout << "Plane: " << plane_addr->ID <<
      //"\narrival: " << plane_addr->arrivalTime <<
      //					"\nposition: " <<
      // plane_addr->position[0] << ", " << plane_addr->position[1] << ", " <<
      // plane_addr->position[2] <<
      //					"\nspeed: " <<
      // plane_addr->speed[0] << ", " << plane_addr->speed[1] << ", " <<
      // plane_addr->speed[2] << "\n";
      //
      //			write(fd, plane_addr, sizeof(*plane_addr));
      //
      //
      //			Plane* plane2;
      //
      //			ssize_t returnSize = read(fd, plane2,
      // sizeof(*plane2));
      //
      //			std::cout << "Plane: " << plane2->ID <<
      //"\narrival: " << plane2->arrivalTime <<
      //					"\nposition: " <<
      // plane2->position[0] << ", " << plane2->position[1] << ", " <<
      // plane2->position[2] <<
      //					"\nspeed: " << plane2->speed[0]
      //<< ", " << plane2->speed[1] << ", " << plane2->speed[2] << "\n";

      // add address of shared memory to planes and waiting planes
      Planes.push_back(plane_addr);
      waitingPlanes.push_back(plane_addr);
    }
  }

  // ============================ PROCESSES ============================

  void *UpdatePlanePosition(void) {
    // update plane position
    // checks for airspace limitations
    // if out of airspace, set plane sm_obj to null, terminate thread
  }

  void *PSR(void) {
    // poll waiting planes
    // if null enter critical section: remove plane
    // else if t_current > t_arrival enter critical section:
    // copy then remove element waiting planes.at(i)
    // append to end of airspace
    // leave critical section
    // when waiting planes queue is empty terminate thread
  }

  void *SSR(void) {
    // poll airspace
    // read ID, pos, vel
    // create entry in airspace sm_obj
    // once all planes polled, notify computer system (msgsend)
    // when plane found to be null enter critical section:
    // remove plane
    // leave critical section
    // when waiting planes AND airspace queues are empty terminate thread
  }

  void *ComputerSystem(void) {
    // when notified, parse airspace sm_obj
    // check for airspace violations within [t, t+3min], notify if found
    // populate map for display (maybe lock critical section), send command if
    // info needed when command received, start interrupt routine related to
    // command
  }

  void *Display(void) {
    // print airspace from map value every 5 seconds
    // if command from computer exists, update next print with necessary info
  }

  void *Console(void) {
    // generate interrupt when keyboard input detected or needed
    // fill buffer with keyboard input until line termination
    // parse command, send to computerSystem
  }

  void *Comm(void) {
    // when notified by computerSystem, sends message m to plane R
  }

  // ============================ MEMBERS ============================

  // plane queues
  std::vector<Plane *> Planes; // all planes shared memory address pointers
  std::vector<Plane *> waitingPlanes; // planes not in airspace
  std::vector<Plane *> airspace;      // planes in airspace

  // timers
  time_t startTime;
  time_t endTime;

  // threads
  std::vector<pthread_t> planeThreads;
  pthread_t primaryRadar;
  pthread_t secondaryRadar;
  pthread_t computerSystem;
  pthread_t display;
  pthread_t console;
  pthread_t comm;

  // thread attributes
  pthread_attr_t detached; // attribute struct for detached threads
};

#endif /* ATC_H_ */
