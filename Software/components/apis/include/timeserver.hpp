#pragma once
#include "datetime.hpp"
#include <string>
#include <sys/time.h>

// start to continously ask timeserver for the current time
// information for the functions used in it can be found here:
// https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/system/system_time.html
void init_timeserver();

// callback function that is called when time is received from timeserver
void time_received(struct timeval *timeval);

// returns whether or not the timeserver has been reached at least once
bool is_time_synchronized();

// switches the used timeserver to the next one in the list
void try_next_timeserver();

// stops timeserver polling
void stop_timeserver();