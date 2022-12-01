#include "ComputerSystem.h"

// contructor
ComputerSystem::ComputerSystem(int numberOfPlanes) {
  numPlanes = numberOfPlanes;
  currPeriod = CS_PERIOD;
  initialize();
}

// destructor
ComputerSystem::~ComputerSystem() {
  shm_unlink("airspace");
  shm_unlink("display");
  for (std::string name : commNames) {
    shm_unlink(name.c_str());
  }
  pthread_mutex_destroy(&mutex);

  delete (timer);
}

// start computer system
void ComputerSystem::start() {
  //		std::cout << "computer system start called\n";
  if (pthread_create(&computerSystemThread, &attr,
                     &ComputerSystem::startComputerSystem,
                     (void *)this) != EOK) {
    computerSystemThread = 0;
  }
}

// join computer system thread
int ComputerSystem::stop() {
  //		std::cout << "computer system stop called\n";
  pthread_join(computerSystemThread, NULL);
  return 0;
}

// entry point for execution function
void *ComputerSystem::startComputerSystem(void *context) {
  ((ComputerSystem *)context)->calculateTrajectories();

  return NULL;
}

int ComputerSystem::initialize() {
  // initialize thread members

  // set threads in detached state
  int rc = pthread_attr_init(&attr);
  if (rc) {
    printf("ERROR, RC from pthread_attr_init() is %d \n", rc);
  }

  rc = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  if (rc) {
    printf("ERROR; RC from pthread_attr_setdetachstate() is %d \n", rc);
  }

  // shared memory members

  // shared memory from ssr for the airspace
  shm_airspace = shm_open("airspace", O_RDONLY, 0666);
  if (shm_airspace == -1) {
    perror("in compsys shm_open() airspace");
    exit(1);
  }

  airspacePtr =
      mmap(0, SIZE_SHM_AIRSPACE, PROT_READ, MAP_SHARED, shm_airspace, 0);
  if (airspacePtr == MAP_FAILED) {
    perror("in compsys map() airspace");
    exit(1);
  }

  // open period shm
  shm_period = shm_open("period", O_RDWR, 0666);
  if (shm_period == -1) {
    perror("in shm_open() PSR: period");
    exit(1);
  }

  // map period shm
  periodPtr = mmap(0, SIZE_SHM_PERIOD, PROT_READ | PROT_WRITE, MAP_SHARED,
                   shm_period, 0);
  if (periodPtr == MAP_FAILED) {
    perror("in map() PSR: period");
    exit(1);
  }

  // open shm display
  shm_display = shm_open("display", O_RDWR, 0666);
  if (shm_display == -1) {
    perror("in compsys shm_open() display");
    exit(1);
  }

  // map display shm
  displayPtr = mmap(0, SIZE_SHM_DISPLAY, PROT_READ | PROT_WRITE, MAP_SHARED,
                    shm_display, 0);
  if (displayPtr == MAP_FAILED) {
    perror("in copmsys map() display");
    exit(1);
  }

  // open comm shm
  int shm_comm = shm_open("waiting_planes", O_RDONLY, 0666);
  if (shm_comm == -1) {
    perror("in compsys shm_open() comm");
    exit(1);
  }

  // map comm shm
  void *commPtr = mmap(0, SIZE_SHM_PSR, PROT_READ, MAP_SHARED, shm_comm, 0);
  if (commPtr == MAP_FAILED) {
    perror("in compsys map() comm");
    exit(1);
  }

  // generate pointers to plane shms
  std::string buffer = "";

  for (int i = 0; i < SIZE_SHM_PSR; i++) {
    char readChar = *((char *)commPtr + i);

    if (readChar == ',') {
      //				std::cout << "compsys initialize() found
      // a planeFD: " << FD_buffer << "\n";

      // open shm for current plane
      int shm_plane = shm_open(buffer.c_str(), O_RDWR, 0666);
      if (shm_plane == -1) {
        perror("in compsys shm_open() plane");

        exit(1);
      }

      // map memory for current plane
      void *ptr = mmap(0, SIZE_SHM_PLANES, PROT_READ | PROT_WRITE, MAP_SHARED,
                       shm_plane, 0);
      if (ptr == MAP_FAILED) {
        perror("in compsys map() plane");
        exit(1);
      }

      commPtrs.push_back(ptr);
      commNames.push_back(buffer);

      buffer = "";
      continue;
    } else if (readChar == ';') {
      // open shm for current plane
      int shm_plane = shm_open(buffer.c_str(), O_RDWR, 0666);
      if (shm_plane == -1) {
        perror("in compsys shm_open() plane");

        exit(1);
      }

      // map memory for current plane
      void *ptr = mmap(0, SIZE_SHM_PLANES, PROT_READ | PROT_WRITE, MAP_SHARED,
                       shm_plane, 0);
      if (ptr == MAP_FAILED) {
        perror("in compsys map() plane");
        exit(1);
      }

      commPtrs.push_back(ptr);
      commNames.push_back(buffer);

      break;
    }

    buffer += readChar;
  }

  return 0;
}

void *ComputerSystem::calculateTrajectories() {
  // create channel to communicate with timer
  int chid = ChannelCreate(0);
  if (chid == -1) {
    std::cout << "couldn't create channel\n";
  }

  Timer *newTimer = new Timer(chid);
  timer = newTimer;
  timer->setTimer(CS_PERIOD, CS_PERIOD);

  int rcvid;
  Message msg;

  bool done = false;
  while (1) {
    if (rcvid == 0) {

      pthread_mutex_lock(&mutex);

      // read airspace shm
      done = readAirspace();

      // prune predictions
      cleanPredictions();

      // compute airspace violations for all planes in the airspace
      computeViolations();

      // send airspace info to display / prune airspace info
      writeToDisplay();

      // update the period based on the traffic
      updatePeriod(chid);

      // unlock mutex
      pthread_mutex_unlock(&mutex);
    }

    // termination check
    if (numPlanes <= 0 || done) {
      std::cout << "computer system done\n";
      sprintf((char *)displayPtr, "terminated");
      ChannelDestroy(chid);
      return 0;
    }

    rcvid = MsgReceive(chid, &msg, sizeof(msg), NULL);
  }

  ChannelDestroy(chid);

  return 0;
}

bool ComputerSystem::readAirspace() {
  //		std::cout << "before read airspace\n";
  std::string readBuffer = "";
  int j = 0;

  //				std::cout << "compsys current planes:\n";
  //				for(aircraft *craft : flyingPlanesInfo){
  //					std::cout << "plane " << craft->id << ",
  // keep:
  //"
  //<< craft->keep
  //<< "\n";
  //				}
  //
  //				printf("computer system read airspace: %s\n",
  // ptr_airspace);

  // variable buffer, these get overwritten as needed
  int id, arrTime, pos[3], vel[3];

  // find planes in airspace
  for (int i = 0; i < SIZE_SHM_AIRSPACE; i++) {
    char readChar = *((char *)airspacePtr + i);
    if (readChar == 't') {
      return true;
    }
    // end of airspace shm, line termination found
    if (readChar == ';') {
      // i=0 no planes found
      if (i == 0) {
        //					std::cout << "compsys no flying
        // planes\n";
        break;
      }

      // load last field in buffer
      vel[2] = atoi(readBuffer.c_str());
      //				std::cout << "compsys found last plane "
      //<< id
      //<< ", checking if already in list\n";

      // check if already in airspace, if yes update with current data
      bool inList = false;
      for (aircraft *craft : flyingPlanesInfo) {
        //					std::cout << "target plane id: "
        //<< id
        //<<
        //"\n"; 					std::cout << "current
        // plane:
        //"
        //<< craft->id << "\n";

        if (craft->id == id) {
          //						std::cout << "compsys:
          // plane
          //"
          //<< craft->id
          //<< " already in list, updating new values\n";
          // if found, update with current info
          craft->pos[0] = pos[0];
          craft->pos[1] = pos[1];
          craft->pos[2] = pos[2];
          craft->vel[0] = vel[0];
          craft->vel[1] = vel[1];
          craft->vel[2] = vel[2];
          // if found, keep, if not will be removed
          craft->keep = true;
          // it is already in the list, do not add new
          inList = true;

          //						std::cout << "checking
          // plane
          //"
          //<< craft->id
          //<< " predictions\n";

          for (trajectoryPrediction *prediction : trajectoryPredictions) {
            if (prediction->id == craft->id) {
              //								std::cout
              //<< "prediction for plane " << craft->id << " found\n";

              // if end of prediction reached, break
              if (prediction->t >= prediction->posX.size() ||
                  prediction->t >= prediction->posY.size() ||
                  prediction->t >= prediction->posZ.size()) {
                //									std::cout
                //<< "plane
                //"
                //<< prediction->id << ": prediction already at end of
                // scope\n";
                break;
              }

              // check posx, posy and poz if same
              if (prediction->posX.at(prediction->t) == craft->pos[0] &&
                  prediction->posY.at(prediction->t) == craft->pos[1] &&
                  prediction->posZ.at(prediction->t) == craft->pos[2]) {
                //									std::cout
                //<< "predictions match\n";
                // set prediction index to next
                // update last entry in predictions
                auto it = prediction->posX.end();
                it -= 2;
                int lastX = *it + (currPeriod / 1000000) * craft->vel[0];
                it = prediction->posY.end();
                it -= 2;
                int lastY = *it + (currPeriod / 1000000) * craft->vel[1];
                it = prediction->posZ.end();
                it -= 2;
                int lastZ = *it + (currPeriod / 1000000) * craft->vel[2];

                // check if still within limits
                bool outOfBounds = false;
                if (lastX > SPACE_X_MAX || lastX < SPACE_X_MIN) {
                  outOfBounds = true;
                }
                if (lastY > SPACE_Y_MAX || lastY < SPACE_Y_MIN) {
                  outOfBounds = true;
                }
                if (lastZ > SPACE_Z_MAX || lastZ < SPACE_Z_MIN) {
                  outOfBounds = true;
                }
                // if not, just increment
                if (outOfBounds) {
                  //										std::cout
                  //<< "plane
                  //"
                  //<< prediction->id << " prediction already reaches end of
                  // airspace\n";
                  prediction->keep = true;
                  prediction->t++;
                  break;
                }

                // add new last prediction
                prediction->posX.pop_back();
                prediction->posX.push_back(lastX);
                prediction->posX.push_back(-1);
                prediction->posY.pop_back();
                prediction->posY.push_back(lastY);
                prediction->posY.push_back(-1);
                prediction->posZ.pop_back();
                prediction->posZ.push_back(lastZ);
                prediction->posZ.push_back(-1);

                // keep for computation and increment index
                prediction->keep = true;
                prediction->t++;
              }
              // update the prediction
              else {
                //									std::cout
                //<< "predictions do not match, recomputing predictions\n";
                prediction->posX.clear();
                prediction->posY.clear();
                prediction->posZ.clear();

                for (int i = 0; i < (180 / (currPeriod / 1000000)); i++) {
                  int currX = craft->pos[0] +
                              i * (currPeriod / 1000000) * craft->vel[0];
                  int currY = craft->pos[1] +
                              i * (currPeriod / 1000000) * craft->vel[1];
                  int currZ = craft->pos[2] +
                              i * (currPeriod / 1000000) * craft->vel[2];

                  prediction->posX.push_back(currX);
                  prediction->posY.push_back(currY);
                  prediction->posZ.push_back(currZ);

                  bool outOfBounds = false;
                  if (currX > SPACE_X_MAX || currX < SPACE_X_MIN) {
                    outOfBounds = true;
                  }
                  if (currY > SPACE_Y_MAX || currY < SPACE_Y_MIN) {
                    outOfBounds = true;
                  }
                  if (currZ > SPACE_Z_MAX || currZ < SPACE_Z_MIN) {
                    outOfBounds = true;
                  }
                  if (outOfBounds) {
                    //													std::cout
                    //<< "end of plane " << prediction->id << " prediction\n";
                    break;
                  }
                }
                // set termination character
                prediction->posX.push_back(-1);
                prediction->posY.push_back(-1);
                prediction->posZ.push_back(-1);
                // set prediction index to next, keep for computation
                prediction->t = 1;
                prediction->keep = true;
              }
            }
          }
          //								std::cout
          //<< "compsys plane
          //"
          //<< craft->id
          //<< " new values:\n"
          //										<<
          //"posx
          //"
          //<< craft->pos[0]
          //<<
          //", posy
          //"
          //<< craft->pos[1] << ", posz " << craft->pos[2] << "\n"
          //										<<
          //"velx
          //"
          //<< craft->vel[0]
          //<<
          //", vely
          //"
          //<< craft->vel[1] << ", velz " << craft->vel[2] << "\n"
          //										<<
          //"set keep value to:
          //"
          //<< craft->keep
          //<<
          //"\n";
          break;
        }
      }

      // if plane was not already in the list, add it
      if (!inList) {
        //					std::cout << "compsys found new
        // plane
        //"
        //<< id
        //<<
        //"\n";
        // new pointer to struct, set members from read
        aircraft *currentAircraft = new aircraft();
        currentAircraft->id = id;
        currentAircraft->t_arrival = arrTime;
        currentAircraft->pos[0] = pos[0];
        currentAircraft->pos[1] = pos[1];
        currentAircraft->pos[2] = pos[2];
        currentAircraft->vel[0] = vel[0];
        currentAircraft->vel[1] = vel[1];
        currentAircraft->vel[2] = vel[2];
        currentAircraft->keep = true; // keep for first computation
        flyingPlanesInfo.push_back(
            currentAircraft); // add to struct pointer vector

        //					std::cout << "computing new
        // predictions for plane " << currentAircraft->id << "\n";
        trajectoryPrediction *currentPrediction = new trajectoryPrediction();

        currentPrediction->id = currentAircraft->id;

        for (int i = 0; i < 180 / (currPeriod / 1000000); i++) {
          int currX = currentAircraft->pos[0] +
                      i * (currPeriod / 1000000) * currentAircraft->vel[0];
          int currY = currentAircraft->pos[1] +
                      i * (currPeriod / 1000000) * currentAircraft->vel[1];
          int currZ = currentAircraft->pos[2] +
                      i * (currPeriod / 1000000) * currentAircraft->vel[2];

          currentPrediction->posX.push_back(currX);
          currentPrediction->posY.push_back(currY);
          currentPrediction->posZ.push_back(currZ);

          bool outOfBounds = false;
          if (currX > SPACE_X_MAX || currX < SPACE_X_MIN) {
            outOfBounds = true;
          }
          if (currY > SPACE_Y_MAX || currY < SPACE_Y_MIN) {
            outOfBounds = true;
          }
          if (currZ > SPACE_Z_MAX || currZ < SPACE_Z_MIN) {
            outOfBounds = true;
          }
          if (outOfBounds) {
            //							std::cout
            //<< "end of plane " << currentPrediction->id << " predictions\n";
            break;
          }
        }
        // set termination character
        currentPrediction->posX.push_back(-1);
        currentPrediction->posY.push_back(-1);
        currentPrediction->posZ.push_back(-1);
        // set prediction index to next, keep for first computation
        currentPrediction->t = 1;
        currentPrediction->keep = true;
        // add to list
        trajectoryPredictions.push_back(currentPrediction);

        break;
      }
      break;
    }
    // found plane in airspace (plane termination character)
    else if (readChar == '/') {
      // load last value in buffer
      vel[2] = atoi(readBuffer.c_str());
      //				std::cout << "compsys found next plane "
      //<< id
      //<< ", checking if already in list\n";

      // check if already in airspace, if yes update with current data
      bool inList = false;
      for (aircraft *craft : flyingPlanesInfo) {
        //					std::cout << "target plane id: "
        //<< id
        //<<
        //"\n"; 					std::cout << "current
        // plane:
        //"
        //<< craft->id << "\n";

        if (craft->id == id) {
          //						std::cout << "compsys:
          // plane
          //"
          //<< craft->id
          //<< " already in list, updating new values\n";

          // if found, update with current info
          craft->pos[0] = pos[0];
          craft->pos[1] = pos[1];
          craft->pos[2] = pos[2];
          craft->vel[0] = vel[0];
          craft->vel[1] = vel[1];
          craft->vel[2] = vel[2];
          // if found, keep, if not will be removed
          craft->keep = true;
          // if already in list, do not add new
          inList = true;

          //						std::cout << "checking
          // plane
          //"
          //<< craft->id
          //<< " predictions\n";

          for (trajectoryPrediction *prediction : trajectoryPredictions) {
            if (prediction->id == craft->id) {
              //								std::cout
              //<< "prediction for plane " << craft->id << " found\n";

              // if end of prediction reached, break
              if (prediction->t >= prediction->posX.size() ||
                  prediction->t >= prediction->posY.size() ||
                  prediction->t >= prediction->posZ.size()) {
                //									std::cout
                //<< "plane
                //"
                //<< prediction->id << ": prediction already at end of
                // scope\n";
                break;
              }

              // check posx, posy and poz if same
              if (prediction->posX.at(prediction->t) == craft->pos[0] &&
                  prediction->posY.at(prediction->t) == craft->pos[1] &&
                  prediction->posZ.at(prediction->t) == craft->pos[2]) {
                //									std::cout
                //<< "predictions match\n";
                // set prediction index to next
                // update last entry in predictions
                auto it = prediction->posX.end();
                it -= 2;
                int lastX = *it + (currPeriod / 1000000) * craft->vel[0];
                it = prediction->posY.end();
                it -= 2;
                int lastY = *it + (currPeriod / 1000000) * craft->vel[1];
                it = prediction->posZ.end();
                it -= 2;
                int lastZ = *it + (currPeriod / 1000000) * craft->vel[2];

                // check if still within limits
                bool outOfBounds = false;
                if (lastX > SPACE_X_MAX || lastX < SPACE_X_MIN) {
                  outOfBounds = true;
                }
                if (lastY > SPACE_Y_MAX || lastY < SPACE_Y_MIN) {
                  outOfBounds = true;
                }
                if (lastZ > SPACE_Z_MAX || lastZ < SPACE_Z_MIN) {
                  outOfBounds = true;
                }
                // if not, just increment
                if (outOfBounds) {
                  //										std::cout
                  //<< "plane
                  //"
                  //<< prediction->id << " prediction already reaches end of
                  // airspace\n";
                  prediction->keep = true;
                  prediction->t++;
                  break;
                }

                // add new last prediction
                prediction->posX.pop_back();
                prediction->posX.push_back(lastX);
                prediction->posX.push_back(-1);
                prediction->posY.pop_back();
                prediction->posY.push_back(lastY);
                prediction->posY.push_back(-1);
                prediction->posZ.pop_back();
                prediction->posZ.push_back(lastZ);
                prediction->posZ.push_back(-1);

                // keep for computation and increment index
                prediction->keep = true;
                prediction->t++;
              }
              // update the prediction
              else {
                //									std::cout
                //<< "predictions do not match, recomputing predictions\n";
                prediction->posX.clear();
                prediction->posY.clear();
                prediction->posZ.clear();

                for (int i = 0; i < 180 / (currPeriod / 1000000); i++) {
                  int currX = craft->pos[0] +
                              i * (currPeriod / 1000000) * craft->vel[0];
                  int currY = craft->pos[1] +
                              i * (currPeriod / 1000000) * craft->vel[1];
                  int currZ = craft->pos[2] +
                              i * (currPeriod / 1000000) * craft->vel[2];

                  prediction->posX.push_back(currX);
                  prediction->posY.push_back(currY);
                  prediction->posZ.push_back(currZ);

                  bool outOfBounds = false;
                  if (currX > SPACE_X_MAX || currX < SPACE_X_MIN) {
                    outOfBounds = true;
                  }
                  if (currY > SPACE_Y_MAX || currY < SPACE_Y_MIN) {
                    outOfBounds = true;
                  }
                  if (currZ > SPACE_Z_MAX || currZ < SPACE_Z_MIN) {
                    outOfBounds = true;
                  }
                  if (outOfBounds) {
                    //													std::cout
                    //<< "reached end of plane " << prediction->id << "
                    // predictions\n";
                    break;
                  }
                }
                // set termination character
                prediction->posX.push_back(-1);
                prediction->posY.push_back(-1);
                prediction->posZ.push_back(-1);
                // set prediction index to next, keep for computation
                prediction->t = 1;
                prediction->keep = true;
              }
            }
          }

          //								std::cout
          //<< "compsys plane
          //"
          //<< craft->id
          //<< " new values:\n"
          //										<<
          //"posx
          //"
          //<< craft->pos[0]
          //<<
          //", posy
          //"
          //<< craft->pos[1] << ", posz " << craft->pos[2] << "\n"
          //										<<
          //"velx
          //"
          //<< craft->vel[0]
          //<<
          //", vely
          //"
          //<< craft->vel[1] << ", velz " << craft->vel[2] << "\n"
          //										<<
          //"set keep value to:
          //"
          //<< craft->keep
          //<<
          //"\n";
          break;
        }
      }

      // if plane was not already in the list, add it
      if (!inList) {
        // new pointer to struct, set members from read
        //					std::cout << "compsys found new
        // plane
        //"
        //<< id
        //<<
        //"\n";
        aircraft *currentAircraft = new aircraft();
        currentAircraft->id = id;
        currentAircraft->t_arrival = arrTime;
        currentAircraft->pos[0] = pos[0];
        currentAircraft->pos[1] = pos[1];
        currentAircraft->pos[2] = pos[2];
        currentAircraft->vel[0] = vel[0];
        currentAircraft->vel[1] = vel[1];
        currentAircraft->vel[2] = vel[2];
        currentAircraft->keep = true; // keep for first computation
        flyingPlanesInfo.push_back(
            currentAircraft); // add to struct pointer vector

        //					std::cout << "computing new
        // predictions for plane " << currentAircraft->id << "\n";
        trajectoryPrediction *currentPrediction = new trajectoryPrediction();

        currentPrediction->id = currentAircraft->id;

        for (int i = 0; i < 180 / (currPeriod / 1000000); i++) {
          int currX = currentAircraft->pos[0] +
                      i * (currPeriod / 1000000) * currentAircraft->vel[0];
          int currY = currentAircraft->pos[1] +
                      i * (currPeriod / 1000000) * currentAircraft->vel[1];
          int currZ = currentAircraft->pos[2] +
                      i * (currPeriod / 1000000) * currentAircraft->vel[2];

          currentPrediction->posX.push_back(currX);
          currentPrediction->posY.push_back(currY);
          currentPrediction->posZ.push_back(currZ);

          bool outOfBounds = false;
          if (currX > SPACE_X_MAX || currX < SPACE_X_MIN) {
            outOfBounds = true;
          }
          if (currY > SPACE_Y_MAX || currY < SPACE_Y_MIN) {
            outOfBounds = true;
          }
          if (currZ > SPACE_Z_MAX || currZ < SPACE_Z_MIN) {
            outOfBounds = true;
          }
          if (outOfBounds) {
            //							std::cout
            //<< "reached end of plane " << currentPrediction->id << "
            // predictions\n";
            break;
          }
        }
        // set termination character
        currentPrediction->posX.push_back(-1);
        currentPrediction->posY.push_back(-1);
        currentPrediction->posZ.push_back(-1);
        // set prediction index to next, keep for first computation
        currentPrediction->t = 1;
        currentPrediction->keep = true;
        // add to list
        trajectoryPredictions.push_back(currentPrediction);
      }

      // reset buffer and index for next plane
      readBuffer = "";
      j = 0;
      continue;
    }
    // found next data field in current plane
    else if (readChar == ',') {
      //				std::cout << "found next plane field
      // data:
      //"
      //<< readBuffer << "\n";
      switch (j) {
      // add whichever character the index j has arrived to
      case 0:
        id = atoi(readBuffer.c_str());
        readBuffer = "";
        j++;
        continue;
      case 1:
        arrTime = atoi(readBuffer.c_str());
        readBuffer = "";
        j++;
        continue;
      case 2:
        pos[0] = atoi(readBuffer.c_str());
        readBuffer = "";
        j++;
        continue;
      case 3:
        pos[1] = atoi(readBuffer.c_str());
        readBuffer = "";
        j++;
        continue;
      case 4:
        pos[2] = atoi(readBuffer.c_str());
        readBuffer = "";
        j++;
        continue;
      case 5:
        vel[0] = atoi(readBuffer.c_str());
        readBuffer = "";
        j++;
        continue;
      case 6:
        vel[1] = atoi(readBuffer.c_str());
        readBuffer = "";
        j++;
        continue;
      default:
        // just reset buffer
        readBuffer = "";
        break;
      }
    }
    readBuffer += readChar;
  }
  //		std::cout << "end read airspace\n";
  //				std::cout << "remaining planes after read:\n";
  //				for(aircraft *craft : flyingPlanesInfo){
  //					std::cout << "plane " << craft->id << ",
  // keep:
  //"
  //<< craft->keep
  //<< "\n"; 					std::cout << "new values: posx
  //"<< craft->pos[0]
  //<<
  //", posy " << craft->pos[1] << ", posz " << craft->pos[2] << "\n";
  //				}
  return false;
}

void ComputerSystem::cleanPredictions() {
  //		std::cout << "printing predictions\n";
  int j = 0;
  auto itpred = trajectoryPredictions.begin();
  while (itpred != trajectoryPredictions.end()) {
    //			std::cout << "plane " << (*itpred)->id << "
    // prediction keep: " << (*itpred)->keep << "\n";
    bool temp = (*itpred)->keep;

    // check if plane was terminated
    if (!temp) {
      //				std::cout << "compsys found plane " <<
      //(*itpred)->id << " prediction terminated\n";
      delete trajectoryPredictions.at(j);
      itpred = trajectoryPredictions.erase(itpred);
    } else {
      // print plane prediction info

      //				printf("plane %i predictions:\n",
      //(*itpred)->id);
      for (int i = (*itpred)->t - 1;
           i < (*itpred)->t + (180 / (currPeriod / 1000000)); i++) {
        int currX = (*itpred)->posX.at(i);
        int currY = (*itpred)->posY.at(i);
        int currZ = (*itpred)->posZ.at(i);

        if (currX == -1 || currY == -1 || currZ == -1) {
          //						std::cout << "reached
          // end of prediction for plane
          //"
          //<<
          //(*itpred)->id << "\n";
          break;
        }

        //					printf("posx: %i, posy: %i,
        // posz: %i\n", currX, currY, currZ);
      }

      (*itpred)->keep =
          false; // if found next time, this will become true again

      // only increment if no plane to remove
      j++;
      ++itpred;
    }
  }
}

void ComputerSystem::computeViolations() {
  //		std::cout << "computing airspace violations\n";
  auto itIndex = trajectoryPredictions.begin();
  while (itIndex != trajectoryPredictions.end()) {
    auto itNext = itIndex;
    ++itNext;
    while (itNext != trajectoryPredictions.end()) {
      // compare predictions, starting at current
      int j = (*itNext)->t - 1;
      for (int i = (*itIndex)->t - 1;
           i < (*itIndex)->t - 1 + (180 / (currPeriod / 1000000)); i++) {
        int currX = (*itIndex)->posX.at(i);
        int currY = (*itIndex)->posY.at(i);
        int currZ = (*itIndex)->posZ.at(i);
        int compX = (*itNext)->posX.at(j);
        int compY = (*itNext)->posY.at(j);
        int compZ = (*itNext)->posZ.at(j);

        if (currX == -1 || currY == -1 || currZ == -1) {
          //						std::cout << "reached
          // end of prediction for plane
          //"
          //<<
          //(*itIndex)->id << "\n";
          break;
        }
        if (compX == -1 || compY == -1 || compZ == -1) {
          //						std::cout << "reached
          // end of prediction for plane
          //"
          //<<
          //(*itNext)->id << "\n";
          break;
        }

        if ((abs(currX - compX) <= 3000 || abs(currY - compY) <= 3000) &&
            abs(currZ - compZ) <= 1000) {
          std::cout << "airspace violation detected between planes "
                    << (*itIndex)->id << " and " << (*itNext)->id
                    << " at time current + " << i * (currPeriod) / 1000000
                    << "\n";
          bool currComm = false;
          bool compComm = false;
          // find comm
          for (void *comm : commPtrs) {
            // comm shm id
            int commId = atoi((char *)comm);

            if (commId == (*itIndex)->id) {
              //								std::cout
              //<< "found plane " << (*itIndex)->id << " comm, sending
              // message\n";
              // find command index in plane shm
              int k = 0;
              char readChar;
              for (; k < SIZE_SHM_PLANES; k++) {
                readChar = *((char *)comm + k);
                if (readChar == ';') {
                  break;
                }
              }
              // set pointer after plane termination
              k++;

              // command string (send one of the planes upwards)
              std::string command = "z,200;";

              // write command to plane
              sprintf((char *)comm + k, "%s", command.c_str());

              // current plane comm found
              currComm = true;
            }
            if (commId == (*itNext)->id) {
              //								std::cout
              //<< "found plane " << (*itNext)->id << " comm, sending
              // message\n";
              // find command index in plane shm
              int k = 0;
              char readChar;
              for (; k < SIZE_SHM_PLANES; k++) {
                readChar = *((char *)comm + k);
                if (readChar == ';') {
                  break;
                }
              }
              // set pointer after plane termination
              k++;

              // command string (send one of the planes downwards)
              std::string command = "z,-200;";

              // write command to plane
              sprintf((char *)comm + k, "%s", command.c_str());

              // compared plane comm found
              compComm = true;
            }
            if (currComm && compComm) {
              break;
            }
          }
          // set associate craft info request to true, display height
          currComm = false;
          compComm = false;
          for (aircraft *craft : flyingPlanesInfo) {
            if (craft->id == (*itIndex)->id) {
              craft->moreInfo = true;
              craft->commandCounter = NUM_PRINT;
              currComm = true;
            }
            if (craft->id == (*itNext)->id) {
              craft->moreInfo = true;
              craft->commandCounter = NUM_PRINT;
              compComm = true;
            }
            if (currComm && compComm) {
              break;
            }
          }
          break;
        }
        //					std::cout << "incrementing j
        // counter for prediction compare\n";
        j++;
      }
      ++itNext;
    }
    ++itIndex;
  }
  //		std::cout << "after compute airspace violations\n";
}

void ComputerSystem::writeToDisplay() {
  //		std::cout << "writing to display\n";

  std::string displayBuffer = "";
  std::string currentPlaneBuffer = "";
  //		printf("airspace that was read: %s\n", airspacePtr);
  // print what was found, remove what is no longer in the airspace
  int i = 0;
  auto it = flyingPlanesInfo.begin();
  while (it != flyingPlanesInfo.end()) {
    //			std::cout << "plane " << (*it)->id << " keep: "
    //<< (*it)->keep << "\n";
    bool temp = (*it)->keep; // check if plane was terminated

    if (!temp) {
      //				std::cout << "compsys found plane " <<
      //(*it)->id << " terminated\n";
      delete flyingPlanesInfo.at(i);
      it = flyingPlanesInfo.erase(it);
      numPlanes--;
      //				std::cout << "computer system number of
      // planes left: " << numPlanes << "\n";
    } else {
      // print plane info
      //				printf("plane %i:\n", (*it)->id);
      //				printf("posx: %i, posy: %i, posz: %i\n",
      //(*it)->pos[0], (*it)->pos[1], (*it)->pos[2]);
      // printf("velx: %i, vely: %i, velz: %i\n", (*it)->vel[0],
      // (*it)->vel[1],
      //(*it)->vel[2]);

      // add plane to buffer for display

      // id,posx,posy,posz,info
      // ex: 1,15000,20000,5000,0
      displayBuffer = displayBuffer + std::to_string((*it)->id) + "," +
                      std::to_string((*it)->pos[0]) + "," +
                      std::to_string((*it)->pos[1]) + "," +
                      std::to_string((*it)->pos[2]) + ",";

      // check if more info requested
      if ((*it)->moreInfo) {
        (*it)->commandCounter--;
        if ((*it)->commandCounter <= 0) {
          (*it)->moreInfo = false;
        }
        displayBuffer += "1/";
      } else {
        displayBuffer += "0/";
      }

      (*it)->keep = false; // if found next time, this will become true again

      // only increment if no plane to remove
      i++;
      ++it;
    }
  }
  // termination character
  displayBuffer = displayBuffer + ";";

  sprintf((char *)displayPtr, "%s", displayBuffer.c_str());
}

void ComputerSystem::updatePeriod(int chid) {
  int newPeriod = 0;
  if (flyingPlanesInfo.size() <= 5) {
    // set period low
    newPeriod = 5000000;
  }
  if (flyingPlanesInfo.size() > 5 && flyingPlanesInfo.size() <= 20) {
    // set period medium
    newPeriod = 3000000;
  }
  if (flyingPlanesInfo.size() > 20 && flyingPlanesInfo.size() <= 50) {
    // set period high
    newPeriod = 2000000;
  }
  if (flyingPlanesInfo.size() > 50 && flyingPlanesInfo.size() <= 200) {
    // set period overdrive
    newPeriod = 1000000;
  }

  if (currPeriod != newPeriod) {
    //			std::cout << "compsys period changed to: " <<
    // newPeriod << "\n";
    currPeriod = newPeriod;
    std::string CSPeriod = std::to_string(currPeriod);
    sprintf((char *)periodPtr, CSPeriod.c_str());
    timer->setTimer(currPeriod, currPeriod);
    //			printf("cs period shm after write: %s\n",
    // periodPtr);
  }
}
