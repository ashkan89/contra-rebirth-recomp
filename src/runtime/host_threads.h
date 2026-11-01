#pragma once
#if defined(PI_ZERO2) && defined(USE_GLES2) && defined(__linux__)
#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#include <cstdlib>
#include <cstdio>
#endif

inline void host_thread_role(int core,const char* name) {
#if defined(PI_ZERO2) && defined(USE_GLES2) && defined(__linux__)
    pthread_setname_np(pthread_self(),name);
    static const bool enabled=[] { const char* value=getenv("CPU_AFFINITY"); return value && atoi(value)!=0; }();
    if(!enabled || sysconf(_SC_NPROCESSORS_ONLN)<4) return;
    cpu_set_t cpus; CPU_ZERO(&cpus); CPU_SET(core,&cpus);
    int error=pthread_setaffinity_np(pthread_self(),sizeof cpus,&cpus);
    if(error) fprintf(stderr,"[threads] affinity for %s failed (%d)\n",name,error);
#else
    (void)core;(void)name;
#endif
}
