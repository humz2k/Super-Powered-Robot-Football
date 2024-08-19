#ifndef _SPRF_RSS_HPP_
#define _SPRF_RSS_HPP_

#include <cstdlib>

extern "C" {

size_t getPeakRSS();

size_t getCurrentRSS();
}

#endif // _SPRF_RSS_HPP_