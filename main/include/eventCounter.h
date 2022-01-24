#ifndef COMPONENTS_MASTERBUS_ESP32_MAIN_INCLUDE_EVENTCOUNTER_H_
#define COMPONENTS_MASTERBUS_ESP32_MAIN_INCLUDE_EVENTCOUNTER_H_

#include <errno.h>
#include <sstream>

class EventCounter {
public:
  EventCounter();
  void restart();
  std::string toString();
  void increment();
  uint32_t nbEvents;
  struct timeval startTS;
};


#endif /* COMPONENTS_MASTERBUS_ESP32_MAIN_INCLUDE_EVENTCOUNTER_H_ */
