#include "statsnode.hpp"

using namespace TrentCode;

void StatsNode::start() {
    startTime = std::chrono::high_resolution_clock::now();
}

void StatsNode::end() {
    stats.push_back(Stat(startTime, std::chrono::high_resolution_clock::now())); 
    unsigned long statTotal = 0;
    unsigned long statCount = 0;
    for (Stat s : stats) {
       auto duration = std::chrono::duration_cast<std::chrono::microseconds>(s.endTime - s.startTime);
       statCount++;
       statTotal = statTotal + duration.count();
    }
    std::cout << statTotal << "/" << statCount << "=" << (statTotal/statCount) << std::endl;
}
