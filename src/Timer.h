/*
 * Timer.h
 *
 *  Created on: Nov. 5, 2022
 *      Author: Mihai
 */

#ifndef TIMER_H_
#define TIMER_H_

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <sys/siginfo.h>
#include <sys/neutrino.h>

#define ONE_THOUSAND	1000
#define ONE_MILLION		1000000
/* offset and period are in microsends. */

// message send definitions

// messages
#define MT_WAIT_DATA        2       // message from client
#define MT_SEND_DATA        3       // message from client

// pulses
#define CODE_TIMER          1       // pulse from timer

// message reply definitions
#define MT_OK               0       // message to client
#define MT_TIMEDOUT         1       // message to client

// message structure
typedef struct
{
    int messageType;                // contains both message to and from client
    int messageData;                // optional data, depending upon message
} ClientMessage;

typedef union
{
    ClientMessage  msg;             // a message can be either from a client, or
    struct _pulse   pulse;          // a pulse
} Message;

class Timer{
public:
	Timer(){
		// default constructor
	}

	Timer(int chid){
		coid = ConnectAttach(0, 0, chid, 0, 0);
		if(coid == -1){
			fprintf(stderr, "%s: couldn't create channel\n");
			perror(NULL);
			exit(EXIT_FAILURE);
		}

	    // set up the kind of event that we want to deliver -- a pulse
		SIGEV_PULSE_INIT(&event, coid, SIGEV_PULSE_PRIO_INHERIT, CODE_TIMER, 0);

		// create the timer, binding it to the event
		if (timer_create (CLOCK_REALTIME, &event, &timerid) == -1) {
			fprintf (stderr, "couldn't create a timer, errno %s\n",errno);
			perror (NULL);
			exit (EXIT_FAILURE);
		}

		std::cout << "timer created\n";
	}

	int setTimer(uint64_t offset, int period){
		timer.it_value.tv_sec = offset / ONE_MILLION;
		timer.it_value.tv_nsec = (offset % ONE_MILLION) * ONE_THOUSAND;
		timer.it_interval.tv_sec = period / ONE_MILLION;
		timer.it_interval.tv_nsec = (period % ONE_MILLION) * ONE_THOUSAND;

		return timer_settime(timerid, 0, &timer, NULL);
	}




public:
	timer_t timerid;			// timer ID
	struct sigevent event;		// event to deliver
	struct itimerspec timer;	// timer data structure
	int coid;					// connection back to ourselves
};

//#include <sys/neutrino.h>
//
//typedef struct
//{
//	int message;  //DATA_REQUEST 1 , DATA_READY 2
//}_message;
//
//typedef union
//{
//	_message msg;
//	struct _pulse pulse;
//}Message;
//
//#include <iostream>
//#include <time.h>
//#include <errno.h>
//#include <sys/siginfo.h>
//#include <sys/neutrino.h>
//
//#define ONE_THOUSAND	1000
//#define ONE_MILLION		1000000
///* offset and period are in microsends. */
//
//#define TIMERCODE 1
//
//class Timer
//{
//	private:
//		int coid;                  //connection back to channel
//		struct sigevent event;     //event to deliver
//		struct itimerspec itime;   //timer's data struct
//		timer_t timer_id;          //timer's ID for the timer
//		sigset_t sigst;
//
//
//	public:
//		Timer() {};
//
//		Timer(int chid)
//		{
//				coid = ConnectAttach(0, 0, chid, 0, 0);   //Attach timer to our channel
//				if (coid == -1)
//				{
//					std::cerr << "Timer: ConnectAttach ERROR :(" << std::endl;
//					//exit(EXIT_FAILURE);
//				}
//				//initialization macro of the sigevent:"event" structure
//				SIGEV_PULSE_INIT(&event, coid, SIGEV_PULSE_PRIO_INHERIT, TIMERCODE, 0);
//				std::cout << "TIMER pulse initiated" << std::endl;
//
//		}
//		~Timer()
//		{
//			  //cout << "TIMER Destructor called" << endl;
//		}
//
//		int setTimer(uint64_t offset, int period) {
//			struct itimerspec timer_spec;
//			struct sigevent sigev;
//			timer_t timer;
//			const int signal = SIGALRM;
//			int res;
//
//			/* set timer parameters */
//			timer_spec.it_value.tv_sec = offset / ONE_MILLION;
//			timer_spec.it_value.tv_nsec = (offset % ONE_MILLION) * ONE_THOUSAND;
//			timer_spec.it_interval.tv_sec = period / ONE_MILLION;
//			timer_spec.it_interval.tv_nsec = (period % ONE_MILLION) * ONE_THOUSAND;
//
//			sigemptyset(&sigst); // initialize a signal set
//			sigaddset(&sigst, signal); // add SIGALRM to the signal set
//			sigprocmask(SIG_BLOCK, &sigst, NULL); //block the signal
//
//			/* set the signal event a timer expiration */
//			memset(&sigev, 0, sizeof(struct sigevent));
//			sigev.sigev_notify = SIGEV_SIGNAL;
//			sigev.sigev_signo = signal;
//
//			/* create timer */
//			res = timer_create(CLOCK_MONOTONIC, &sigev, &timer);
//
//			if (res < 0) {
//				perror("Timer Create");
//				exit(-1);
//			}
//
//			/* activate the timer */
//			return timer_settime(timer, 0, &timer_spec, NULL);
//		}
//
//		void setTimer(long seconds, long nanoseconds)
//		{
//			//Will receive  pulse initially (the itime.it_value) and every
//			//(the itime.it_interval)seconds thereafter
//			itime.it_value.tv_sec = seconds;
//			itime.it_value.tv_nsec = nanoseconds;
//			itime.it_interval.tv_sec = seconds;
//			itime.it_interval.tv_nsec = nanoseconds;
//			timer_settime(timer_id, 0, &itime, NULL);    //0 for relative time
//		}
//};

//=============================================================================



#endif /* TIMER_H_ */
