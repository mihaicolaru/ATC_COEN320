/*
 * SSR.cpp
 *
 *  Created on: Nov 30, 2022
 *      Author: JZ
 */

#include "SSR.h"

SSR::SSR(int numberOfPlanes) {
  currPeriod = SSR_PERIOD;
  initialize(numberOfPlanes);
}

SSR::~SSR() {
  shm_unlink("flying_planes");
  shm_unlink("airspace");
  shm_unlink("period");
  pthread_mutex_destroy(&mutex);
  delete timer;
}

int SSR::start() {
  if (pthread_create(&SSRthread, &attr, &SSR::startSSR, (void *)this) != EOK) {
    SSRthread = NULL;
  }
}

int SSR::stop() {
  pthread_join(SSRthread, NULL);
  return 0;
}

void *SSR::startSSR(void *context) { ((SSR *)context)->operateSSR(); }

int SSR::initialize(int numberOfPlanes) {
  numPlanes = numberOfPlanes;
  // set thread in detached state
  int rc = pthread_attr_init(&attr);
  if (rc) {
    printf("ERROR, RC from pthread_attr_init() is %d \n", rc);
  }

  rc = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  if (rc) {
    printf("ERROR; RC from pthread_attr_setdetachstate() is %d \n", rc);
  }

  // open list of waiting planes shm
  shm_flyingPlanes = shm_open("flying_planes", O_RDWR, 0666);
  if (shm_flyingPlanes == -1) {
    perror("in shm_open() SSR: flying planes");
    exit(1);
  }

  flyingPlanesPtr = mmap(0, SIZE_SHM_SSR, PROT_READ | PROT_WRITE, MAP_SHARED,
                         shm_flyingPlanes, 0);
  if (flyingPlanesPtr == MAP_FAILED) {
    perror("in map() SSR: flying planes");
    exit(1);
  }

  // open airspace shm
  shm_airspace = shm_open("airspace", O_RDWR, 0666);
  if (shm_airspace == -1) {
    perror("in shm_open() SSR: airspace");
    exit(1);
  }

  // map airspace shm
  airspacePtr = mmap(0, SIZE_SHM_AIRSPACE, PROT_READ | PROT_WRITE, MAP_SHARED,
                     shm_airspace, 0);
  if (airspacePtr == MAP_FAILED) {
    perror("in map() SSR: airspace");
    exit(1);
  }

  // open period shm
  shm_period = shm_open("period", O_RDONLY, 0666);
  if (shm_period == -1) {
    perror("in shm_open() SSR: period");
    exit(1);
  }

  // map airspace shm
  periodPtr = mmap(0, SIZE_SHM_PERIOD, PROT_READ, MAP_SHARED, shm_period, 0);
  if (periodPtr == MAP_FAILED) {
    perror("in map() SSR: period");
    exit(1);
  }

  return 0;
}

void *SSR::operateSSR(void) {
  // create channel to communicate with timer
  int chid = ChannelCreate(0);
  if (chid == -1) {
    std::cout << "couldn't create channel!\n";
  }

  Timer *newTimer = new Timer(chid);
  timer = newTimer;
  timer->setTimer(SSR_PERIOD, SSR_PERIOD);

  int rcvid;
  Message msg;

  while (1) {
    if (rcvid == 0) {
      // lock mutex
      pthread_mutex_lock(&mutex);

      // check period shm to see if must update period
      updatePeriod(chid);

      // read flying planes shm
      bool writeFlying = readFlyingPlanes();

      // get info from planes, write to airspace
      bool writeAirspace = getPlaneInfo();

      // if new/terminated plane found, write new flying planes
      if (writeFlying || writeAirspace) {
        writeFlyingPlanes();
      }

      // unlock mutex
      pthread_mutex_unlock(&mutex);

      // check for termination
      if (numPlanes <= 0) {
        std::cout << "ssr done\n";
        sprintf((char *)airspacePtr, "terminated");
        ChannelDestroy(chid);

        return 0;
      }
    }
    rcvid = MsgReceive(chid, &msg, sizeof(msg), NULL);
  }

  ChannelDestroy(chid);

  return 0;
}

// update ssr period based on period shm
void SSR::updatePeriod(int chid) {
  int newPeriod = atoi((char *)periodPtr);
  if (newPeriod != currPeriod) {
    currPeriod = newPeriod;
    timer->setTimer(currPeriod, currPeriod);
  }
}

// ================= read flying planes shm =================
bool SSR::readFlyingPlanes() {
  bool write = false;
  std::string FD_buffer = "";
  for (int i = 0; i < SIZE_SHM_SSR; i++) {
    char readChar = *((char *)flyingPlanesPtr + i);

    // shm termination character found
    if (readChar == ';') {
      if (i == 0) {
        break; // no flying planes
      }
      // check if flying plane was already in list
      bool inFile = false;

      for (std::string filename : flyingFileNames) {
        if (filename == FD_buffer) {
          inFile = true;
          break;
        }
      }

      // if not, add to list
      if (!inFile) {
        write = true; // found plane that is not already flying

        flyingFileNames.push_back(FD_buffer);

        // open shm for current plane
        int shm_plane = shm_open(FD_buffer.c_str(), O_RDONLY, 0666);
        if (shm_plane == -1) {
          perror("in shm_open() SSR plane");

          exit(1);
        }

        // map memory for current plane
        void *ptr =
            mmap(0, SIZE_SHM_PLANES, PROT_READ, MAP_SHARED, shm_plane, 0);
        if (ptr == MAP_FAILED) {
          perror("in map() SSR");
          exit(1);
        }

        planePtrs.push_back(ptr);

        // end of flying planes
        break;
      }
      break;
    }
    // found a plane, open shm
    else if (readChar == ',') {
      // check if plane was already in list
      bool inFile = false;

      for (std::string filename : flyingFileNames) {
        if (filename == FD_buffer) {
          //								std::cout << FD_buffer << " already
          //in list\n";
          inFile = true;
        }
      }

      // if not, add to list
      if (!inFile) {
        write = true; // found new flying plane that wasn't already flying

        flyingFileNames.push_back(FD_buffer);

        // open shm for current plane
        int shm_plane = shm_open(FD_buffer.c_str(), O_RDONLY, 0666);
        if (shm_plane == -1) {
          perror("in shm_open() SSR plane");

          exit(1);
        }

        // map memory for current plane
        void *ptr =
            mmap(0, SIZE_SHM_PLANES, PROT_READ, MAP_SHARED, shm_plane, 0);
        if (ptr == MAP_FAILED) {
          perror("in map() SSR");
          exit(1);
        }

        planePtrs.push_back(ptr);

        // reset buffer for next plane
        FD_buffer = "";
        continue;
      }

      FD_buffer = "";
      continue;
    }

    // just add the char to the buffer
    FD_buffer += readChar;
  }

  return write;
}

// ================= get info from planes =================
bool SSR::getPlaneInfo() {
  bool write = false;
  // buffer for all plane info
  std::string airspaceBuffer = "";

  int i = 0;
  auto it = planePtrs.begin();
  while (it != planePtrs.end()) {
    std::string readBuffer = "";
    char readChar = *((char *)*it);

    // remove plane (terminated)
    if (readChar == 't') {
      write = true; // found plane to remove

      // remove current fd from flying planes fd vector
      flyingFileNames.erase(flyingFileNames.begin() + i);

      // remove current plane from ptr vector
      it = planePtrs.erase(it);

      // reduce number of planes
      numPlanes--;
    }
    // plane not terminated, read all data and add to buffer for airspace
    else {
      int j = 0;
      //						std::cout << "reading
      //current plane data\n";
      for (; j < SIZE_SHM_PLANES; j++) {
        char readChar = *((char *)*it + j);

        // end of plane shm
        if (readChar == ';') {
          break;
        }

        // if not first plane read, append "/" to front (plane separator)
        if (i != 0 && j == 0) {
          readBuffer += "/"; // plane separator
        }
        // add current character to buffer
        readBuffer += readChar;
      }
      // no planes
      if (j == 0) {
        break;
      }
      i++; // only increment if no plane to terminate and plane info added
      ++it;
    }

    // add current buffer to buffer for airspace shm
    airspaceBuffer += readBuffer;
  }

  // termination character for airspace write
  airspaceBuffer += ";";

  sprintf((char *)airspacePtr, "%s", airspaceBuffer.c_str());

  return write;
}

// ================= write new flying planes shm =================
void SSR::writeFlyingPlanes() {
  // new flying planes buffer
  std::string currentAirspace = "";

  int j = 0;
  // add planes to transfer to buffer
  for (std::string filename : flyingFileNames) {
    if (j == 0) {
      currentAirspace += filename;
      j++;
    } else {
      currentAirspace += ",";
      currentAirspace += filename;
    }
  }
  // termination character for flying planes list
  currentAirspace += ";";

  // write new flying planes list
  sprintf((char *)flyingPlanesPtr, "%s", currentAirspace.c_str());
}
