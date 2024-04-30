#ifndef FOAMGENERATIONMAIN_H
#define FOAMGENERATIONMAIN_H

#include <string>
#include <unordered_map>
#include <any>

void runSimulationFromNode(const std::unordered_map<std::string, std::any>& params);

#endif // FOAMGENERATIONMAIN_H
