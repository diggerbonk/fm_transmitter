
#pragma once

#include <string>
#include <iostream>
#include <mutex>
#include <map>
#include <list>
#include <sstream>
#include <chrono>
#include <queue>
#include "statsnode.hpp"
#include "stat.hpp"

namespace TrentCode
{

const unsigned int aggregationPeriod = 60; // aggregate every one minute
const unsigned int maxStatCount = 1000; // force aggregation at this size
const unsigned int maxCallDepth = 10; // max call depth.

extern thread_local std::list<std::string> path;
extern thread_local std::map<std::string, StatsNode> statsMap;
extern thread_local unsigned int callDepth;
extern thread_local unsigned int statCount;

class Observation
{
    StatsNode * nodePtr = NULL;
public:

    Observation(const char * name)
    { 
        path.push_back(name);
        std::pair<std::map<std::string, StatsNode >::iterator, bool > item = 
            statsMap.insert(std::pair<std::string,StatsNode>(getPath(), StatsNode()));
        nodePtr = &(*item.first).second;
        nodePtr->start();
    }

    std::string getPath() {
        std::ostringstream os;
        for (std::string s : path) {
            os << s << ":";
        }
        return os.str();
    }

    void end() {
        if (NULL != nodePtr)
        {
            statCount++; 
            callDepth--;
            std::cout << "OUT " << getPath() << std::endl;
            nodePtr->end();
            if (!path.empty()) path.pop_back();
            nodePtr = NULL;

            // check if it is time to 
            if (statCount > maxStatCount) {
                // aggregate stats. 
            }
        }
    }

    ~Observation() 
    {
        end();
    }
}; // class Observation

} // namespace TrentCode

#define CPROF TrentCode::Observation TrentCode(__func__)
