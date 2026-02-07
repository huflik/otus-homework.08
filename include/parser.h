#pragma once

#include <string>
#include "config.h"  

class Parser
{
public:
    Config Parse(int argc, char* argv[]);

private:
    static HashType ParseHashType(const std::string& str);
};
