#include "Timer.h"

Timer::Timer(int chid) {
  coid = ConnectAttach(0, 0, chid, 0, 0);
  if (coid == -1) {
    fprintf(stderr, "%s: couldn't create channel!\n");
    perror(NULL);
    exit(EXIT_FAILURE);
  }

  // set up the kind of event that we want to deliver -- a pulse
  SIGEV_PULSE_INIT(&event, coid, SIGEV_PULSE_PRIO_INHERIT, CODE_TIMER, 0);

  // create the timer, binding it to the event
  if (timer_create(CLOCK_REALTIME, &event, &timerid) == -1) {
    fprintf(stderr, "couldn't create a timer, errno %d\n", errno);
    perror(NULL);
    exit(EXIT_FAILURE);
  }
}

// kind of timer: relative
int Timer::setTimer(int offset, int period) {
  timer.it_value.tv_sec = offset / ONE_MILLION;
  timer.it_value.tv_nsec = (offset % ONE_MILLION) * ONE_THOUSAND;
  timer.it_interval.tv_sec = period / ONE_MILLION;
  timer.it_interval.tv_nsec = (period % ONE_MILLION) * ONE_THOUSAND;

  return timer_settime(timerid, 0, &timer, NULL);
}
