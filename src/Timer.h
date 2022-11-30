/*
 * Timer.h
 *
 *  Created on: Nov. 5, 2022
 *      Author: Mihai
 */

#ifndef TIMER_H_
#define TIMER_H_

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/neutrino.h>
#include <sys/siginfo.h>
#include <time.h>
#include <unistd.h>

#include "Limits.h"
// message structure
typedef struct {
  int messageType; // contains both message to and from client
  int messageData; // optional data, depending upon message
} ClientMessage;

typedef union {
  ClientMessage msg;   // a message can be either from a client, or
  struct _pulse pulse; // a pulse
} Message;

class Timer {
public:
  Timer(int chid);
  int setTimer(int offset, int period);
  timer_t timerid;         // timer ID
  struct sigevent event;   // event to deliver
  struct itimerspec timer; // timer data structure
  int coid;                // connection back to ourselves
};

#endif /* TIMER_H_ */
