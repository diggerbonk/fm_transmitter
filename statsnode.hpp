
#pragma once

#include <iostream>
#include <list>
#include <chrono>
#include "stat.hpp"

namespace TrentCode
{

class StatsNode
{
public:

    std::chrono::high_resolution_clock::time_point startTime;
    std::list<Stat> stats; 

    void start();
    void end();

}; // class StatsNode

} // namespace TrentCode

