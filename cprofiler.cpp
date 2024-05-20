#include "cprofiler.hpp"

namespace TrentCode 
{
thread_local std::list<std::string> path;
thread_local std::map<std::string, StatsNode> statsMap;
thread_local unsigned int callDepth = 0;
thread_local unsigned int statCount = 0;

};
