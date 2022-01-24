#include <eventCounter.h>

EventCounter::EventCounter() : nbEvents(0) {
  //    memset(&startTS, 0, sizeof(struct timeval));
  restart();
}

void EventCounter::restart() {
  nbEvents=0;
  if(gettimeofday(&startTS, NULL)!=0){
    perror("gettimeofday failed");
  }
}

std::string EventCounter::toString() {
  struct timeval now;
  gettimeofday(&now, NULL);
  uint32_t elapsedTime=now.tv_sec-startTS.tv_sec;
  float nbEventPerSecond=((float)nbEvents)/elapsedTime;
  std::stringstream ss;
  ss<<nbEvents<<" events in "<<elapsedTime<<" seconds ("<<nbEventPerSecond<<"/s)";
  return ss.str();
}

void EventCounter::increment(){
  this->nbEvents++;
}
