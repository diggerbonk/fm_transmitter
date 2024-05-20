
#pragma once

#include <chrono>

namespace TrentCode
{

struct Stat
{
    Stat(std::chrono::high_resolution_clock::time_point st,
            std::chrono::high_resolution_clock::time_point et) 
    {
        startTime = st;
        endTime = et;
    }

    std::chrono::high_resolution_clock::time_point startTime;
    std::chrono::high_resolution_clock::time_point endTime;
}; // struct Stat


} // namespace TrentCode

