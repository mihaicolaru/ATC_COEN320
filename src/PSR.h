/*
 * PSR.h
 *
 *  Created on: Oct. 24, 2022
 *      Author: mihai, minhtrannhat
 */

#ifndef PSR_H_
#define PSR_H_

#include <fstream>
#include <iostream>

class PSR {
public:
  // constructor
  PSR() {
    std::string filename = "./input.txt";
    std::ifstream input_file_stream;

    input_file_stream.open(filename);

    if (!input_file_stream) {
      std::cout << "Can't find file input.txt" << std::endl;
      return;
    }

    int ID, arrivalTime, arrivalCordX, arrivalCordY, arrivalCordZ,
        arrivalSpeedX, arrivalSpeedY, arrivalSpeedZ;

    std::string seperator = " ";

    std::cout << "ID  "
              << "arrivalTime  "
              << "arrivalCordX  "
              << "arrivalCordY  "
              << "arrivalCordZ  "
              << "arrivalSpeedX  "
              << "arrivalSpeedY  "
              << "arrivalSpeedZ" << std::endl;

    while (input_file_stream >> ID >> arrivalTime >> arrivalCordX >>
           arrivalCordY >> arrivalCordZ >> arrivalSpeedX >> arrivalSpeedY >>
           arrivalSpeedZ) {
      std::cout << ID << seperator << arrivalTime << seperator << arrivalCordX
                << seperator << arrivalCordY << seperator << arrivalCordZ
                << seperator << arrivalSpeedX << seperator << arrivalCordY
                << seperator << arrivalSpeedZ << std::endl;
      // Initialize planes and then put them to Queues
      // Planes (ID, arrivalTime, ...)
    }
  }

  // destructor
  ~PSR() {}

  int discover() {
    // scan all planes and find ready (Tarrival <= Tcurrent)
    // move ready planes to ready list
  }

private:
  // list of all planes
  // list of ready planes
};

#endif /* PSR_H_ */
