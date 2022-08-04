#pragma once
#include <fstream>
#include "slang/compilation/Compilation.h"
namespace verilog {
    class Netlist;
    void emit2(const slang::RootSymbol&, std::ifstream &input);
}
