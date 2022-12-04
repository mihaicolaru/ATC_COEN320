/*
 * Display.cpp
 *
 *  Created on: Nov 30, 2022
 *      Author: JZ
 */

#include "Display.h"
#include <iostream>

Display::Display() {
  initialize(); // Construction Initialization Call
}

Display::~Display() {
  shm_unlink("display"); // Unlink shared memory on termination
}

int Display::initialize() {
  /*Make threads in detached state*/
  int rc = pthread_attr_init(&attr);
  if (rc) {
    printf("ERROR, RC from pthread_attr_init() is %d \n", rc);
  }

  rc = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  if (rc) {
    printf("ERROR; RC from pthread_attr_setdetachstate() is %d \n", rc);
  }

  /*Create shared memory for display*/
  // open list of waiting planes shm
  shm_display = shm_open("display", O_RDWR, 0666);
  if (shm_display == -1) {
    perror("in shm_open() Display");
    exit(1);
  }

  ptr_display = mmap(0, SIZE_SHM_DISPLAY, PROT_READ | PROT_WRITE, MAP_SHARED,
                     shm_display, 0);

  if (ptr_display == MAP_FAILED) {

    perror("in map() Display");
    exit(1);
  }

  return 0;
}

void Display::start() {
  if (pthread_create(&displayThread, &attr, &Display::startDisplay,
                     (void *)this) != EOK) {
    displayThread = 0;
  }
}

int Display::stop() {
  pthread_join(displayThread, NULL); // Join thread on stop
  return 0;
}

void *Display::startDisplay(void *context) {
  ((Display *)context)->updateDisplay();
  return NULL;
}

void *Display::updateDisplay(void) {
  // Link to channel
  int chid = ChannelCreate(0);
  if (chid == -1) {
    std::cout << "couldn't create display channel!\n";
  }

  // Setup period of 5 seconds
  Timer timer(chid);
  timer.setTimer(PERIOD_D, PERIOD_D);

  // Receive data
  int rcvid;
  Message msg;
  int logging_counter = 1;
  std::ofstream out("log");

  while (1) {
    if (rcvid == 0) {
      pthread_mutex_lock(&mutex);
      // Parsing buffers
      int axis = 0, z = 0; // 0=ID, 1=X, 2=Y, 3=Z, 4=Height display control bit;
      std::string buffer = "";
      std::string x = "", y = "", display_bit = "";
      std::string id = "";
      // Read from shared memory pointer
      for (int i = 0; i < SIZE_SHM_DISPLAY; i++) {
        char readChar = *((char *)ptr_display + i);
        if (readChar == 't') {
          std::cout << "display done\n";
          ChannelDestroy(chid);
          out.close();
          return 0;
        }
        // Check for ending character
        if (readChar == ';') {
          break;
        } else if (readChar == ',' || readChar == '/') {
          // Check for 2 delimiters
          // Load buffer according to placement of value
          if (buffer.length() > 0) {
            switch (axis) {
            case 0:
              id = buffer;
              break;
            case 1:
              x = buffer;
              break;
            case 2:
              y = buffer;
              break;
            case 3:
              z = stoi(buffer);
              z += SPACE_ELEVATION;
              break;
            case 4:
              display_bit = buffer;
              break;
            }
          }
          if (readChar == ',') {
            axis++;
            buffer = "";
          } else if (readChar == '/') {
            // One plane has finished loading, parsing and reset control values
            if (map[(100000 - stoi(y)) / SCALER][stoi(x) / SCALER] == "") {
              map[(100000 - stoi(y)) / SCALER][stoi(x) / SCALER] += id;
            } else {
              map[(100000 - stoi(y)) / SCALER][stoi(x) / SCALER] += "\\" + id;
            }
            // Height display control
            if (display_bit == "1") {
              height_display = height_display + "Plane " + id +
                               " has height of " + std::to_string(z) + " ft\n";
            }
            x = "";
            y = "";
            z = 0;
            id = "";
            display_bit = "";
            axis = 0;
            buffer = "";
          }
        } else {
          buffer +=
              readChar; // Keep loading buffer until delimiter has been detected
        }
      }
      pthread_mutex_unlock(&mutex);

      // Logging the airspace into a log file
      // Since every period is 5 seconds, we want to wait 6 periods to do this
      if (logging_counter == 6) {
        std::cout << "Logging current airspace..." << std::endl;
        std::streambuf *coutbuf = std::cout.rdbuf(); // save old buf
        std::cout.rdbuf(out.rdbuf()); // redirect std::cout to out.txt!

        // this used to print to stdout but now we redirected it to out.txt
        printMap(); // Display map and height command
        std::cout << std::endl;

        std::cout.rdbuf(coutbuf); // reset to standard output again
        logging_counter = 1;
      }

      // if not during a multiple of 30 second period, just print to stdout
      // normally
      else {
        printMap(); // Display map and height command
      }

      logging_counter++;

      height_display = ""; // Reset buffer
      // Reset map array
      memset(map, 0,
             sizeof(map[0][0]) * block_count *
                 block_count); // Reset map to 0 for next set
    }
    rcvid = MsgReceive(chid, &msg, sizeof(msg), NULL);
  }
  ChannelDestroy(chid);
  return 0;
}
// Printing map
void Display::printMap() {
  for (int j = 0; j < block_count; j++) {
    for (int k = 0; k < block_count; k++) {
      // Print Empty block if no item
      if (map[j][k] == "") {
        std::cout << "_|";
      } else {
        // print plane ID if there are items
        std::cout << map[j][k] << "|";
      }
    }
    std::cout << std::endl;
  }
  // Display height
  printf("%s\n", height_display.c_str());
}
