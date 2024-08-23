#include <cstdlib>
#include <cstdio>
#include <new>

//#define SPRF_ALLOCATION_TRACKING

#ifdef SPRF_ALLOCATION_TRACKING

void * operator new(std::size_t n){
    return malloc(n);
}

void operator delete(void* p) throw(){
    free(p);
}

#endif