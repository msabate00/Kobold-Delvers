#ifndef __TIMER_H__
#define __TIMER_H__

#include "app/defs.h"
#include <chrono>

class Timer
{
public:

    Timer();

    void Start(uint32 startingMS = 0);
    void Stop();
    float Read() const;
    std::string ReadString() const;


    uint32 CountDown(int total) const;
    bool IsStarted() const;

private:
    std::chrono::steady_clock::time_point startTime;
    bool started;
};

#endif //__TIMER_H__